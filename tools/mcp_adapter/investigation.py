"""Read recorded evidence; strict verification stays in the C++ replay implementation."""
import json
from arena_experiments.report import rows
from .storage import AdapterError, MIB, input_file, integer, json_file


def replay_inspect(trace, cursor=0, limit=64, operation=None, kinds=None, check_cancel=lambda: False):
    trace = input_file(trace, 256*MIB)
    integer(cursor, 0, 10000000, 'cursor')
    integer(limit, 1, 256, 'limit')
    if operation is not None:
        integer(operation, 1, 10000000, 'operation')
    allowed = {'header', 'request', 'event', 'response', 'state', 'complete'}
    if kinds is not None and (not kinds or set(kinds)-allowed):
        raise AdapterError('invalid_input', 'Unknown or empty trace record filter')
    result = dict(trace=str(trace), verification='not_checked', visibility='diagnostic',
                  records=[], footer_present=False, next_cursor=cursor, more=False)
    count = op = used = read = 0
    footer = False
    with trace.open('rb') as stream:
        while True:
            if check_cancel():
                raise AdapterError('cancelled', 'Trace inspection cancelled')
            line = stream.readline(2*MIB+4097)
            if not line:
                break
            read += len(line)
            if len(line) > 2*MIB+4096 or read > 256*MIB:
                raise AdapterError('artifact_limit', 'Trace exceeds record/file bound')
            if footer:
                result['trailing_data'] = True
                footer = False
                break
            try:
                record = json.loads(line)
                kind = record['type']
                if kind not in allowed or 'data' not in record:
                    raise ValueError('Unknown record shape')
            except (ValueError, TypeError, KeyError, RecursionError) as exc:
                result['parse_error'] = dict(record=count, message=str(exc))
                break
            if kind == 'header':
                result['header'] = record['data']
            if kind == 'request':
                op += 1
            if kind == 'complete':
                footer = record['data'] == {'complete': True}
            selected = count >= cursor and (operation is None or op == operation) and (kinds is None or kind in kinds)
            if selected:
                if not result['more'] and len(result['records']) < limit and used+len(line) <= 4*MIB:
                    result['records'].append(dict(cursor=count, operation=op, **record))
                    used += len(line)
                    result['next_cursor'] = count+1
                else:
                    result['more'] = True
            count += 1
    result.update(footer_present=footer, operation_count=op, record_count=count)
    if not result['more']:
        result['next_cursor'] = count
    return result


def trial_inspect(storage, directory, trial_id):
    root = storage.register(directory)
    input_file(root/'trials.jsonl', 4096*MIB)
    # Check rooted paths before existing report readers open them.
    if not (root/'trials.jsonl').resolve().is_relative_to(root):
        raise AdapterError('artifact_path', 'Trial journal escapes root')
    row = next((r for r in rows(root) if r['id'] == trial_id), None)
    if row is None:
        raise AdapterError('unknown_trial', 'Trial is not in this run')
    result = dict(trial=row, visibility='diagnostic', directory=str(root))
    if (root/'trial_index.json').exists():
        index = json_file(root, 'trial_index.json', 32*MIB)
        result['retention'] = next((r for r in index if r['id'] == trial_id), None)
    for key in ('trace', 'decisions'):
        if row.get(key):
            p = (root/row[key]).resolve()
            if not p.is_relative_to(root):
                raise AdapterError('artifact_path', 'Trial artifact escapes root')
            result[key] = dict(path=str(p), available=p.is_file())
    return result

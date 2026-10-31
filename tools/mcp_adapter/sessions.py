"""One synchronous Client per session, with stale-safe admission and bounded events."""
from collections import deque
from contextlib import contextmanager
import json
import threading
import time
import uuid

from headless_client import Client, ClientError, Cancelled, DeadlineExceeded
from arena_experiments.config import MIB, read_json
from arena_experiments.trial import trace_metadata
from .storage import AdapterError, input_file, integer


class EventRing:
    def __init__(self, count=4096, byte_limit=4*MIB):
        self.items = deque()
        self.next = 0
        self.bytes = 0
        self.count, self.byte_limit = count, byte_limit

    def append(self, chunk):
        for event in chunk['events']:
            n = len(json.dumps(event, separators=(',', ':')).encode())
            self.items.append((self.next, event, n))
            self.next += 1
            self.bytes += n
            while len(self.items) > self.count or self.bytes > self.byte_limit:
                self.bytes -= self.items.popleft()[2]

    def page(self, cursor, limit):
        integer(cursor, 0, 2**63-1, 'cursor')
        integer(limit, 1, 256, 'limit')
        first = self.items[0][0] if self.items else self.next
        if cursor < first:
            raise AdapterError('event_gap', 'Cursor has expired', earliest_cursor=first, next_cursor=self.next)
        if cursor > self.next:
            raise AdapterError('invalid_input', 'Cursor is beyond current events')
        page = [dict(cursor=i, event=e) for i, e, _ in self.items if i >= cursor][:limit]
        return dict(events=page, next_cursor=page[-1]['cursor']+1 if page else cursor,
                    available_through=self.next, visibility='public')


class Live:
    def __init__(self, root):
        self.handle = uuid.uuid4().hex
        self.root = root
        self.lock = threading.Lock()
        self.events = EventRing()
        self.client = None
        self.last_used = time.monotonic()
        self.cancel = threading.Event()
        self.check_cancel = lambda: False

    def cancelled(self):
        return self.cancel.is_set() or self.check_cancel()


class Sessions:
    def __init__(self, runner, assets, storage, *, client_factory=Client, idle_seconds=1800):
        self.runner, self.assets, self.storage = runner, assets, storage
        self.factory, self.idle_seconds = client_factory, idle_seconds
        self.lock = threading.RLock()
        self.live = {}
        self.closed = False

    def list(self):
        with self.lock:
            return [dict(handle=h, busy=s.lock.locked(), idle_seconds=time.monotonic()-s.last_used,
                         output=str(s.root), identity=dict(s.client.identity or {}) if s.client else None)
                    for h, s in self.live.items()]

    @contextmanager
    def access(self, handle, expected=None, check_cancel=None):
        with self.lock:
            s = self.live.get(handle)
            if not s:
                raise AdapterError('stale_session', 'Unknown, reset, closed or expired session handle')
            if not s.lock.acquire(blocking=False):
                raise AdapterError('session_busy', 'Another operation is in flight')
        try:
            if expected is not None:
                if any(not isinstance(expected.get(k), str) or not expected[k].isascii() or
                       not expected[k].isdecimal() for k in ('session', 'revision')):
                    raise AdapterError('invalid_input', 'Session and revision must be decimal strings')
                if expected != s.client.identity:
                    raise AdapterError('stale_revision', 'Expected identity does not match current observation',
                                       current=dict(s.client.identity or {}))
            s.check_cancel = check_cancel or (lambda: False)
            if s.cancelled():
                raise Cancelled('Request cancelled before dispatch')
            yield s
        finally:
            s.check_cancel = lambda: False
            s.last_used = time.monotonic()
            s.lock.release()

    def _dispose(self, s, orderly=True):
        try:
            c = s.client
            if c:
                if orderly and not c.closed and not c.failed:
                    c.cancelled = None
                    try:
                        c.request('close', _timeout=.5)
                    except Exception:
                        pass
                c.dispose()
        finally:
            with self.lock:
                self.live.pop(s.handle, None)
            self.storage.release(s.root)

    def _request(self, s, op, **fields):
        try:
            response = s.client.request(op, **fields)
            return dict(handle=s.handle, response=response, identity=dict(s.client.identity or {}),
                        events_cursor=s.events.next, trace=str(s.root/'trace.jsonl'))
        except (ClientError, OSError) as exc:
            self._dispose(s, orderly=False)
            code = 'cancelled' if isinstance(exc, Cancelled) else 'timeout' if isinstance(exc, DeadlineExceeded) else 'runner_failure'
            if 'graphics_unavailable' in str(exc):
                code = 'graphics_unavailable'
            raise AdapterError(code, str(exc), session_unusable=True,
                               mutation_may_have_completed=op in ('start', 'reset', 'act', 'advance', 'save', 'load', 'retry_terminal'),
                               trace=str(s.root/'trace.jsonl')) from exc

    def start(self, scenario=None, scenario_path=None, mode='diagnostic', seed=None,
              handle=None, session=None, revision=None, check_cancel=None, campaign_save_path=None):
        if mode not in ('diagnostic', 'visual') or sum(x is not None for x in (scenario, scenario_path, campaign_save_path)) != 1:
            raise AdapterError('invalid_input', 'Choose diagnostic/visual and exactly one scenario, scenario_path or campaign_save_path')
        if scenario_path:
            scenario = read_json(input_file(scenario_path, MIB))
        if campaign_save_path is None and (not isinstance(scenario, dict) or len(json.dumps(scenario, allow_nan=False).encode()) > MIB):
            raise AdapterError('invalid_input', 'Scenario must be an object no larger than 1 MiB')
        if campaign_save_path is not None and (handle or seed is not None):
            raise AdapterError('invalid_input', 'Resume into a new session without a seed override')
        saved_bytes = None
        if campaign_save_path is not None:
            with input_file(campaign_save_path, 16*MIB).open('rb') as source:
                saved_bytes = source.read(16*MIB+1)
            if len(saved_bytes) > 16*MIB:
                raise AdapterError('invalid_input', 'Campaign save exceeds 16 MiB')
        fields = dict(scenario=scenario) if saved_bytes is None else {}
        if seed is not None:
            fields['seed'] = integer(seed, 0, 2**32-1, 'seed')
        if handle:
            with self.access(handle, dict(session=session, revision=revision), check_cancel) as s:
                if mode != s.mode:
                    raise AdapterError('invalid_input', 'Replacement retains observation mode; use a new session to change it')
                result = self._request(s, 'start', **fields)
                return self._rotate(s, result)
        with self.lock:
            if self.closed:
                raise AdapterError('shutdown', 'Server is closing')
            if len(self.live) >= 2:
                raise AdapterError('session_capacity', 'At most two live sessions')
            s = Live(self.storage.reserve('session', 290*MIB))
            s.mode = mode
            s.lock.acquire()
            self.live[s.handle] = s
        try:
            s.check_cancel = check_cancel or (lambda: False)
            slot = s.root/'campaign.json'
            if saved_bytes is not None:
                slot.write_bytes(saved_bytes)
            s.client = self.factory(self.runner, self.assets, mode, s.root/'trace.jsonl', s.events.append,
                                    timeout=10, startup_timeout=30, cancelled=s.cancelled, campaign_save=slot)
            result = self._request(s, 'load' if saved_bytes is not None else 'start', **fields)
            if not result['response']['ok']:
                self._dispose(s)
                result['handle'] = None
            return result
        except BaseException:
            self._dispose(s)
            raise
        finally:
            s.check_cancel = lambda: False
            s.last_used = time.monotonic()
            s.lock.release()

    def _rotate(self, s, result):
        if result['response']['ok']:
            with self.lock:
                self.live.pop(s.handle)
                s.handle = uuid.uuid4().hex
                self.live[s.handle] = s
            result['handle'] = s.handle
        return result

    def request(self, handle, op, session=None, revision=None, check_cancel=None, **fields):
        if op not in ('observe', 'inspect', 'actions', 'preview', 'act', 'advance', 'reset', 'save_status', 'preview_save', 'save', 'load', 'retry_terminal'):
            raise AdapterError('invalid_input', 'Unsupported session operation')
        if op == 'advance':
            integer(fields.get('ticks'), 0, 1000, 'ticks')
        expected = dict(session=session, revision=revision) if op in ('preview', 'act', 'advance', 'reset', 'preview_save', 'save', 'load', 'retry_terminal') else None
        with self.access(handle, expected, check_cancel) as s:
            result = self._request(s, op, **fields)
            if op == 'save' and result['response'].get('ok') and result['response'].get('data', {}).get('published'):
                result['save_artifact'] = str(s.root/'campaign.json')
            return self._rotate(s, result) if op in ('reset', 'load') else result

    def events(self, handle, cursor=0, limit=256):
        with self.access(handle) as s:
            return s.events.page(cursor, limit)

    def close(self, handle, check_cancel=None):
        with self.access(handle, check_cancel=check_cancel) as s:
            result = self._request(s, 'close')
            self._dispose(s)
            _, complete, size = trace_metadata(s.root/'trace.jsonl')
            result.update(trace_complete=complete, trace_bytes=size, runner_exit=s.client.process.returncode)
            if s.client.process.returncode:
                raise AdapterError('runner_failure', 'Runner failed during recording close',
                                   trace=result['trace'], trace_complete=complete,
                                   runner_exit=s.client.process.returncode)
            return result

    def sweep(self):
        with self.lock:
            expired = [s for s in self.live.values() if time.monotonic()-s.last_used >= self.idle_seconds]
        for s in expired:
            if s.lock.acquire(blocking=False):
                try:
                    self._dispose(s)
                finally:
                    s.lock.release()

    def shutdown(self):
        with self.lock:
            self.closed = True
            values = list(self.live.values())
            for s in values:
                s.cancel.set()
        for s in values:
            # Client checks cancellation at most every 50ms and bounds its cleanup.
            with s.lock:
                self._dispose(s)

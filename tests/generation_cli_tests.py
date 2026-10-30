"""Small owned generator/transport contracts; no balance runs or broad seed sweeps."""
import json
from collections import deque
from pathlib import Path
import subprocess
import sys
import tempfile
from headless_cli_tests import fixture

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from headless_client import Client
from mcp_adapter.sessions import Sessions
from mcp_adapter.storage import Storage, MIB, AdapterError


def open_door(request, state):
    """One diagnostic, unpopulated fixture route through the real transport."""
    door = next(f for f in state['generation']['fixtures'] if f['kind'] == 0 and f['id'] != 0)
    rows = state['generation']['map'].splitlines()
    directions = [(0, -1), (1, 0), (0, 1), (-1, 0)]
    dx, dy = directions[door['heading']]
    target = door['position'][0]-dx, door['position'][1]-dy
    start = tuple(state['position'])
    queue, previous = deque([start]), {start: None}
    while queue:
        here = queue.popleft()
        for dx, dy in directions:
            nxt = here[0]+dx, here[1]+dy
            if nxt not in previous and nxt != tuple(door['position']) and rows[nxt[1]][nxt[0]] != '#':
                previous[nxt] = here
                queue.append(nxt)
    assert target in previous
    path = []
    at = target
    while previous[at] is not None:
        path.append(at)
        at = previous[at]
    def move(direction):
        nonlocal state
        assert request('act', action=dict(kind='move', direction=direction))['data']['accepted']
        state = request('advance', ticks=17, until_decision=False)['data']['observation']
    def face(heading):
        for _ in range(4):
            if state['heading'] == heading:
                return
            move('turn_right')
        raise AssertionError('heading')
    for at in reversed(path):
        face(directions.index((at[0]-state['position'][0], at[1]-state['position'][1])))
        move('forward')
    face(door['heading'])
    action = dict(kind='interact', fixture=door['id'])
    assert request('preview', action=action)['data']['accepted']
    opened = request('act', action=action)['data']
    assert opened['accepted'] and next(f for f in opened['observation']['fixtures'] if f['fixture'] == door['id'])['completed']
    assert request('act', action=action)['data']['accepted']
    move('forward')
    assert state['position'] == door['position']


def main(generator, runner):
    def generate(*args, okay=True):
        p = subprocess.run([generator, *args], capture_output=True, text=True, timeout=10)
        assert (p.returncode == 0) == okay, (p.stdout, p.stderr)
        return p
    a = json.loads(generate('--seed', '5', '--format', 'json').stdout)
    b = json.loads(generate('--seed', '5', '--format', 'json').stdout)
    assert a['fingerprint'] == b['fingerprint'] and a['map'] == b['map']
    forced = json.loads(generate('--seed', '5', '--format', 'json', '--force-fallback').stdout)
    assert forced['fallback'] and forced['attempts'] == 16 and len(forced['rejections']) == 16
    # Two records test the streaming interface only, not generator coverage/performance.
    stream = [json.loads(line) for line in generate('--seed', '5', '--count', '2').stdout.splitlines()]
    assert len(stream) == 3 and stream[-1]['count'] == 2 and stream[-1]['failures'] == 0
    assert all('attempts' in row and 'rejections' in row for row in stream[:2])
    for args in ((), ('--seed', '-1'), ('--seed', '4294967296'), ('--seed', '0', '--depth', '0'),
                 ('--seed', '0', '--count', '0'), ('--seed', '0', '--count', '100001'),
                 ('--seed', '4294967295', '--count', '2'), ('--seed', '5', '--format', 'bad')):
        generate(*args, okay=False)
    with tempfile.TemporaryDirectory(prefix='blobber-pc01-cli-') as temp:
        root = Path(temp)
        arena = fixture(root)
        catalog_path = root / 'rules/catalog.json'
        cat = json.loads(catalog_path.read_text())
        enemy = cat['monsters'][0]
        for name in ('goblin', 'goblin.archer', 'goblin.guardian.placeholder'):
            cat['monsters'].append(dict(enemy, id=name))
        for name in ('healing_potion', 'mana_potion'):
            cat['items'].append(dict(cat['items'][1], id=name))
        catalog_path.write_text(json.dumps(cat))
        npc = json.loads((root / 'npcs/enemy.json').read_text())
        for name in ('goblin', 'goblin.archer'):
            (root / f'npcs/{name}.json').write_text(json.dumps(dict(npc, id=name)))
        (root / 'npcs/level.json').write_text(json.dumps(dict(definitions=['enemy.json', 'goblin.json', 'goblin.archer.json'], instances=[])))
        setup = dict(kind='generated_floor', schema_version=1, generator_version=3,
                     template_version=3, seed=5, depth=1, party=arena['party'])
        trace = root / 'direct.jsonl'
        with Client(runner, root, record=trace) as client:
            start = client.request('start', scenario=setup)
            assert start['ok'] and start['data']['generation']['fingerprint'] == a['fingerprint']
            initial = start['data']
            assert initial['objective']['members'] == [2, 3] and not initial['objective']['completed']
            assert client.request('act', action=dict(kind='move', direction='forward'))['data']['accepted']
            assert client.request('advance', ticks=100, until_decision=True)['ok']
            state = client.request('observe')['data']
            open_door(client.request, state)
            assert client.request('reset')['data']['generation'] == initial['generation']
            bad = dict(setup, generator_version=999)
            assert not client.request('start', scenario=bad)['ok']
            assert client.request('observe')['data']['generation'] == initial['generation']
        replay = subprocess.run([runner, '--assets', str(root), '--replay', str(trace)],
                                capture_output=True, text=True, timeout=15)
        assert replay.returncode == 0, (replay.stdout, replay.stderr)
        sessions = Sessions(runner, root, Storage(root / 'adapter', 1024 * MIB))
        try:
            result = sessions.start(scenario=setup)
            assert result['response']['ok']
            assert result['response']['data']['generation'] == initial['generation']
            assert result['response']['data']['objective'] == initial['objective']
            tokens = dict(handle=result['handle'], **result['identity'])
            bad = sessions.start(scenario=dict(setup, depth=0), **tokens)
            assert not bad['response']['ok'] and bad['handle'] == result['handle']
            reset = sessions.request(op='reset', **tokens)
            assert reset['handle'] != result['handle']
            try:
                sessions.request(op='act', action=dict(kind='move', direction='forward'), **tokens)
                raise AssertionError('stale adapter handle accepted')
            except AdapterError as error:
                assert error.code == 'stale_session'
            active = dict(handle=reset['handle'], **reset['identity'])
            def adapter_request(op, **fields):
                result = sessions.request(op=op, **active, **fields)
                active.update(handle=result['handle'], **result['identity'])
                return result['response']
            open_door(adapter_request, reset['response']['data'])
            assert sessions.close(active['handle'])['trace_complete']
        finally:
            sessions.shutdown()
        assert not sessions.live
    print('Generator CLI, direct client, strict replay and real MCP session adapter passed')


if __name__ == '__main__':
    main(*sys.argv[1:])

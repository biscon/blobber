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
    blocked = {tuple(f['position']) for f in state['generation']['fixtures'] if f['kind'] in (2,3)}
    start = tuple(state['position'])
    queue, previous = deque([start]), {start: None}
    while queue:
        here = queue.popleft()
        for dx, dy in directions:
            nxt = here[0]+dx, here[1]+dy
            if nxt not in previous and nxt not in blocked and nxt != tuple(door['position']) and rows[nxt[1]][nxt[0]] != '#':
                previous[nxt] = here
                queue.append(nxt)
    if target not in previous:
        door_dx, door_dy = directions[door['heading']]
        target = door['position'][0]+door_dx, door['position'][1]+door_dy
        assert target in previous
        door = dict(door, heading=(door['heading']+2)%4)
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
    assert not request('act', action=action)['data']['accepted']
    state = request('advance', ticks=30, until_decision=False)['data']['observation']
    closed = request('act', action=action)['data']
    assert closed['accepted'] and not next(f for f in closed['observation']['fixtures'] if f['fixture'] == door['id'])['open']
    assert not request('act', action=dict(kind='move', direction='forward'))['data']['accepted']
    state = request('advance', ticks=30, until_decision=False)['data']['observation']
    assert request('act', action=action)['data']['accepted']
    state = request('advance', ticks=30, until_decision=False)['data']['observation']
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
    assert all(4 <= r['ceiling_ticks'] <= 8 for r in a['rooms'])
    assert all(r['ceiling_ticks'] == 4 for r in a['rooms'] if r['role'] in ('entrance', 'objective'))
    assert any(r['ceiling_ticks'] > 4 for r in a['rooms'])
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
        for name in ('goblin', 'goblin.archer', 'skeleton', 'zombie', 'giant.rat', 'goblin_shaman', 'bugbear'):
            cat['monsters'].append(dict(enemy, id=name))
        for name in ('goblin.captain', 'bugbear.captain', 'bugbear.commander'):
            cat['monsters'].append(dict(enemy, id=name))
        for name in ('healing_potion', 'mana_potion', 'arrow'):
            cat['items'].append(dict(cat['items'][1], id=name, **(dict(kind='ammunition',stack_limit=100,ammunition='arrow') if name=='arrow' else {})))
        catalog_path.write_text(json.dumps(cat))
        npc = json.loads((root / 'npcs/enemy.json').read_text())
        for name in ('goblin', 'goblin.archer', 'skeleton', 'zombie', 'giant.rat', 'goblin_shaman', 'bugbear'):
            (root / f'npcs/{name}.json').write_text(json.dumps(dict(npc, id=name)))
        residents = ('mayor', 'boric', 'balin', 'innkeeper', 'blacksmith', 'shadowtart')
        for name in residents:
            (root/f'npcs/{name}.json').write_text(json.dumps(dict(npc, id=name, role='resident')))
        definitions = ['enemy.json', *[n+'.json' for n in ('goblin','goblin.archer','skeleton','zombie','giant.rat','goblin_shaman','bugbear')], *[n+'.json' for n in residents]]
        (root/'npcs/level.json').write_text(json.dumps(dict(definitions=definitions, instances=[])))
        setup = dict(kind='generated_floor', schema_version=1, generator_version=13,
                     template_version=9, seed=5, depth=1, party=arena['party'])
        trace = root / 'direct.jsonl'
        with Client(runner, root, record=trace) as client:
            start = client.request('start', scenario=setup)
            assert start['ok'] and start['data']['generation']['fingerprint'] == a['fingerprint']
            initial = start['data']
            assert initial['objective']['members'] == [p['id'] for p in a['population'] if p['guardian']] and not initial['objective']['completed']
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
        campaign = dict(setup, kind='campaign')
        campaign.pop('depth')
        def check_campaign(request):
            started = request('start', scenario=campaign)
            assert started['ok'] and started['data']['campaign']['location'] == 0
            # Walk from the current city square to the dungeon stair approach.
            for direction in ('right', 'right', 'forward'):
                moved = request('act', action=dict(kind='move', direction=direction))['data']
                assert moved['accepted'], moved
                request('advance', ticks=100, until_decision=False)
            preview = request('preview', action=dict(kind='interact', fixture=0))['data']
            assert preview['accepted'], preview
            entered = request('act', action=dict(kind='interact', fixture=0))['data']
            assert entered['accepted'] and entered['observation']['campaign']['location'] == 1
            up = next(f['fixture'] for f in entered['observation']['fixtures'] if f['kind'] == 2)
            # Arrival faces into the room; turn back toward the dedicated stairwell.
            for _ in range(2):
                assert request('act', action=dict(kind='move', direction='turn_right'))['data']['accepted']
                request('advance', ticks=17, until_decision=False)
            returned = request('act', action=dict(kind='interact', fixture=up))['data']
            assert returned['accepted'] and returned['observation']['campaign']['location'] == 0
            assert returned['observation']['campaign']['visited_entries'] == [1]
            # Upstairs arrival faces away from the city stair entrance.
            for _ in range(2):
                assert request('act', action=dict(kind='move', direction='turn_right'))['data']['accepted']
                request('advance', ticks=17, until_decision=False)
            assert not request('act', action=dict(kind='waypoint', depth=2))['data']['accepted']
            back = request('act', action=dict(kind='waypoint', depth=1))['data']
            assert back['accepted'] and back['observation']['campaign']['location'] == 1
            eligibility = back['observation']['campaign']['travel']['recall']
            kits = eligibility['kits']
            assert kits > 0 and eligibility['ready']
            assert request('preview', action=dict(kind='recall'))['data']['accepted']
            recalled = request('act', action=dict(kind='recall'))['data']
            assert recalled['accepted'] and recalled['observation']['campaign']['location'] == 0
            assert recalled['observation']['campaign']['travel']['recall']['kits'] == kits - 1
            # Step back from Sanctuary's recessed door, then east to the stair landing.
            for direction in ('back', 'right', 'right', 'right', 'right'):
                moved = request('act', action=dict(kind='move', direction=direction))['data']
                assert moved['accepted'], moved
                request('advance', ticks=100, until_decision=False)
            back = request('act', action=dict(kind='waypoint', depth=1))['data']
            assert back['accepted'], back
        campaign_trace = root / 'campaign.jsonl'
        with Client(runner, root, record=campaign_trace) as client:
            check_campaign(client.request)
        replay = subprocess.run([runner, '--assets', str(root), '--replay', str(campaign_trace)],
                                capture_output=True, text=True, timeout=15)
        assert replay.returncode == 0, (replay.stdout, replay.stderr)
        sessions = Sessions(runner, root, Storage(root / 'campaign-adapter', 1024 * MIB))
        active = {}
        def campaign_request(op, **fields):
            if op == 'start':
                result = sessions.start(**fields)
            else:
                result = sessions.request(op=op, **active, **fields)
            active.update(handle=result['handle'], **result['identity'])
            return result['response']
        try:
            check_campaign(campaign_request)
            assert sessions.close(active['handle'])['trace_complete']
        finally:
            sessions.shutdown()
    print('Generator CLI, direct client, strict replay and real MCP session adapter passed')


if __name__ == '__main__':
    main(*sys.argv[1:])

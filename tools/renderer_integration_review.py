#!/usr/bin/env python3
"""Explicit single-expedition renderer review. Ordinary commands; no balance experiment or OS input.

The route uses labelled diagnostic inspection; command admission still uses the
selected visual backend. Images are captured separately by strict replay.
"""
import argparse
from collections import deque
import json
from pathlib import Path
from headless_client import Client

DIRECTIONS = [(0, -1), (1, 0), (0, 1), (-1, 0)]


def run(args):
    args.output.mkdir(parents=True, exist_ok=False)
    scenario = json.loads(args.scenario.read_text())
    selected = {}
    state = None
    with Client(args.runner, args.assets, 'visual', args.output/'trace.jsonl',
                visibility=args.backend, campaign_save=args.output/'campaign.json') as client:
        def request(op, **fields):
            nonlocal state
            reply = client.request(op, **fields)
            if not reply['ok']:
                raise RuntimeError((op, reply))
            data = reply['data']
            if isinstance(data, dict) and 'observation' in data:
                state = data['observation']
            elif isinstance(data, dict) and 'position' in data:
                state = data
            return data

        def mark(label):
            selected[label] = client.sequence

        def act(action, required=True):
            result = request('act', action=action)
            if required and not result['accepted']:
                raise RuntimeError((action, result['reason'], state['phase']))
            return result['accepted']

        def preview(action):
            return request('preview', action=action)['accepted']

        def advance(ticks=25):
            request('advance', ticks=ticks, until_decision=False)

        def fight():
            for step in range(600):
                request('observe')
                if state['phase'] == 'defeat':
                    raise RuntimeError('Review expedition defeated; no replacement state fabricated')
                if state['phase'] == 'victory':
                    mark('combat-victory')
                    act(dict(kind='continue'))
                    advance(100)
                    return
                if state['awaiting_action']:
                    members = [state['active_character']]
                elif state['phase'] in ('exploration', 'approach'):
                    members = [2, 3, 1]
                else:
                    advance()
                    continue
                accepted = False
                for member in members:
                    for target in state['targets']:
                        for attack in ('melee', 'ranged'):
                            action = dict(kind='combat', character=member, action=attack, target=target)
                            if preview(action):
                                act(action); accepted = True
                                if 'combat-start' not in selected:
                                    advance(10); mark('combat-start')
                                break
                        if accepted:
                            break
                        if member in (1, 3):
                            action = dict(kind='combat', character=member, action='spell',
                                          spell='sacred_flame' if member == 1 else 'fire_bolt', enemies=[target])
                            if preview(action):
                                act(action); accepted = True; break
                    if accepted:
                        break
                if not accepted and state['awaiting_action']:
                    action = dict(kind='combat', character=state['active_character'], action='defend')
                    if not preview(action):
                        action.update(action='feature', feature='end_turn')
                    act(action)
                advance()
            raise RuntimeError('Review combat exceeded its bounded script')

        def walk(target):
            detail = request('inspect')
            rows = detail['generation']['map'].splitlines()
            fixtures = {tuple(f['position']): f for f in detail['fixtures']}
            start = tuple(state['position']); target = tuple(target)
            queue, previous = deque([start]), {start: None}
            while queue and target not in previous:
                here = queue.popleft()
                for dx, dy in DIRECTIONS:
                    cell = here[0]+dx, here[1]+dy
                    if not (0 <= cell[1] < len(rows) and 0 <= cell[0] < len(rows[0])):
                        continue
                    fixture = fixtures.get(cell)
                    if rows[cell[1]][cell[0]] == '#' or cell in previous:
                        continue
                    if fixture and fixture['kind'] == 5 and not fixture['completed']:
                        continue
                    previous[cell] = here; queue.append(cell)
            if target not in previous:
                raise RuntimeError(('No reviewed route', target))
            path, cell = [], target
            while cell != start:
                path.append(cell); cell = previous[cell]
            for cell in reversed(path):
                if state['phase'] not in ('exploration', 'approach'):
                    fight()
                delta = cell[0]-state['position'][0], cell[1]-state['position'][1]
                heading = DIRECTIONS.index(delta)
                for _ in range(4):
                    if state['heading'] == heading:
                        break
                    act(dict(kind='move', direction='turn_right')); advance(17)
                fixture = fixtures.get(cell)
                if fixture and fixture['kind'] == 0 and not fixture['completed']:
                    act(dict(kind='interact', fixture=fixture['fixture']))
                    advance(15)
                    if 'door-opening' not in selected:
                        mark('door-opening')
                    advance(15); fixture['completed'] = True
                if not act(dict(kind='move', direction='forward'), required=False):
                    fight()
                    act(dict(kind='move', direction='forward'))
                advance(17)

        request('start', scenario=scenario); mark('hub')
        hub = request('inspect')
        act(dict(kind='interact', fixture=hub['fixtures'][0]['fixture'])); mark('floor1')
        detail = request('inspect')
        approach = next(f for f in detail['generation']['fixtures'] if f['id'] == detail['generation']['objective']['approach_door'])
        dx, dy = DIRECTIONS[approach['heading']]
        walk((approach['position'][0]-dx, approach['position'][1]-dy))
        for _ in range(4):
            if state['heading'] == approach['heading']:
                break
            act(dict(kind='move', direction='turn_right')); advance(17)
        act(dict(kind='interact', fixture=approach['id'])); advance(30)
        act(dict(kind='move', direction='forward'), required=False); advance(17)
        fight()
        detail = request('inspect')
        if not detail['objective']['completed']:
            raise RuntimeError('Review did not complete designated guardians')
        reward = next(iter(detail['loot']), None)
        if reward:
            walk(reward['position'])
            if act(dict(kind='pickup', loot=reward['handle'], character=0), required=False):
                mark('loot')
        detail = request('inspect')
        stair = next(f for f in detail['fixtures'] if f['kind'] == 3)
        walk(stair['position'])
        act(dict(kind='interact', fixture=stair['fixture'])); mark('floor2')
        act(dict(kind='recall')); mark('recall')
        slot = request('save_status')['slot_revision']
        if not request('save', slot_revision=slot)['durable']:
            raise RuntimeError('Review save was not durable')
        request('load', replace=True); mark('continue')
        request('close')
    (args.output/'review.json').write_text(json.dumps(dict(selected=selected, backend=args.backend,
        note='One deterministic diagnostic-routed command trace; effects gallery is separate.'), indent=2)+'\n')
    print(json.dumps(selected))


def compare(args):
    """Replay identical requests through the other backend and compare simulation results."""
    args.output.mkdir(parents=True, exist_ok=False)
    source = [json.loads(line) for line in args.compare_trace.read_text().splitlines()]
    requests = [r['data'] for r in source if r['type'] == 'request']
    responses = [r['data'] for r in source if r['type'] == 'response']
    states = [r['data'] for r in source if r['type'] == 'state']
    differences = []
    def gameplay(state):
        if not state:
            return None
        result = {key: state.get(key) for key in ('position', 'heading', 'phase', 'round', 'gameplay_ms',
                  'time_ms', 'party', 'objective', 'quest_keys', 'fixtures', 'ground', 'door_opening',
                  'attacks', 'pending_count', 'ai_time', 'loot')}
        result['enemies'] = [{k:v for k,v in enemy.items() if k != 'targetable'} for enemy in state.get('enemies', [])]
        return result
    with Client(args.runner, args.assets, 'visual', args.output/'trace.jsonl', visibility=args.backend,
                campaign_save=args.output/'campaign.json') as client:
        for number, (request, expected, state) in enumerate(zip(requests, responses, states), 1):
            fields = {k:v for k,v in request.items() if k not in ('op','id','version','session','revision')}
            actual = client.request(request['op'], **fields)
            if actual['ok'] != expected['ok'] or (isinstance(expected.get('data'), dict) and
                actual.get('data', {}).get('accepted') != expected['data'].get('accepted')):
                differences.append(dict(operation=number, reason='admission'))
            # Compare diagnostic observations from the trace only at existing inspect requests;
            # comparing does not insert extra ticks or observation requests into the sequence.
            if request['op'] == 'inspect' and gameplay(actual.get('data')) != gameplay(state):
                differences.append(dict(operation=number, reason='gameplay state'))
    report = dict(requests=len(requests), differences=differences, backend=args.backend,
                  compared='Admission at every request; canonical gameplay at diagnostic inspections')
    (args.output/'comparison.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report))
    if differences:
        raise RuntimeError('Renderer backend gameplay comparison differed')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runner', type=Path, required=True)
    parser.add_argument('--assets', type=Path, required=True)
    parser.add_argument('--scenario', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--compare-trace', type=Path)
    parser.add_argument('--backend', choices=('legacy', 'world'), default='world')
    args = parser.parse_args()
    compare(args) if args.compare_trace else run(args)

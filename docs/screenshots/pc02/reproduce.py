#!/usr/bin/env python3
"""Explicit diagnostic fixture walk/replay captures; no enemies or balance policy."""
import argparse
from collections import deque
import json
from pathlib import Path
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools'))
from headless_client import Client


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument('--runner', type=Path, required=True)
    parser.add_argument('--assets', type=Path, required=True)
    parser.add_argument('--scenario', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    scenario = json.loads(args.scenario.read_text())
    trace = args.output / 'walk.jsonl'
    samples = []
    directions = [(0, -1), (1, 0), (0, 1), (-1, 0)]
    with Client(args.runner.resolve(), args.assets.resolve(), record=trace, timeout=30) as client:
        def request(op, **fields):
            result = client.request(op, **fields)
            assert result['ok'], result
            return result['data']
        state = request('start', scenario=scenario)
        generation = state['generation']
        (args.output / 'generation.json').write_text(json.dumps(generation, indent=2)+'\n')
        def sample(label):
            samples.append(dict(operation=client.sequence, label=label))
        def action(payload, accepted=True):
            nonlocal state
            result = request('act', action=payload)
            assert result['accepted'] == accepted, result
            state = result['observation']
        def move(direction):
            nonlocal state
            action(dict(kind='move', direction=direction))
            state = request('advance', ticks=17, until_decision=False)['observation']
        def face(heading):
            for _ in range(4):
                if state['heading'] == heading:
                    return
                move('turn_right')
            raise AssertionError('heading')
        def walk(target):
            target = tuple(target)
            start = tuple(state['position'])
            rows = generation['map'].splitlines()
            closed = {tuple(f['position']) for f in state['fixtures']
                      if f['kind'] == 0 and not f['completed']}
            previous = {start: None}
            queue = deque([start])
            while queue:
                here = queue.popleft()
                for dx, dy in directions:
                    nxt = here[0]+dx, here[1]+dy
                    if not (0 <= nxt[1] < len(rows) and 0 <= nxt[0] < len(rows[0])):
                        continue
                    if nxt not in previous and nxt not in closed and rows[nxt[1]][nxt[0]] != '#':
                        previous[nxt] = here
                        queue.append(nxt)
            assert target in previous, ('unreachable', target)
            path = []
            at = target
            while previous[at] is not None:
                path.append(at)
                at = previous[at]
            for at in reversed(path):
                dx, dy = at[0]-state['position'][0], at[1]-state['position'][1]
                face(directions.index((dx, dy)))
                move('forward')
        sample('entrance')
        door = next(f for f in generation['fixtures'] if f['kind'] == 0)
        dx, dy = directions[door['heading']]
        approach = door['position'][0]-dx, door['position'][1]-dy
        walk(approach)
        face(door['heading'])
        sample('closed-door')
        action(dict(kind='interact', fixture=door['id']), accepted=not bool(door['key']))
        key = next((f for f in generation['fixtures'] if f['kind'] == 1), None)
        if key:
            walk(key['position'])
            sample('key-before')
            action(dict(kind='interact', fixture=key['id']))
            sample('key-collected')
            action(dict(kind='interact', fixture=key['id']))
            walk(approach)
            face(door['heading'])
            sample('door-with-key')
            action(dict(kind='interact', fixture=door['id']))
        sample('opened-door')
        action(dict(kind='interact', fixture=door['id']))
        move('forward')
        sample('open-threshold')
        goal = next(f for f in generation['fixtures'] if f['kind'] in (3, 4))
        walk(goal['position'])
        sample('endpoint')
        request('close')
    (args.output / 'samples.json').write_text(json.dumps(samples, indent=2)+'\n')
    command = [str(args.runner.resolve()), '--assets', str(args.assets.resolve()), '--replay', str(trace),
               '--capture-dir', str(args.output / 'captures')]
    for item in samples:
        command += ['--capture-operation', str(item['operation'])]
    with (args.output / 'capture.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    print(json.dumps(samples))


if __name__ == '__main__':
    main()

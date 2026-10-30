#!/usr/bin/env python3
"""Explicit application-owned capture review, using supplied content. Never a default test.

Starts a recorded visual session, approaches the encounter with ordinary commands,
then verifies/captures selected boundaries through MCP. No OS input or focus control.
"""
import argparse
import json
import os
from pathlib import Path
import sys

import anyio
from mcp import Client
from mcp.client.stdio import StdioServerParameters


async def review(args):
    project = Path(__file__).resolve().parents[1]
    args.output.mkdir(parents=True, exist_ok=False)
    params = StdioServerParameters(command=sys.executable, args=[str(project/'tools/blobber_mcp.py'),
        '--runner',str(args.runner.resolve()),'--assets',str(args.assets.resolve()),
        '--output-root',str(args.output.resolve())],
        env={k:os.environ[k] for k in ('DISPLAY','WAYLAND_DISPLAY','XAUTHORITY','XDG_RUNTIME_DIR') if k in os.environ})
    async with Client(params, read_timeout_seconds=45) as client:
        async def call(name, **fields):
            result = await client.call_tool(name, fields)
            if result.is_error:
                raise RuntimeError(str(result))
            return result.structured_content
        r = await call('session_start', scenario_path=str(args.scenario.resolve()), mode='visual')
        selected = [1]
        handle = r['handle']
        for n in range(48):
            state = r['response']['data'].get('observation', r['response']['data'])
            if state['phase'] not in ('exploration','approach'):
                break
            tokens = dict(handle=handle, **r['identity'])
            if state['moving']:
                r = await call('session_advance', **tokens, ticks=25)
            else:
                r = await call('session_act', **tokens, action={'kind':'move','direction':'forward'})
            if n in (0, 1):
                selected.append(int(r['response']['id'])+1)
        selected.append(int(r['response']['id'])+1)
        trace = (await call('session_close', handle=handle))['trace']
        job = await call('replay_capture', trace=trace, operations=sorted(set(selected)))
        for _ in range(1200):
            status = await call('job_status', job_id=job['job_id'])
            if status['state'] not in ('running','cancelling'):
                break
            await anyio.sleep(.25)
        if status['state'] != 'completed':
            raise RuntimeError(str(status))
        root = Path(status['output'])/'captures'
        metadata = json.loads((root/'capture.json').read_text())
        for item in metadata['images']:
            image = await client.call_tool('artifact_read', {'path':str(root/item['file']), 'image':True})
            assert not image.is_error and image.content[0].type == 'image'
        (args.output/'review.json').write_text(json.dumps(dict(job=status, capture=metadata), indent=2)+'\n')
        print(json.dumps(dict(output=str(root), operations=selected, replay=metadata['replay']), indent=2))


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runner',type=Path,required=True)
    parser.add_argument('--assets',type=Path,required=True)
    parser.add_argument('--scenario',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    anyio.run(review, parser.parse_args())

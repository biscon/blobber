"""Explicit optional-SDK stdio contracts; --playtest opts into a fixture encounter and experiment.

No balance matrix is registered with CTest. Fixtures belong to these tests.
"""
import argparse
import json
import os
from pathlib import Path
import sys
import tempfile

import anyio
from mcp import Client as MCPClient
from mcp.client.stdio import StdioServerParameters

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from headless_client import Client
from headless_cli_tests import fixture


async def main(runner, playtest=False, evidence=None):
    with tempfile.TemporaryDirectory(prefix='blobber-mcp-') as temporary:
        root = Path(temporary)
        assets = root/'assets'; assets.mkdir()
        scenario = fixture(assets)
        (root/'scenario.json').write_text(json.dumps(scenario))
        params = StdioServerParameters(command=sys.executable, args=[str(ROOT/'tools/blobber_mcp.py'),
                    '--workspace', str(ROOT), '--runner', str(runner.resolve()), '--assets', str(assets),
                    '--output-root', str(root/'output')])
        # Test legacy negotiation too, because installed agent clients may use an older protocol revision.
        async with MCPClient(params, mode='legacy', read_timeout_seconds=45) as mcp:
            listing = await mcp.list_tools()
            names = {t.name for t in listing.tools}
            assert {'session_start','job_cancel','replay_capture','artifact_read'} <= names
            async def call(name, **fields):
                result = await mcp.call_tool(name, fields)
                assert not result.is_error, (name, result)
                assert result.structured_content is not None, result
                return result.structured_content
            bad_schema = await mcp.call_tool('session_start', {'scenario':{}, 'unexpected':True})
            assert bad_schema.is_error
            bad_ticks = await mcp.call_tool('session_advance', {'handle':'none','session':'1','revision':'1','ticks':True})
            assert bad_ticks.is_error
            cap = await call('capabilities')
            assert cap['engine']['protocol'] == 1
            r = await call('session_start', scenario=scenario)
            handle = r['handle']
            state = r['response']['data']
            with Client(runner.resolve(), assets) as direct:
                d = direct.request('start', scenario=scenario)
                assert state == d['data']
                args = dict(handle=handle, **r['identity'])
                action = dict(kind='move', direction='forward')
                preview = await call('session_preview', **args, action=action)
                assert preview['response']['data'] == direct.request('preview', action=action)['data']
                moved = await call('session_act', **args, action=action)
                assert moved['response']['data'] == direct.request('act', action=action)['data']
                stale = await mcp.call_tool('session_act', dict(args, action=action))
                assert stale.is_error and stale.structured_content['error']['code'] == 'stale_revision'
                r = await call('session_advance', handle=handle, **moved['identity'], ticks=25)
                assert r['response']['data'] == direct.request('advance', ticks=25)['data']
            invalid = await mcp.call_tool('session_start', dict(scenario={'invalid': True}, handle=handle, **r['identity']))
            assert invalid.is_error
            assert (await call('session_observe', handle=handle))['identity'] == r['identity']
            reset = await call('session_reset', handle=handle, **r['identity'])
            stale = await mcp.call_tool('session_observe', {'handle': handle})
            assert stale.is_error
            handle = reset['handle']
            r = reset
            if playtest:
                for _ in range(1000):
                    state = r['response']['data'].get('observation', r['response']['data'])
                    if state['terminal']:
                        break
                    tokens = dict(handle=handle, **r['identity'])
                    if state['awaiting_action']:
                        action = dict(kind='combat', character=state['active_character'], action='melee', target=0)
                        preview = await call('session_preview', **tokens, action=action)
                        if not preview['response']['data']['accepted']:
                            action = dict(kind='combat', character=state['active_character'], action='defend')
                        r = await call('session_act', **tokens, action=action)
                    elif state['phase'] in ('exploration', 'approach') and not state['moving']:
                        r = await call('session_act', **tokens, action=dict(kind='move', direction='forward'))
                    else:
                        r = await call('session_advance', **tokens, ticks=1000, until_decision=True)
                assert state['terminal'] and state['phase'] == 'victory'
            closed = await call('session_close', handle=handle)
            trace = closed['trace']
            inspected = await call('replay_inspect', trace=trace, limit=3)
            assert inspected['footer_present'] and inspected['verification'] == 'not_checked'
            async def finish(job):
                for _ in range(1000):
                    status = await call('job_status', job_id=job['job_id'])
                    if status['state'] not in ('running','cancelling'):
                        # Results may be finishing their final atomic publication.
                        await anyio.sleep(.1)
                        return status
                    await anyio.sleep(.02)
                raise AssertionError('Job did not finish')
            verified = await finish(await call('replay_verify', trace=trace))
            assert verified['state'] == 'completed', verified
            damaged = root/'truncated.jsonl'
            original = Path(trace).read_text()
            damaged.write_text('\n'.join(original.splitlines()[:-1])+'\n')
            failure = await finish(await call('replay_verify', trace=str(damaged)))
            assert failure['reason'] == 'replay_incomplete', failure
            changed = root/'changed.jsonl'
            records = original.splitlines()
            header = json.loads(records[0]); header['data']['binary'] = 'wrong'
            records[0] = json.dumps(header)
            changed.write_text('\n'.join(records)+'\n')
            failure = await finish(await call('replay_verify', trace=str(changed)))
            assert failure['reason'] == 'replay_incompatible', failure
            if not cap['engine']['capture']:
                capture = await finish(await call('replay_capture', trace=trace, operations=[1]))
                assert capture['reason'] == 'graphics_unavailable', capture
            if playtest:
                manifest = dict(version=1, parties=[dict(id='fixture', scenario='scenario.json')],
                    rosters=[dict(id='one', monsters=scenario['monsters'])],
                    policies=[dict(id='focus', kind='focus', version=1)], seeds=[5], retention='all')
                (root/'matrix.json').write_text(json.dumps(manifest))
                checked = await finish(await call('experiment_validate', manifest_path=str(root/'matrix.json')))
                assert checked['state'] == 'completed', checked
                done = await finish(await call('experiment_start', manifest_path=str(root/'matrix.json')))
                assert done['state'] == 'completed', done
                results = await call('job_results', job_id=done['job_id'])
                run = Path(results['directory'])
                row = json.loads((run/'trials.jsonl').read_text().splitlines()[0])
                assert row['status'] == 'victory'
                trial = await call('trial_inspect', directory=str(run), trial_id=row['id'])
                assert trial['retention']['trace_retained']
                report = await call('artifact_read', path=str(run/'report.md'))
                assert report['text']
                rerun = await finish(await call('trial_rerun', directory=str(run), trial_id=row['id']))
                assert rerun['state'] == 'completed', rerun
                rerun_result = await call('job_results', job_id=rerun['job_id'])
                assert rerun_result['rerun']['matched']
                # Cancellation is an explicit separate experiment, not a routine contract.
                manifest['seeds'] = list(range(1000))
                (root/'matrix.json').write_text(json.dumps(manifest))
                pending = await call('experiment_start', manifest_path=str(root/'matrix.json'))
                await call('job_cancel', job_id=pending['job_id'])
                stopped = await finish(pending)
                assert stopped['state'] == 'cancelled', stopped
                if evidence:
                    import shutil
                    shutil.copytree(root/'output', evidence)
            # Leave a session open; server lifespan must close it on client disconnect.
            await call('session_start', scenario=scenario)
        await anyio.sleep(.1)
        assert all(json.loads(p.read_text().splitlines()[-1]) == {'type':'complete','data':{'complete':True}} for p in (root/'output').glob('session-*/trace.jsonl'))
        # Native protocol clients use the same tool surface; startup creates no sessions/jobs.
        async with MCPClient(params, read_timeout_seconds=45) as mcp:
            result = await mcp.call_tool('session_list')
            assert result.structured_content == {'sessions': []}
        print('MCP stdio: discovery, schemas, forwarding, stale/reset, replay and lifecycle passed' +
              ('; explicit fixture encounter/experiment/rerun/cancellation passed' if playtest else ''))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('runner', type=Path)
    parser.add_argument('--playtest', action='store_true')
    parser.add_argument('--evidence', type=Path)
    args = parser.parse_args()
    anyio.run(main, args.runner, args.playtest, args.evidence)

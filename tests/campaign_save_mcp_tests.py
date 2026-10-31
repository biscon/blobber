"""Explicit optional-SDK campaign persistence contracts; no policies or experiments."""
from pathlib import Path
import sys
import subprocess
import tempfile
import anyio
from mcp import Client as MCPClient
from mcp.client.stdio import StdioServerParameters
from campaign_save_cli_tests import setup
ROOT=Path(__file__).resolve().parents[1]

async def main(runner, world_writer=None):
    with tempfile.TemporaryDirectory(prefix='blobber-pc06-mcp-') as temporary:
        root=Path(temporary);assets=root/'assets';assets.mkdir();scenario=setup(assets)
        if world_writer:
            subprocess.run([str(world_writer.resolve()), str(assets)], check=True)
        backend = dict(mode='visual') if world_writer else {}
        params=StdioServerParameters(command=sys.executable,args=[str(ROOT/'tools/blobber_mcp.py'),
            '--workspace',str(ROOT),'--runner',str(runner.resolve()),'--assets',str(assets),'--output-root',str(root/'output')])
        async with MCPClient(params,read_timeout_seconds=45) as client:
            async def call(name,**kwargs):
                result=await client.call_tool(name,kwargs)
                assert not result.is_error,(name,result)
                return result.structured_content
            names={t.name for t in (await client.list_tools()).tools}
            assert {'session_save_status','session_preview_save','session_save','session_load','session_retry_terminal'}<=names
            assert (await call('capabilities'))['save_resume']
            started=await call('session_start',scenario=scenario,**backend)
            tokens=dict(handle=started['handle'],**started['identity'])
            assert (await call('session_preview_save',**tokens))['response']['data']['eligible']
            status=await call('session_save_status',handle=started['handle'])
            saved=await call('session_save',**tokens,slot_revision=status['response']['data']['slot_revision'])
            assert saved['response']['data']['durable'],saved
            artifact=saved['save_artifact'];assert Path(artifact).is_file()
            resumed=await call('session_load',handle=saved['handle'],**saved['identity'],replace=True)
            assert resumed['response']['ok'] and resumed['handle']!=saved['handle'],resumed
            stale=await client.call_tool('session_observe',{'handle':saved['handle']});assert stale.is_error
            closed=await call('session_close',handle=resumed['handle']);assert closed['trace_complete']
            fresh=await call('session_start',campaign_save_path=artifact,**backend)
            assert fresh['response']['ok'] and fresh['response']['data']['campaign']['location']==0,fresh
            await call('session_close',handle=fresh['handle'])
            job=await call('replay_verify',trace=closed['trace'])
            for _ in range(100):
                status=await call('job_status',job_id=job['job_id'])
                if status['state'] not in ('running','cancelling'):break
                await anyio.sleep(.05)
            assert status['state']=='completed' and status['exit_code']==0,status
    print('Campaign save SDK stdio: schemas, safe preview, save artifact, resumed handles, fresh session and strict replay passed')

if __name__=='__main__':anyio.run(main,Path(sys.argv[1]),Path(sys.argv[2]) if len(sys.argv)>2 else None)

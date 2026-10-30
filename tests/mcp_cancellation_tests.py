"""Optional SDK/process cancellation checks with an owned fake runner; no gameplay."""
import json
import os
from pathlib import Path
import signal
import sys
import tempfile

import anyio
from mcp import Client
from mcp.client.stdio import StdioServerParameters

ROOT = Path(__file__).resolve().parents[1]


def gone(pid):
    try:
        os.kill(pid, 0)
        return False
    except ProcessLookupError:
        return True


async def main():
    with tempfile.TemporaryDirectory(prefix='blobber-mcp-cancel-') as tmp:
        root = Path(tmp)
        runner = root/'runner'
        runner.write_text('''#!/usr/bin/env python3
import json,os,sys,time
from pathlib import Path
root=Path(__file__).parent
(root/'runner.pid').write_text(str(os.getpid()))
(root/'server.pid').write_text(str(os.getppid()))
for line in sys.stdin:
 r=json.loads(line)
 if r['op']=='act':
  (root/'action.received').write_text('yes')
  time.sleep(30)
 data={'session':'1','revision':'1'} if r['op']=='start' else {'closed':True}
 print(json.dumps(dict(version=1,type='response',id=r['id'],ok=True,data=data)),flush=True)
 if r['op']=='close':break
''')
        runner.chmod(0o700)
        params = StdioServerParameters(command=sys.executable, args=[str(ROOT/'tools/blobber_mcp.py'),
            '--runner',str(runner),'--assets',str(root),'--output-root',str(root/'output')])
        async with Client(params, mode='legacy') as mcp:
            async def start():
                result=await mcp.call_tool('session_start', {'scenario':{}})
                assert not result.is_error, result
                return result.structured_content
            r=await start()
            pid=int((root/'runner.pid').read_text())
            with anyio.move_on_after(.3) as scope:
                await mcp.call_tool('session_act', dict(handle=r['handle'],**r['identity'],action={}))
            assert scope.cancel_called and (root/'action.received').exists()
            for _ in range(100):
                result=await mcp.call_tool('session_list')
                if not result.structured_content['sessions'] and gone(pid): break
                await anyio.sleep(.03)
            assert not result.structured_content['sessions'] and gone(pid), result
            # EOF cleanup independently tested by the ordinary stdio suite. Exercise SIGTERM here.
            await start()
            pid=int((root/'runner.pid').read_text())
            server=int((root/'server.pid').read_text())
            os.kill(server, signal.SIGTERM)
            for _ in range(100):
                if gone(pid):break
                await anyio.sleep(.03)
            assert gone(pid), 'Server SIGTERM left its child alive'
        print('MCP cancellation: interrupted mutation invalidated/reaped; SIGTERM cleaned up child')


if __name__=='__main__': anyio.run(main)

"""Owned save/transport/replay lifecycle contracts; no gameplay policy or balance run."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from headless_cli_tests import fixture
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from headless_client import Client
from mcp_adapter.sessions import Sessions
from mcp_adapter.storage import Storage, MIB
from mcp_adapter.investigation import replay_inspect


def setup(root):
    arena=fixture(root)
    path=root/'rules/catalog.json'
    cat=json.loads(path.read_text())
    enemy=cat['monsters'][0]
    for name in ('goblin','goblin.archer','goblin.guardian.placeholder'):
        cat['monsters'].append(dict(enemy,id=name))
    for name in ('healing_potion','mana_potion'):
        cat['items'].append(dict(cat['items'][1],id=name))
    path.write_text(json.dumps(cat))
    npc=json.loads((root/'npcs/enemy.json').read_text())
    for name in ('goblin','goblin.archer'):
        (root/f'npcs/{name}.json').write_text(json.dumps(dict(npc,id=name)))
    (root/'npcs/level.json').write_text(json.dumps(dict(definitions=['enemy.json','goblin.json','goblin.archer.json'],instances=[])))
    return dict(kind='campaign',schema_version=1,generator_version=3,template_version=3,seed=5,party=arena['party'])


def main(runner):
    with tempfile.TemporaryDirectory(prefix='blobber-pc06-cli-') as temp:
        root=Path(temp);scenario=setup(root);slot=root/'campaign.json';trace=root/'save.jsonl'
        with Client(runner,root,record=trace,campaign_save=slot) as client:
            state=client.request('start',scenario=scenario)
            assert state['ok'],state
            status=client.request('save_status')['data'];assert not status['exists']
            assert client.request('preview_save')['data']['eligible']
            result=client.request('save',slot_revision=status['slot_revision'])
            assert result['ok'] and result['data']['durable'],result
            hub=client.request('observe')['data']
            stair=hub['fixtures'][0]['fixture']
            result=client.request('act',action=dict(kind='interact',fixture=stair))
            assert result['data']['accepted'],result
            assert client.request('act',action=dict(kind='recall'))['data']['accepted']
            status=client.request('save_status')['data']
            saved=client.request('save',slot_revision=status['slot_revision'])
            assert saved['ok'] and saved['data']['durable'],saved
            before=client.identity.copy()
            assert not client.request('load')['ok']
            assert client.identity==before
            loaded=client.request('load',replace=True)
            assert loaded['ok'] and client.identity['session']!=before['session'],loaded
            assert loaded['data']['campaign']['location']==0
            assert loaded['data']['campaign']['visited_entries']==[1]
            assert not client.request('reset')['ok']
            good=slot.read_bytes();before=client.identity.copy()
            slot.write_bytes(b'\xff{corrupt save')
            rejected=client.request('load',replace=True)
            assert not rejected['ok'] and client.identity==before,rejected
            assert client.request('observe')['data']['campaign']['visited_entries']==[1]
            slot.write_bytes(good)
        saved_bytes=slot.read_bytes()
        slot.unlink()
        replay=subprocess.run([runner,'--assets',str(root),'--campaign-save',str(slot),'--replay',str(trace)],capture_output=True,text=True,timeout=30)
        assert replay.returncode==0,(replay.stdout,replay.stderr)
        assert not slot.exists(),'replay must never write configured disk slot'
        # Separate real process, with no character library or originating session.
        slot.write_bytes(saved_bytes)
        with Client(runner,root,campaign_save=slot) as client:
            loaded=client.request('load');assert loaded['ok'],loaded
            assert loaded['data']['campaign']['visited_entries']==[1]
        storage=Storage(root/'mcp',1024*MIB)
        sessions=Sessions(runner,root,storage)
        try:
            started=sessions.start(campaign_save_path=str(slot))
            assert started['response']['ok'],started
            token=dict(handle=started['handle'],**started['identity'])
            status=sessions.request(op='save_status',handle=started['handle'])['response']['data']
            saved=sessions.request(op='save',slot_revision=status['slot_revision'],**token)
            assert saved['response']['ok'] and Path(saved['save_artifact']).is_file(),saved
            token=dict(handle=saved['handle'],**saved['identity'])
            loaded=sessions.request(op='load',replace=True,**token)
            assert loaded['response']['ok'] and loaded['handle']!=saved['handle'],loaded
            evidence=replay_inspect(Path(loaded['trace']),limit=128)
            assert 'parse_error' not in evidence,evidence
        finally:sessions.shutdown()
    print('campaign save CLI/client/local MCP/replay contracts passed')

if __name__=='__main__':main(sys.argv[1])

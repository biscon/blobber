"""Fast owned adapter contracts. No game content, policies or balance playthroughs."""
import json
import os
from pathlib import Path
import sys
import tempfile
import time
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from mcp_adapter.storage import Storage, AdapterError, MIB
from mcp_adapter.sessions import Sessions, EventRing
from mcp_adapter.jobs import Jobs, probe_capabilities
from mcp_adapter.investigation import replay_inspect


class FakeClient:
    def __init__(self, executable, assets, mode, record, on_event, **kwargs):
        self.visibility = kwargs.get('visibility')
        self.identity = None
        self.closed = self.failed = False
        self.disposed = False
        self.calls = []
        self.epoch = 0
        self.cancelled = kwargs['cancelled']
        self.on_event = on_event
        self.process = type('Process', (), {'returncode': 0})()

    def request(self, op, **fields):
        self.calls.append((op, fields))
        if op in ('start', 'reset'):
            if fields.get('scenario') == {'reject': True}:
                return dict(ok=False, error={'code': 'invalid_request'})
            self.epoch += 1
            self.identity = dict(session=str(self.epoch), revision='1')
        elif op in ('act', 'advance'):
            self.identity['revision'] = str(int(self.identity['revision'])+1)
            self.on_event(dict(events=[dict(event='fixture')]))
        if op == 'close':
            self.closed = True
        return dict(ok=True, data=dict(self.identity or {}, accepted=fields.get('action') != {'reject': True}))

    def dispose(self):
        self.disposed = True


class Fixture(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.storage = Storage(self.root/'output', 1024*MIB)
        self.sessions = Sessions('runner', '.', self.storage, client_factory=FakeClient)
        self.addCleanup(self.sessions.shutdown)

    def start(self):
        return self.sessions.start(scenario={})

    def tokens(self, r):
        return dict(handle=r['handle'], **r['identity'])

    def error(self, code, operation):
        with self.assertRaises(AdapterError) as cm:
            operation()
        self.assertEqual(cm.exception.code, code)
        return cm.exception

    def test_world_visibility_is_retained_and_replacement_cannot_change_it(self):
        r = self.sessions.start(scenario={}, mode='visual', visibility='world')
        self.assertEqual(self.sessions.live[r['handle']].client.visibility, 'world')
        self.error('invalid_input', lambda: self.sessions.start(scenario={}, mode='visual', visibility='legacy', **self.tokens(r)))
        changed = self.sessions.request(op='reset', **self.tokens(r))
        self.assertEqual(self.sessions.live[changed['handle']].client.visibility, 'world')
        self.error('invalid_input', lambda: self.sessions.start(scenario={}, mode='diagnostic', visibility='world'))

    def test_stale_revision_not_forwarded(self):
        r = self.start()
        c = self.sessions.live[r['handle']].client
        changed = self.sessions.request(op='act', action={}, **self.tokens(r))
        calls = len(c.calls)
        self.error('stale_revision', lambda: self.sessions.request(op='act', action={}, **self.tokens(r)))
        self.assertEqual(len(c.calls), calls)
        self.assertNotEqual(r['identity'], changed['identity'])

    def test_failed_replacement_preserves_handle_then_reset_rotates(self):
        r = self.start()
        failed = self.sessions.start(scenario={'reject': True}, **self.tokens(r))
        self.assertFalse(failed['response']['ok'])
        self.assertEqual(failed['handle'], r['handle'])
        new = self.sessions.request(op='reset', **self.tokens(r))
        self.assertNotEqual(new['handle'], r['handle'])
        self.error('stale_session', lambda: self.sessions.request(r['handle'], 'observe'))
        self.assertTrue(self.sessions.request(new['handle'], 'observe')['response']['ok'])

    def test_failed_new_start_releases_capacity(self):
        r = self.sessions.start(scenario={'reject': True})
        self.assertIsNone(r['handle'])
        self.assertFalse(self.sessions.live)
        self.assertFalse(self.storage.reservations)

    def test_capacity_busy_and_close_reap(self):
        a, b = self.start(), self.start()
        self.error('session_capacity', self.start)
        s = self.sessions.live[a['handle']]
        with s.lock:
            self.error('session_busy', lambda: self.sessions.request(a['handle'], 'observe'))
        self.sessions.close(a['handle'])
        self.assertTrue(s.client.disposed)
        self.assertEqual(len(self.sessions.list()), 1)

    def test_rejected_action_keeps_revision_and_event_order(self):
        r = self.start()
        new = self.sessions.request(op='act', action={'reject': True}, **self.tokens(r))
        self.assertFalse(new['response']['data']['accepted'])
        self.assertEqual(new['identity']['revision'], '2')
        self.assertEqual(self.sessions.events(r['handle'])['events'][0]['event']['event'], 'fixture')

    def test_expiry_and_shutdown(self):
        r = self.start()
        s = self.sessions.live[r['handle']]
        s.last_used -= 1801
        self.sessions.sweep()
        self.assertTrue(s.client.disposed)
        self.assertFalse(self.sessions.list())
        self.sessions.shutdown()
        self.error('shutdown', self.start)

    def test_event_gap_and_bounded_bytes(self):
        ring = EventRing(count=2, byte_limit=100)
        ring.append({'events': [dict(event=str(n)) for n in range(5)]})
        e = self.error('event_gap', lambda: ring.page(0, 2))
        self.assertEqual(e.details['earliest_cursor'], 3)
        self.assertEqual([x['cursor'] for x in ring.page(3, 1)['events']], [3])
        ring.append({'events': [dict(event='x'*200)]})
        self.assertFalse(ring.items)
        self.assertEqual(ring.page(6, 1)['next_cursor'], 6)

    def test_limits_reject_before_dispatch(self):
        r = self.start()
        for ticks in (-1, 1001, True):
            self.error('invalid_input', lambda: self.sessions.request(op='advance', ticks=ticks, **self.tokens(r)))
        self.error('invalid_input', lambda: self.sessions.start(scenario={}, scenario_path='file'))

    def test_output_reservations_account_actual_once(self):
        storage = Storage(self.root/'bounded', 100)
        a = storage.reserve('a', 60)
        (a/'data').write_bytes(b'x'*30)
        self.error('output_capacity', lambda: storage.reserve('b', 41))
        b = storage.reserve('b', 40)
        storage.release(a)
        self.error('output_capacity', lambda: storage.reserve('c', 31))
        self.assertTrue(b.is_dir())

    def test_artifact_pages_and_escape(self):
        root = self.storage.reserve('test', MIB)
        f = root/'report.md'; f.write_text('abcdef')
        self.assertEqual(self.storage.read(f, 2, 3)['text'], 'cde')
        self.assertFalse(self.storage.read(f, 2, 3)['eof'])
        external = self.root/'private'; external.write_text('private')
        (root/'escape').symlink_to(external)
        self.error('artifact_path', lambda: self.storage.read(root/'escape'))
        self.error('artifact_path', lambda: self.storage.read(root/'..'/'test'))
        fifo = root/'fifo'; os.mkfifo(fifo)
        self.error('artifact_path', lambda: self.storage.read(fifo))

    def test_replay_pages_diagnostic_not_verified(self):
        trace = self.root/'trace'
        records = [dict(type='header', data={'mode': 'visual'}), dict(type='request', data={'op': 'start'}),
                   dict(type='response', data={}), dict(type='state', data={}),
                   dict(type='request', data={'op': 'reset'}), dict(type='response', data={}),
                   dict(type='state', data={}), dict(type='complete', data={'complete': True})]
        trace.write_text(''.join(json.dumps(x)+'\n' for x in records))
        r = replay_inspect(trace, limit=2, operation=2)
        self.assertTrue(r['more']); self.assertTrue(r['footer_present'])
        self.assertEqual(r['verification'], 'not_checked')
        r2 = replay_inspect(trace, cursor=r['next_cursor'], operation=2)
        self.assertEqual(r2['records'][0]['type'], 'state')
        trace.write_text(trace.read_text()+'{}\n')
        self.assertFalse(replay_inspect(trace)['footer_present'])


class ProcessContracts(unittest.TestCase):
    error = Fixture.error

    def setUp(self):
        Fixture.setUp(self)
        self.jobs = Jobs('runner', '.', self.storage, grace=.15, kill_grace=.1)
        self.addCleanup(self.jobs.shutdown)

    def launch(self, script):
        return self.jobs.launch('fixture', 16*MIB, 3,
                                lambda root: [sys.executable, '-c', script])

    def wait(self, job_id):
        j = self.jobs.jobs[job_id]
        j.thread.join(timeout=5)
        self.assertFalse(j.thread.is_alive())
        return self.jobs.status(job_id)

    def test_job_status_cancel_and_capacity(self):
        r = self.launch('import time; time.sleep(20)')
        self.error('job_capacity', lambda: self.launch('pass'))
        self.jobs.cancel(r['job_id']); self.jobs.cancel(r['job_id'])
        done = self.wait(r['job_id'])
        self.assertEqual(done['state'], 'cancelled')
        self.assertIsNotNone(self.jobs.jobs[r['job_id']].process.poll())
        self.jobs.forget(r['job_id'])
        self.assertTrue(Path(done['output']).exists())

    def test_uncooperative_child_and_descendant_killed(self):
        script = "import signal,subprocess,sys,time; signal.signal(signal.SIGTERM,signal.SIG_IGN); p=subprocess.Popen([sys.executable,'-c','import time; time.sleep(20)']); print(p.pid,flush=True); time.sleep(20)"
        r = self.launch(script)
        # Wait for the owned fixture to install its signal handler, not a timing guess.
        deadline = time.monotonic()+2
        while not self.jobs.jobs[r['job_id']].stdout and time.monotonic()<deadline:
            time.sleep(.01)
        self.jobs.cancel(r['job_id'])
        done = self.wait(r['job_id'])
        self.assertTrue(done['forced_termination'])
        self.assertEqual(done['state'], 'cancelled')

    def test_stderr_drained_and_stdout_limit(self):
        r = self.launch("import os; os.write(2,b'x'*(2*1024*1024)); print('ok')")
        done = self.wait(r['job_id'])
        self.assertEqual(done['state'], 'completed')
        self.assertTrue(done['stderr_truncated'])
        self.assertEqual((Path(done['output'])/'stderr.txt').stat().st_size, MIB)
        r = self.launch("import os,time; os.write(1,b'x'*(9*1024*1024)); time.sleep(20)")
        done = self.wait(r['job_id'])
        self.assertEqual(done['reason'], 'output_limit')
        self.assertLessEqual((Path(done['output'])/'stdout.txt').stat().st_size, 8*MIB)

    def test_capability_probe_output_bound(self):
        script = self.root/'probe'
        script.write_text('#!/usr/bin/env python3\nimport os\nos.write(1,b"x"*100000)\n')
        script.chmod(0o700)
        self.error('runner_failure', lambda: probe_capabilities(script))

    def test_job_wall_deadline(self):
        r = self.jobs.launch('fixture', 16*MIB, .05,
                             lambda root: [sys.executable, '-c', 'import time; time.sleep(20)'])
        done = self.wait(r['job_id'])
        self.assertEqual(done['state'], 'timed_out')
        self.assertEqual(done['reason'], 'wall_limit')
        self.assertTrue((Path(done['output'])/'stdout.txt').is_file())

    def test_nonzero_exit_is_not_combat_loss(self):
        r = self.launch('raise RuntimeError("owned failure")')
        done = self.wait(r['job_id'])
        self.assertEqual(done['state'], 'failed')
        self.assertNotIn('combat_loss', self.jobs.results(r['job_id']))

    def test_retained_running_metadata_is_interrupted(self):
        p = self.root/'old'; p.mkdir()
        (p/'job.json').write_text('{"state":"running"}')
        self.assertEqual(self.jobs.results(directory=p)['job']['state'], 'interrupted')

    def test_shutdown_reaps_all_children(self):
        r = self.launch('import time; time.sleep(20)')
        self.jobs.shutdown()
        self.assertIsNotNone(self.jobs.jobs[r['job_id']].process.poll())
        self.assertFalse(self.storage.reservations)


if __name__ == '__main__':
    unittest.main()

"""One bounded subprocess job at a time; existing experiment CLI owns its rules/reports."""
import copy
import json
import os
from pathlib import Path
import selectors
import signal
import subprocess
import sys
import threading
import time
import uuid

from arena_experiments import config
from arena_experiments.report import atomic_json, rows
from .storage import AdapterError, MIB, artifact_list, input_file, json_file

TOOLS = Path(__file__).resolve().parents[1]
ACTIVE = ('running', 'cancelling')


class Job:
    def __init__(self, kind, root, command, seconds):
        self.id = uuid.uuid4().hex
        self.kind, self.root, self.command, self.seconds = kind, root, command, seconds
        self.state = 'running'
        self.started = time.monotonic()
        self.cancel = threading.Event()
        self.process = None
        self.thread = None
        self.exit_code = None
        self.reason = ''
        self.forced = False
        self.stdout = bytearray()
        self.stderr = bytearray()
        self.stderr_truncated = False
        self.finished = None
        self.published = False


class Jobs:
    def __init__(self, runner, assets, storage, *, grace=5, kill_grace=1):
        self.runner, self.assets, self.storage = Path(runner), Path(assets), storage
        self.lock = threading.RLock()
        self.jobs = {}
        self.closed = False
        self.grace, self.kill_grace = grace, kill_grace

    def launch(self, kind, maximum, seconds, prepare):
        with self.lock:
            if self.closed:
                raise AdapterError('shutdown', 'Server is closing')
            if any(j.state in ACTIVE for j in self.jobs.values()):
                raise AdapterError('job_capacity', 'One active background job; no queue')
            if len(self.jobs) >= 64:
                raise AdapterError('job_capacity', 'Forget a terminal job before launching another (64 records)')
            root = self.storage.reserve(kind, maximum)
            try:
                command = prepare(root)
                job = Job(kind, root, command, seconds)
                job.process = subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                               stderr=subprocess.PIPE, start_new_session=True, bufsize=0)
                self.jobs[job.id] = job
                self._persist(job)
                job.thread = threading.Thread(target=self._run, args=(job,), daemon=True, name='blobber-job')
                job.thread.start()
            except BaseException:
                if 'job' in locals() and job.process:
                    self._signal_group(job, signal.SIGKILL)
                    job.process.wait()
                    job.process.stdout.close(); job.process.stderr.close()
                    self.jobs.pop(job.id, None)
                self.storage.release(root)
                raise
            return self.status(job.id)

    def _persist(self, job):
        atomic_json(job.root/'job.json', self._status(job))

    def _status(self, j):
        return dict(job_id=j.id, kind=j.kind, state=j.state if j.published or j.state in ACTIVE else 'running', reason=j.reason,
                    elapsed_seconds=(j.finished or time.monotonic())-j.started,
                    output=str(j.root), run_directory=str(j.root/'run'), exit_code=j.exit_code,
                    forced_termination=j.forced, stderr_truncated=j.stderr_truncated)

    @staticmethod
    def _signal_group(j, sig):
        try:
            os.killpg(j.process.pid, sig)
        except ProcessLookupError:
            pass

    def _run(self, j):
        selector = selectors.DefaultSelector()
        stop_time = None
        escalated = False
        exited = None
        try:
            for stream, kind in ((j.process.stdout, 'stdout'), (j.process.stderr, 'stderr')):
                os.set_blocking(stream.fileno(), False)
                selector.register(stream, selectors.EVENT_READ, kind)
            while selector.get_map() or j.process.poll() is None:
                now = time.monotonic()
                if stop_time is None and (j.cancel.is_set() or now-j.started >= j.seconds):
                    stop_time = now
                    with self.lock:
                        j.state = 'cancelling'
                        j.reason = j.reason or ('cancel_requested' if j.cancel.is_set() else 'wall_limit')
                    if j.process.poll() is None:
                        j.process.terminate()  # CLI handles this cooperatively and reaps its runner.
                if stop_time is not None and now-stop_time >= self.grace and not escalated:
                    self._signal_group(j, signal.SIGTERM)
                    j.forced = escalated = True
                if stop_time is not None and now-stop_time >= self.grace+self.kill_grace:
                    self._signal_group(j, signal.SIGKILL)
                    j.forced = True
                if j.process.poll() is not None:
                    exited = exited or now
                    if now-exited > 1:
                        self._signal_group(j, signal.SIGKILL)  # Descendant retained a pipe after its owner exited.
                        if now-exited > 2:
                            break
                for key, _ in selector.select(.05):
                    data = os.read(key.fd, 65536)
                    if not data:
                        selector.unregister(key.fileobj)
                        continue
                    if key.data == 'stdout':
                        remaining = 8*MIB-len(j.stdout)
                        j.stdout.extend(data[:remaining])
                        if len(data) > remaining:
                            j.reason = 'output_limit'
                            j.cancel.set()
                    else:
                        j.stderr.extend(data)
                        if len(j.stderr) > MIB:
                            j.stderr_truncated = True
                            del j.stderr[:-MIB]
            j.exit_code = j.process.wait(timeout=2)
            # Ensure no owned descendants survive even if the CLI exited unexpectedly.
            self._signal_group(j, signal.SIGKILL)
            with self.lock:
                j.state = ('timed_out' if j.reason == 'wall_limit' else
                           'failed' if j.reason == 'output_limit' else
                           'completed' if j.exit_code == 0 else
                           'cancelled' if j.cancel.is_set() else 'failed')
                j.reason = j.reason or ('process_complete' if j.exit_code == 0 else 'process_failed')
                if b'graphics_unavailable' in j.stderr or b'graphics_unavailable' in j.stdout:
                    j.reason = 'graphics_unavailable'
                elif j.kind in ('replay', 'capture') and j.exit_code and j.state == 'failed':
                    diagnostic = bytes(j.stderr)+bytes(j.stdout)
                    j.reason = ('replay_incompatible' if b'mismatch at operation 0 header' in diagnostic else
                                'replay_mismatch' if b'Replay mismatch' in diagnostic else
                                'replay_incomplete' if b'Incomplete recording' in diagnostic else
                                'capture_invalid_point' if b'capture_invalid_point' in diagnostic else
                                'replay_or_capture_failed')
        except Exception as exc:
            self._signal_group(j, signal.SIGKILL)
            j.process.wait(timeout=2)
            j.state, j.reason = 'failed', 'job_monitor: '+str(exc)
        finally:
            selector.close()
            j.process.stdout.close(); j.process.stderr.close()
            with self.lock:
                j.finished = time.monotonic()
                try:
                    (j.root/'stdout.txt').write_bytes(j.stdout)
                    (j.root/'stderr.txt').write_bytes(j.stderr)
                    j.published = True
                    self._persist(j)
                except OSError as exc:
                    j.published = True
                    j.state, j.reason = 'failed', 'artifact_failure: '+str(exc)
                self.storage.release(j.root)

    def _get(self, job_id):
        j = self.jobs.get(job_id)
        if not j:
            raise AdapterError('unknown_job', 'Unknown or forgotten job; retained output can be opened by path')
        return j

    def status(self, job_id):
        with self.lock:
            j = self._get(job_id)
            result = self._status(j)
        batch = j.root/'run'/'batch.json'
        if batch.exists():
            try:
                result['batch'] = json_file(batch.parent, batch.name)
                b = result['batch']
                result['progress'] = dict(planned=b['planned'], started=b['started'],
                    recorded_trials=max(0, b['started']-int('active_trial' in b)),
                    unstarted=b['planned']-b['started'], active_trial=b.get('active_trial'))
            except (OSError, ValueError):
                result['batch_read'] = 'not_yet_available'
        if j.state not in ACTIVE:
            result['stderr_tail'] = bytes(j.stderr[-8192:]).decode('utf-8', errors='replace')
        return result

    def list(self):
        with self.lock:
            return [self._status(j) for j in self.jobs.values()]

    def cancel(self, job_id):
        with self.lock:
            j = self._get(job_id)
            if j.state in ACTIVE and j.process.poll() is None:
                j.cancel.set()
        return self.status(job_id)

    def forget(self, job_id):
        with self.lock:
            j = self._get(job_id)
            if j.state in ACTIVE or j.thread.is_alive():
                raise AdapterError('job_busy', 'Only terminal jobs can be forgotten')
            del self.jobs[job_id]
        return dict(forgotten=job_id, artifacts_preserved=str(j.root))

    def results(self, job_id=None, directory=None):
        if (job_id is None) == (directory is None):
            raise AdapterError('invalid_input', 'Choose exactly one job_id or directory')
        if job_id:
            with self.lock:
                j = self._get(job_id)
                root, status = j.root, self._status(j)
        else:
            root = self.storage.register(directory)
            status = dict(state='retained_artifacts', execution_resumed=False)
            if (root/'job.json').exists():
                status['recorded_job'] = json_file(root, 'job.json')
                if status['recorded_job'].get('state') in ACTIVE:
                    status['state'] = 'interrupted'
        run = root/'run' if (root/'run').is_dir() else root
        if not run.resolve().is_relative_to(root.resolve()):
            raise AdapterError('artifact_path', 'Run directory escapes registered root')
        result = dict(job=status, directory=str(run), artifacts=artifact_list(root), run_artifacts=artifact_list(run))
        for name, maximum in (('batch.json', MIB), ('summary.json', 16*MIB), ('rerun.json', MIB)):
            if (run/name).exists():
                value = json_file(run, name, maximum)
                # Summaries can span many groups; return a bounded overview plus the original artifact.
                if name == 'summary.json':
                    value = {k:v for k,v in value.items() if k in ('planned','started','unstarted','counts','batch','artifact_failures')}
                result[name.removesuffix('.json')] = value
        if (root/'captures'/'capture.json').is_file():
            if not (root/'captures').resolve().is_relative_to(root.resolve()):
                raise AdapterError('artifact_path', 'Capture directory escapes root')
            result['capture'] = json_file(root/'captures', 'capture.json', MIB)
            result['capture_artifacts'] = artifact_list(root/'captures')
        if (root/'stdout.txt').is_file() and (root/'stdout.txt').stat().st_size <= 8*MIB:
            try:
                output = json_file(root, 'stdout.txt', 8*MIB)
                if isinstance(output, dict):
                    if 'checks' in output:
                        result['validation'] = dict(ok=output.get('ok'), checks=len(output['checks']),
                                                    detail_artifact=str(root/'stdout.txt'))
                    else:
                        result['process_result'] = output
            except ValueError:
                pass  # Existing experiment CLI also emits progress lines; reports are authoritative.
        if result.get('batch', {}).get('status') == 'running' and status['state'] not in ACTIVE:
            result['execution_note'] = 'No live owner; recorded running state is interrupted, not resumable'
        return result

    def experiment(self, manifest_path, validate=False, structure_only=False):
        manifest_path = input_file(manifest_path, MIB)
        resolved = config.load(manifest_path)
        maximum = (resolved['limits']['output_bytes'] if not validate else 0)+48*MIB
        def prepare(root):
            snapshot = copy.deepcopy(resolved)
            for i, party in enumerate(snapshot['parties']):
                name = f'party-{i}.json'
                atomic_json(root/name, party['scenario'])
                party['scenario'] = name
            atomic_json(root/'input.json', snapshot)
            command = [sys.executable, str(TOOLS/'arena_experiment.py'), 'validate' if validate else 'run',
                       str(root/'input.json'), '--runner', str(self.runner), '--assets', str(self.assets)]
            if validate:
                if structure_only:
                    command += ['--structure-only']
            else:
                command += ['--output', str(root/'run')]
            return command
        return self.launch('validate' if validate else 'experiment', maximum,
                           resolved['limits']['batch_seconds'], prepare)

    def rerun(self, directory, trial_id):
        root = self.storage.register(directory)
        cfg = json_file(root, 'manifest.json', 32*MIB)
        # Validate retained inputs and bounds before allocating a job; engine/code checks remain in rerun.
        resolved = config.resolve(cfg, root, embedded=True)
        json_file(root, 'provenance.json', 4*MIB)
        if not (root/'trials.jsonl').resolve().is_relative_to(root):
            raise AdapterError('artifact_path', 'Trial journal escapes run root')
        return self.launch('rerun', resolved['limits']['output_bytes']+16*MIB,
                           resolved['limits']['batch_seconds'], lambda output: [
                               sys.executable, str(TOOLS/'arena_experiment.py'), 'rerun', str(root),
                               '--trial', trial_id, '--runner', str(self.runner), '--assets', str(self.assets),
                               '--output', str(output/'run')])

    def replay(self, trace, mode=None, operations=None):
        trace = input_file(trace, 256*MIB)
        with trace.open('rb') as f:
            line = f.readline(2*MIB+4097)
        if len(line) > 2*MIB+4096:
            raise AdapterError('invalid_input', 'Oversized trace header')
        header = json.loads(line)
        if not isinstance(header, dict) or header.get('type') != 'header' or not isinstance(header.get('data'), dict):
            raise AdapterError('invalid_input', 'Missing trace header')
        recorded_mode = header['data']['mode']
        if mode is not None and mode != recorded_mode:
            raise AdapterError('replay_incompatible', 'Requested mode differs from recording')
        if recorded_mode not in ('diagnostic', 'visual'):
            raise AdapterError('invalid_input', 'Invalid recorded mode')
        if operations is not None:
            from .storage import integer
            if not 1 <= len(operations) <= 8 or len(set(operations)) != len(operations):
                raise AdapterError('invalid_input', 'Choose 1..8 distinct operation indices')
            for n in operations:
                integer(n, 1, 10000000, 'operation')
        def prepare(root):
            # Freeze the bounded trace so inspection/capture cannot race an external writer.
            total = 0
            with trace.open('rb') as source, (root/'trace.jsonl').open('xb') as target:
                while data := source.read(MIB):
                    total += len(data)
                    if total > 256*MIB:
                        raise AdapterError('invalid_input', 'Trace grew beyond its limit')
                    target.write(data)
            command = [str(self.runner), '--assets', str(self.assets), '--mode', recorded_mode,
                       '--replay', str(root/'trace.jsonl')]
            if header['data'].get('campaign_persistence') == 1:
                command += ['--campaign-save', str(root/'replay-memory-slot.json')]
            if operations is not None:
                command += ['--capture-dir', str(root/'captures')]
                for op in operations:
                    command += ['--capture-operation', str(op)]
            return command
        return self.launch('capture' if operations is not None else 'replay', 336*MIB, 300, prepare)

    def shutdown(self):
        with self.lock:
            self.closed = True
            jobs = list(self.jobs.values())
            for j in jobs:
                if j.state in ACTIVE:
                    j.cancel.set()
        for j in jobs:
            j.thread.join(timeout=self.grace+self.kill_grace+5)
            if j.thread.is_alive():
                self._signal_group(j, signal.SIGKILL)
                j.thread.join(timeout=3)


def probe_capabilities(runner, check_cancel=lambda: False):
    """A short capability query with bounded pipes, independent of the job registry."""
    process = subprocess.Popen([str(runner), '--capabilities'], stdin=subprocess.DEVNULL,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True)
    selector = selectors.DefaultSelector()
    buffers = {'stdout': bytearray(), 'stderr': bytearray()}
    end = time.monotonic()+5
    try:
        for stream, name in ((process.stdout, 'stdout'), (process.stderr, 'stderr')):
            os.set_blocking(stream.fileno(), False)
            selector.register(stream, selectors.EVENT_READ, name)
        while selector.get_map() or process.poll() is None:
            if check_cancel():
                raise AdapterError('cancelled', 'Capability probe cancelled')
            if time.monotonic() >= end:
                raise AdapterError('timeout', 'Capability probe exceeded five seconds')
            for key, _ in selector.select(.05):
                data = os.read(key.fd, 4096)
                if not data:
                    selector.unregister(key.fileobj)
                    continue
                if len(buffers[key.data])+len(data) > 65536:
                    raise AdapterError('runner_failure', 'Capability output exceeds 64 KiB')
                buffers[key.data].extend(data)
        if process.returncode:
            raise AdapterError('runner_failure', buffers['stderr'].decode('utf-8', errors='replace'))
        result = json.loads(buffers['stdout'])
        if not isinstance(result, dict) or result.get('protocol') != 1:
            raise AdapterError('runner_failure', 'Invalid capability response')
        return result
    finally:
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        process.wait(timeout=2)
        selector.close()
        process.stdout.close(); process.stderr.close()

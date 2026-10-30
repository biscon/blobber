"""Official SDK boundary; the stdlib adapters remain independently testable."""
from contextlib import asynccontextmanager
from functools import wraps
import json
import subprocess
import signal
import threading

import anyio
from mcp.server import MCPServer
from mcp.server.mcpserver.tools import Tool
from mcp.types import ToolAnnotations, CallToolResult, TextContent, ImageContent

from .storage import AdapterError, Storage, MIB
from .sessions import Sessions
from .jobs import Jobs, probe_capabilities
from .investigation import replay_inspect as inspect_trace, trial_inspect as inspect_trial


def cancelled():
    try:
        anyio.from_thread.check_cancelled()
        return False
    except RuntimeError:
        return False  # Direct contract calls outside an SDK worker thread.
    except BaseException:
        return True


def create_server(runner, assets, output_root, capacity=8192*MIB):
    storage = Storage(output_root, capacity)
    sessions = Sessions(runner, assets, storage)
    jobs = Jobs(runner, assets, storage)

    @asynccontextmanager
    async def lifespan(_server):
        async def maintenance():
            while True:
                await anyio.sleep(5)
                await anyio.to_thread.run_sync(sessions.sweep)
        async with anyio.create_task_group() as group:
            async def signals():
                with anyio.open_signal_receiver(signal.SIGINT, signal.SIGTERM) as receiver:
                    async for _ in receiver:
                        group.cancel_scope.cancel()
                        return
            group.start_soon(maintenance)
            group.start_soon(signals)
            try:
                yield None
            finally:
                group.cancel_scope.cancel()
                with anyio.CancelScope(shield=True):
                    await anyio.to_thread.run_sync(sessions.shutdown)
                    await anyio.to_thread.run_sync(jobs.shutdown)

    registered = []
    probe_lock = threading.Lock()

    def tool(readonly=False):
        def decorate(fn):
            @wraps(fn)
            def wrapped(*args, **kwargs):
                try:
                    if cancelled():
                        raise AdapterError('cancelled', 'Request cancelled before dispatch')
                    value = fn(*args, **kwargs)
                    if isinstance(value, CallToolResult):
                        return value
                    failed = isinstance(value, dict) and value.get('response', {}).get('ok') is False
                    return CallToolResult(is_error=failed,
                                          content=[TextContent(type='text', text=json.dumps(value, allow_nan=False))],
                                          structured_content=value)
                except AdapterError as exc:
                    return CallToolResult(is_error=True, content=[TextContent(type='text', text=json.dumps(exc.result()))],
                                          structured_content={'error': exc.result()})
                except (OSError, ValueError, KeyError, TypeError, subprocess.SubprocessError) as exc:
                    error = dict(code='invalid_input' if isinstance(exc, (ValueError, KeyError, TypeError)) else 'io_failure',
                                 message=str(exc))
                    return CallToolResult(is_error=True, content=[TextContent(type='text', text=json.dumps(error))],
                                          structured_content={'error': error})
            registered_tool = Tool.from_function(
                wrapped, annotations=ToolAnnotations(read_only_hint=readonly, destructive_hint=False,
                                                      open_world_hint=False), structured_output=False)
            model = registered_tool.fn_metadata.arg_model
            model.model_config.update(strict=True, extra='forbid')
            model.model_rebuild(force=True)
            registered_tool.parameters = model.model_json_schema(by_alias=True)
            registered.append(registered_tool)
            return wrapped
        return decorate

    def launch(operation):
        if cancelled():
            raise AdapterError('cancelled', 'Launch cancelled')
        result = operation()
        if cancelled():
            jobs.cancel(result['job_id'])
            raise AdapterError('cancelled', 'Launch cancelled; child cancellation requested', job_id=result['job_id'])
        return result

    @tool(True)
    def capabilities() -> dict:
        """Report configured paths, bounds and compiled backends; runtime graphics are unchecked. No window."""
        if not probe_lock.acquire(blocking=False):
            raise AdapterError('probe_busy', 'Another capability probe is in flight')
        try:
            result = probe_capabilities(runner, cancelled)
        finally:
            probe_lock.release()
        return dict(runner=str(runner), assets=str(assets), output_root=str(storage.root),
                    engine=result, sessions=2, active_jobs=1, terminal_jobs=64,
                    output_bytes=storage.capacity, replay_seconds=300, capture_points=8,
                    save_resume=False, balance_experiments='explicit_opt_in')

    @tool()
    def session_start(scenario: dict | None = None, scenario_path: str | None = None,
                      mode: str = 'diagnostic', seed: int | None = None,
                      handle: str | None = None, session: str | None = None, revision: str | None = None) -> dict:
        """Start a recorded arena, generated_floor or persistent campaign with doors, keys, discovery and checked stairs. Choose scenario object OR path. Replacement requires current tokens."""
        return sessions.start(scenario, scenario_path, mode, seed, handle, session, revision, cancelled)

    @tool(True)
    def session_list() -> dict:
        """List live session handles, output paths and busy/idle state."""
        return dict(sessions=sessions.list())

    @tool(True)
    def session_observe(handle: str) -> dict:
        """Read current observation, retaining diagnostic/visual mode and player filtering. Time does not advance."""
        return sessions.request(handle, 'observe', check_cancel=cancelled)

    @tool()
    def session_inspect(handle: str) -> dict:
        """Explicitly reveal diagnostic state; permanently marks diagnostic inspection in this run."""
        return sessions.request(handle, 'inspect', check_cancel=cancelled)

    @tool(True)
    def session_actions(handle: str) -> dict:
        """Discover bounded candidates and eligibility reasons. Preview a complete payload to establish legality."""
        return sessions.request(handle, 'actions', check_cancel=cancelled)

    @tool(True)
    def session_preview(handle: str, session: str, revision: str, action: dict) -> dict:
        """Check an existing headless action payload against expected tokens, without spending time/resources."""
        return sessions.request(handle, 'preview', session, revision, cancelled, action=action)

    @tool()
    def session_act(handle: str, session: str, revision: str, action: dict) -> dict:
        """Submit a checked headless action, including interact for known fixtures. Check response.ok AND accepted; rejected acts can consume revision."""
        return sessions.request(handle, 'act', session, revision, cancelled, action=action)

    @tool()
    def session_advance(handle: str, session: str, revision: str, ticks: int,
                        until_decision: bool = False) -> dict:
        """Advance 0..1000 fixed 10ms ticks. Decision stops may return zero; never automatically invent an action."""
        return sessions.request(handle, 'advance', session, revision, cancelled, ticks=ticks, until_decision=until_decision)

    @tool()
    def session_reset(handle: str, session: str, revision: str, seed: int | None = None) -> dict:
        """Reconstruct the original scenario; successful reset returns a new handle and invalidates old tokens."""
        return sessions.request(handle, 'reset', session, revision, cancelled, **({'seed': seed} if seed is not None else {}))

    @tool(True)
    def session_events(handle: str, cursor: int = 0, limit: int = 256) -> dict:
        """Page public events (max 256); expired cursors explicitly report a gap. Trace contents are diagnostic."""
        return sessions.events(handle, cursor, limit)

    @tool()
    def session_close(handle: str) -> dict:
        """Close and reap a live session; preserve its trace and finish recording when possible."""
        return sessions.close(handle, cancelled)

    @tool()
    def experiment_validate(manifest_path: str, structure_only: bool = False) -> dict:
        """Explicitly validate a manifest as a bounded job. Structure-only does not establish engine/policy validity."""
        return launch(lambda: jobs.experiment(manifest_path, validate=True, structure_only=structure_only))

    @tool()
    def experiment_start(manifest_path: str) -> dict:
        """Opt-in balance/playtesting job using existing manifest limits/policies. Never an automatic unrelated-change check."""
        return launch(lambda: jobs.experiment(manifest_path))

    @tool(True)
    def job_list() -> dict:
        """List bounded live/terminal job registry. No background jobs automatically resume after restart."""
        return dict(jobs=jobs.list())

    @tool(True)
    def job_status(job_id: str) -> dict:
        """Poll execution and engine batch status separately. Completion does not mean victory or verified balance."""
        return jobs.status(job_id)

    @tool()
    def job_cancel(job_id: str) -> dict:
        """Idempotently request cancellation; poll until terminal. Partial artifacts remain available."""
        return jobs.cancel(job_id)

    @tool(True)
    def job_results(job_id: str | None = None, directory: str | None = None) -> dict:
        """Get existing reports/summary overview. Choose job ID OR retained run directory; never resumes or regenerates it."""
        return jobs.results(job_id, directory)

    @tool()
    def job_forget(job_id: str) -> dict:
        """Remove terminal registry metadata only; retained artifacts are never deleted."""
        return jobs.forget(job_id)

    @tool(True)
    def trial_inspect(directory: str, trial_id: str) -> dict:
        """Inspect an existing trial's metrics, assumptions, coverage, decisions and trace retention. Diagnostic evidence."""
        return inspect_trial(storage, directory, trial_id)

    @tool()
    def trial_rerun(directory: str, trial_id: str) -> dict:
        """Explicit fresh-start policy rerun with existing strict code/binary/content checks; not a saved-fight resume."""
        return launch(lambda: jobs.rerun(directory, trial_id))

    @tool(True)
    def replay_inspect(trace: str, cursor: int = 0, limit: int = 64,
                       operation: int | None = None, kinds: list[str] | None = None) -> dict:
        """Page diagnostic trace evidence (record cursor, one-based operation); footer presence is not verified replay."""
        return inspect_trace(trace, cursor, limit, operation, kinds, cancelled)

    @tool()
    def replay_verify(trace: str) -> dict:
        """Launch strict full engine replay as a bounded job; preserve executable/content mismatches and truncation errors."""
        return launch(lambda: jobs.replay(trace))

    @tool()
    def replay_capture(trace: str, operations: list[int]) -> dict:
        """Verify then capture 1..8 operation boundaries using the same graphics-enabled binary, hidden window and current HUD."""
        return launch(lambda: jobs.replay(trace, operations=operations))

    @tool(True)
    def artifact_read(path: str, offset: int = 0, limit: int = 65536, image: bool = False):
        """Read up to 1 MiB of registered artifact text, or one PNG up to 16 MiB as an MCP image. Offsets are bytes."""
        value = storage.read(path, offset, limit, image)
        if image:
            return CallToolResult(content=[ImageContent(type='image', data=value['base64'], mime_type='image/png')])
        return value

    server = MCPServer('Blobber', version='13.3', lifespan=lifespan, tools=registered,
                       instructions='General-purpose local project tooling; use wherever relevant. '
                       'Balance experiments including smoke matrices are opt-in, never default verification. '
                       'Replay/trace inspection is diagnostic. No save/resume or OS desktop input.')
    return server

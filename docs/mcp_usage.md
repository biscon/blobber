# Local MCP play and investigation

Slice 13.3 adds an optional local stdio adapter to the existing headless interface,
experiment runner and strict replay. These tools are available wherever relevant
to project work. Examples below illustrate capabilities; they are not an exclusive
list of uses or conditions for using the tools.

**Balance experiments, including `smoke.json`, are opt-in.** Connecting the server,
building, running ordinary tests, or making an unrelated change never starts one.
The adapter does not change gameplay rules, provide arbitrary state mutation, or
implement slice 14's save/resume. Replay and policy rerun start from the beginning.

PC01 extends `session_start` with the explicit
[`generated_floor` scenario](headless_protocol.md#pc01-generated-floor-scenarios).
The normal session/reset/actions/replay/capture lifecycle works for empty generated
floors. Diagnostic captures include a labelled full-map companion image; no hidden
map is added to visual player observations. Existing experiment policies remain
arena-only; connecting the adapter does not launch a prototype or seed sweep.

## Install and invoke

Linux/POSIX is the supported process environment for this slice, matching the
existing client and runner. The official MCP SDK requires Python 3.11 or newer.
The optional lock pins `mcp==2.3.0` and its transitive dependencies with hashes;
it is resolved for Python 3.11+ (conditional dependencies retained). Validation
here used Python 3.14.8. No system package or toolchain installation is required.

From the repository root:

```sh
cmake --preset headless-release
cmake --build --preset headless-release --target blobber_headless -j 6
python3 -m venv .venv
.venv/bin/python -m pip install --require-hashes -r tools/mcp-requirements.lock
.venv/bin/python tools/blobber_mcp.py --help
```

Launch through an MCP client, using absolute paths:

```sh
/absolute/blobber/.venv/bin/python /absolute/blobber/tools/blobber_mcp.py \
  --workspace /absolute/blobber \
  --runner /absolute/blobber/cmake-build-headless-release/blobber_headless \
  --assets /absolute/blobber/assets \
  --output-root /absolute/blobber/output/mcp
```

The process speaks MCP, **not** the headless runner's JSON-lines protocol. Diagnostics
use stderr. It creates no session, experiment or window at startup. No HTTP listener,
authentication service, database or persistent execution daemon is involved.
Paths supplied to tools are local paths; relative paths resolve against the configured
workspace. The output root holds exclusively created session/job directories.

Existing tools remain stdlib Python and CMake does not install/import MCP. To refresh
the optional dependency lock deliberately, use the command recorded in its header
(`uv pip compile --universal --python-version 3.11 --generate-hashes ...`). Dependency
updates should receive their own SDK integration verification.

## Agent configuration

For Codex, copy this example into your chosen user/project configuration, replacing
`/absolute/blobber`. Installation does not modify your configuration automatically.

```toml
[mcp_servers.blobber]
command = "/absolute/blobber/.venv/bin/python"
args = [
  "/absolute/blobber/tools/blobber_mcp.py",
  "--workspace", "/absolute/blobber",
  "--runner", "/absolute/blobber/cmake-build-headless-release/blobber_headless",
  "--assets", "/absolute/blobber/assets",
  "--output-root", "/absolute/blobber/output/mcp"
]
cwd = "/absolute/blobber"
startup_timeout_sec = 15
tool_timeout_sec = 45
```

These are documented [Codex stdio configuration fields](https://learn.chatgpt.com/docs/extend/mcp).
No approval-policy overrides or global permission changes are needed in this example.
For another local MCP host, use the same executable as `command` and the same argument
array as `args`; set its working directory to the repository. Long work returns job
IDs immediately, so a tool timeout need not span an entire experiment.

For captures, build the normal graphics-enabled `blobber_headless` and use that
binary **from recording onward**, with its prepared assets directory:

```sh
cmake --preset release
cmake --build --preset release --target blobber_headless -j 6
```

Use `cmake-build-release/blobber_headless` and `cmake-build-release/assets` in the
configuration. Prepared scene/NPC/loot banks, fonts and UI assets must already be
available through the ordinary project asset build. The MCP server never bakes
assets automatically. If needed, the normal graphics build prepares them:

```sh
cmake --build --preset release -j 6
```

Explicitly forward display environment variables for application-owned hidden
captures. For Codex, add the following to the same server block:

```toml
env_vars = ["DISPLAY", "WAYLAND_DISPLAY", "XAUTHORITY", "XDG_RUNTIME_DIR"]
```

Other hosts have their own environment forwarding syntax. In the SDK client, pass
only the variables present in the invoking process through `StdioServerParameters.env`.
The SDK's default subprocess environment omits display variables. Missing graphics
support returns `graphics_unavailable`; CPU visual picking itself needs no display.
No OS input injection, focus changes, desktop switching or control of other windows
is used or permitted.

## Tool contract

Tools return structured content and a JSON text representation for client compatibility.
Adapter/schema/protocol failures are MCP tool errors. A valid gameplay refusal is a
normal result with `response.ok:true` and `response.data.accepted:false`; inspect both.
Original engine messages remain in `response`. [Headless protocol/action payloads](headless_protocol.md)
remain authoritative. Tool schemas reject unknown top-level fields and scalar coercions.

| Tools | Use |
| --- | --- |
| `capabilities` | Paths, bounds and compiled backends; runtime graphics remains `unchecked` until used |
| `session_start` | Exactly one scenario object or `scenario_path`; explicit `diagnostic`/`visual`; optional seed |
| `session_list`, `session_close` | Inspect live handles or finish/reap a session, retaining its trace |
| `session_observe`, `session_inspect` | Current mode-filtered state, or explicit privileged diagnostic inspection |
| `session_actions`, `session_preview` | Candidate discovery and authoritative legality of a complete action |
| `session_act`, `session_advance`, `session_reset` | Existing actions, 0–1000 fixed ticks, or fresh reconstruction |
| `session_events` | Public-event cursor pages, up to 256 events |
| `experiment_validate`, `experiment_start` | Explicit manifest validation or opt-in experiment jobs |
| `job_list`, `job_status`, `job_cancel` | Poll or cancel bounded background work |
| `job_results`, `job_forget` | Existing reports/artifacts, or forget terminal registry metadata without deleting files |
| `trial_inspect`, `trial_rerun` | Trial evidence/retention or strict fresh policy reproduction |
| `replay_inspect`, `replay_verify`, `replay_capture` | Diagnostic records, strict verification, or selected hidden scene/HUD captures |
| `artifact_read` | Registered artifact text pages or explicitly requested PNG image content |

A start result supplies an opaque `handle`, decimal-string `identity.session` and
`identity.revision`, the original response, an event cursor and a trace path.
Pass the latest identity to preview, act, advance, reset and replacement start.
The adapter checks these before the Python client supplies current tokens.
A stale call never gets silently upgraded to the latest revision. A successful
reset/replacement returns a new handle; failed replacement retains the old one.
Replacement retains the current observation mode. Reads do not advance time;
`session_inspect` permanently marks diagnostic inspection, as in the existing engine.

A complete payload example for `session_act` is:

```json
{
  "handle": "HANDLE_FROM_START",
  "session": "1",
  "revision": "1",
  "action": {"kind": "move", "direction": "forward"}
}
```

Action discovery does not enumerate all recipient combinations. Use preview for
complete spell/item/feature payloads. Full arena victory/defeat is terminal; partial
victory requires the existing `continue` action. A decision advance can return zero
at settled exploration; explicitly move or wait with `until_decision:false`.

Public events are retained in a 4,096-event/4 MiB ring, shared across resets within
that recording. Cursor zero reads from the beginning if retained. Expired cursors
return `event_gap` and the earliest available cursor. Trace records contain privileged
information even for visual play; they are never substituted into public event pages.

## Jobs, artifacts and lifecycle

There are two live sessions and one active background job maximum, with no queue.
Calls competing for one session receive `session_busy`. Sessions expire after
30 minutes idle. Ordinary/startup client deadlines are 10/30 seconds. Simulation
work retains the existing tick/mutation/request and 256 MiB recording limits.

Jobs retain existing experiment budgets; replay/capture jobs have a 300-second wall
limit. Status distinguishes execution (`running`, `cancelling`, `completed`, `failed`,
`timed_out`, `cancelled`) from engine `batch` status and trial combat outcomes.
A complete batch is not necessarily all victories, and a report is not a balance
verdict. Validation stdout contains its existing coverage result. No reports are
regenerated while a batch runs. See the [experiment guide](../experiments/arena/README.md)
for denominators, uncertainty, coverage limitations and retention rules.

New experiments freeze the resolved manifest and scenario inputs into their own
job directory. Jobs use the existing CLI, reports and strict rerun rules. Cancellation
requests orderly CLI shutdown first; after five seconds remaining owned processes
receive SIGTERM, then SIGKILL one second later. Stdout is capped at 8 MiB; stderr is
a drained 1 MiB tail, explicitly labeled when truncated. Terminal status is published
after final adapter artifacts. `job_cancel` is idempotent and must be followed by polling.

The output root defaults to an 8 GiB admission budget. Each launch reserves its maximum
output, inputs and diagnostic margin before it starts; actual retained files count
against later launches. Admission may reject a large reservation despite ample space
for the expected average run. Reduce a manifest's explicit budget deliberately, remove
unneeded output yourself, or choose another output root. No budget is silently reduced
and no retained failures are automatically deleted by the adapter. Existing experiment
trace retention still applies within each run.

At most 64 job records are retained; `job_forget` frees a terminal registry slot, not
its files. `job_results(directory=...)` can open retained outputs after restart; it
marks abandoned recorded `running` states interrupted and never resumes them. Up to
64 external run directories can be registered for artifact reads. The output root
is registered automatically. Path traversal and symlinks escaping those roots reject.
Text reads use byte offsets, default 64 KiB and maximum 1 MiB; PNG reads allow 16 MiB.
Do not infer trace replayability from its file merely existing.

EOF/SIGINT/SIGTERM closes sessions and cancels/reaps jobs. An uncertain interrupted
mutation makes its session unusable; the tool reports that it may have completed and
never retries it. Abrupt process death is not a durable job service: output survives,
but there is no automatic recovery or session continuation.

## Replay and captures

`replay_inspect` accepts a trace path, record cursor, optional one-based request
`operation`, record-kind filters, and up to 256 records. Pages are bounded at 4 MiB.
`footer_present` is only a structural observation; `verification:not_checked` remains
until a separate strict verification job succeeds. A record cursor is not a tick.

`replay_verify` freezes the trace and invokes existing strict replay. Errors preserve
incompatibility, mismatch and incomplete-recording distinctions. Rebuilt executables
and changed content can invalidate recordings even when a change seems unrelated.
A graphics-disabled trace cannot be captured by switching to another executable.

`replay_capture(trace, operations)` selects 1–8 distinct completed request boundaries.
The same graphics-enabled binary verifies the whole trace, then reconstructs again
and captures those boundaries with a hidden application-owned window. Rendering is
checked against normalized state before/after each observer callback. Capture output
is at most 64 MiB, PNG at fixed 1920×1080. Metadata includes trace/header/prepared-assets
identity and each operation's ticks/session/revision. `job_results` exposes the capture
manifest and image paths; `artifact_read(image=true)` returns an MCP image.

The HUD shows current gameplay state using the existing presenter. Cursor position,
open dialogs and historical UI interaction/log state are not reconstructed. Captures
are not recordings of a prior user's desktop. Failed capture jobs can retain partial
images, but only a successfully completed job/manifest establishes verified capture.
A missing backend, assets or graphics context is a presentation limitation, not a
combat outcome. Capability probing opens no window and does not claim GPU readiness.

## Review and validation

Routine stdlib contracts (no fight, matrix, or MCP dependency):

```sh
python3 tests/mcp_adapter_tests.py
ctest --preset headless-release \
  -R '^(automation|headless_cli|arena|arena_experiment_tools|automation_allocations|mcp_adapter_tools)$'
```

Explicit optional SDK integration, using generated fixtures:

```sh
.venv/bin/python tests/mcp_stdio_tests.py cmake-build-headless-release/blobber_headless
.venv/bin/python tests/mcp_cancellation_tests.py
```

Explicit fixture playthrough/experiment/rerun/cancellation review (not CTest):

```sh
.venv/bin/python tests/mcp_stdio_tests.py cmake-build-headless-release/blobber_headless \
  --playtest --evidence /tmp/blobber-mcp-fixture-review
```

For human agent review: connect; inspect capabilities; start/observe/discover/preview;
move and advance; inspect events; submit an old revision to see rejection; reset and
verify the old handle fails; finish/close the fixture encounter above. Then deliberately
call `experiment_start` on `experiments/arena/smoke.json` (18 trials), poll its job,
and read `report.md` through `job_results`/`artifact_read`. Choose a retained trial
from the run's index, inspect it, verify its trace and perform `trial_rerun`. Launch
another explicitly bounded job and cancel it to inspect partial output.

Optional graphics review uses supplied game content, not test fixtures:

```sh
.venv/bin/python tools/mcp_capture_review.py \
  --runner cmake-build-release/blobber_headless \
  --assets cmake-build-release/assets \
  --scenario experiments/arena/parties/balanced.json \
  --output /tmp/blobber-mcp-capture-review
```

This approaches contact through normal commands and captures start/movement/contact
boundaries. Select a terminal operation from a completed recorded trial separately
when investigating victory/defeat. Disconnect and verify owned processes stop and
artifacts remain. Record unavailable graphics checks as unexecuted; interactive
agent-host/desktop review belongs to the user. [Design](mcp_design.md) and
[verification/handoff](slice133_verification.md) record the implemented boundary and evidence.

## PC02 fixture interactions

Generated-floor scenarios now require generator/template versions **2/2**, with
scenario schema 1 unchanged. `session_preview` and `session_act` accept
`{"kind":"interact","fixture":0}` through the same checked host command as F/click
in the game. Use current observation tokens and fixture IDs from observations or
action discovery. Player observations contain discovered cells, remembered fixtures
and acquired quest keys; diagnostic inspection remains explicit and does not reveal
cells to later player observations. Strict replay includes these states. Generated
captures now include scene, discovered map and diagnostic map PNGs.
See [protocol and rules](headless_protocol.md#generated-floor-scenarios-pc01pc02) and
[PC02 verification](pc02_verification.md). No new MCP tools or balance policy are needed.

# Slice 13.3: local MCP play and investigation

Status: implemented, **Accepted and closed**, 2026-10-30. The user approved the reviewed
plan with “Implement the plan.” 13.1/13.2 remain Accepted and closed; save/resume
remains slice 14 and unstarted. After reviewing the capture gallery, the user accepted:
“I accept the slice, record it in the doc”. The design was recorded before code changes and now describes the implemented result.
[Usage/configuration](mcp_usage.md) · [Verification/handoff](slice133_verification.md).

## Responsibilities and interfaces

An optional Python stdio MCP server adapts the existing Client, experiment CLI,
reports and strict C++ replay. It owns no game rules or alternate policy. Tools
cover session start/replacement/reset/close/list, observe/explicit diagnostic inspect,
actions/preview/act/advance, paged public events, experiment validation/start,
job list/status/cancel/results/forget, trial inspection/rerun, replay
inspection/verification/capture and bounded artifact reads. Use is open-ended
wherever relevant to project work. All balance matrices, including smoke, remain
explicit opt-in playtesting outside default builds/tests/CI and unrelated checks.

The official MCP SDK 2.3.0 is pinned in an optional hashed virtual-environment lock. Existing stdlib tools
and CMake builds do not depend on the SDK. STDIO only, no hosted service, database,
queue service, worker pool or sampling. Subprocess argv never passes through a shell.
Only MCP writes server stdout; diagnostics and drained child output are separate.

## Ownership, capacities and failure handling

Two live sessions maximum, one Client/child each; one in-flight operation per
session with fail-fast busy errors. Opaque handles rotate only after successful
reset/replacement. Mutations and preview compare caller decimal-string engine
identity/revision under the session lock before the client fills current tokens.
Failed replacement preserves the old handle. Queries preserve visual filtering;
inspect explicitly exposes diagnostics. Record by default into exclusive files.
A public event ring holds at most 4096 events/4 MiB, pages of 256; expired cursors
report a gap. Never replace public events with private trace data.

One active background job, no queue, 64 retained terminal records. Forget removes
metadata, never files. CLI experiments use frozen manifest/scenario inputs and
existing preflight, serial trials, budgets, retention, reporting and rerun checks.
Status derives recorded-trial progress from the runner’s atomic batch metadata, keeping execution,
batch, combat, artifact and verification outcomes separate. Original reports and
statistics remain authoritative. Read existing runs without resuming execution.

Output root defaults to an 8 GiB budget with maximum-operation reservations before
admission; never silently reduce requested work. Existing trace limits (256 MiB),
experiment limits and request bounds remain. Session startup/request deadlines are
30/10 seconds, idle expiry 30 minutes. Replay/capture deadline 300 seconds; capture
output 64 MiB, at most eight operation boundaries. Cancellation is cooperative,
then owned process-group termination after five seconds and kill after one more.
EOF/signals clean up and reap children; unexpected death cannot resume work.
Uncertain in-flight mutations invalidate the session and are never retried.
Artifact paths are rooted, bounded regular files; reject traversal/symlink escape.

## Replay and application-owned captures

Inspection streams recorded requests/events/responses/boundary state with one-based
operation indices and distinct completeness/verification labels. Verification
reconstructs through existing strict replay, retaining binary/content/timing checks
and first mismatch operation/path. No snapshots/checkpoints or save-game semantics.

A small optional C++ observer exports selected completed replay boundaries. Verify
the full trace first, then replay again to render selected points through existing
NpcAnimator/FramePipeline/SceneRenderer/CommandController/Presenter. Preserve state
before/after callbacks. Capture current scene and HUD at 1920x1080; do not reconstruct
cursor, dialogs or historical UI interactions. Metadata ties images to trace header,
operation/ticks/session/revision, assets and verification. Same executable identity
is required: headless-build traces cannot be captured by switching binaries.
Graphics-disabled builds retain all CPU tools. Missing backend/assets/context yields
an explicit graphics error, never a combat loss. Hidden application-owned windows
only; no OS input/focus/desktop automation. GPU resources die before their window.

## Installation, validation and handoff

Provide hashed optional dependency lock, venv installation, absolute-path invocation,
Codex config and generic stdio configuration. Do not modify user agent configuration.
Startup does not run sessions, experiments, baking or window probes.

Fast fixture-owned contracts cover schemas, forwarding, stale/busy/lifecycle errors,
limits/pagination, cancellation/cleanup, bounded artifacts and report semantics.
Official SDK stdio integration covers discovery, output/errors/cancellation and
legacy/current protocol compatibility. Real short generated-fixture command sequences compare direct and
MCP state/events. Autonomous experiments remain separate explicit integration
review commands. C++ Debug/Release replay/automation/allocation/visual/HUD checks
and focused sanitizers validate changed lifetimes. Hidden GPU/capture checks are
separate; unavailable checks are explicitly unexecuted. Review includes a fixture
encounter, opt-in smoke comparison, cancelled batch, trial rerun/replay and selected
captures. Evidence and limitations are recorded in [the verification handoff](slice133_verification.md).
Tracking reached Ready for review before user acceptance on 2026-10-30; 13.3 is now closed.


## Implemented module boundaries and verification limits

`tools/blobber_mcp.py` resolves invocation paths and starts the official SDK.
`mcp_adapter/server.py` registers strict tool argument schemas and MCP results,
handles lifespan/signals/cancellation, and confines SDK imports to this boundary.
`sessions.py` owns existing Client instances, handle/revision admission and public
rings; `jobs.py` owns CLI/replay children and bounded process groups, output draining,
status publication and cancellation. `storage.py` owns reservation accounting and
registered artifact access; `investigation.py` streams recorded evidence. The
existing experiment/policy/report implementation and client remain unchanged.

`automation/trace.*` calls an optional observer after existing response/state checks
and checks state again after the callback. The ordinary replay path retains its
signature through a default null observer. Internal mutable accessors exist only to
borrow the existing HUD controller; no state-setting verb is exposed on the wire.
`automation/capture.*` owns renderer/window/presenter resources in dependency order,
uses fresh transient HUD state at selected boundaries, and publishes capture
metadata only after the second replay succeeds. No new allocator work enters the
normal simulation/frame path; JSON, SDK, inspection, image export and file I/O are
diagnostic work. Screenshot files may remain after a failed capture; job/manifest
status determines success, never file existence.

The first capture attempt correctly reported missing display configuration. Explicit
forwarding allowed hidden NVIDIA OpenGL capture verification. The optional SDK client
omits display variables by default; this is documented, not treated as a simulation
failure. Current/legacy SDK clients and Codex command configuration parsing were
verified. Live interaction through the user's Codex host remains a user review step;
no installed agent configuration or desktop input/focus was changed. Full unrelated
suites and performance benchmarks were not repeated. See the handoff for exact scope.

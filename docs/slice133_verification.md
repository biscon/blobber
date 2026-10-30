# Slice 13.3 verification and handoff

**Roadmap update (2026-10-30):** The user subsequently closed the static-demo
roadmap. Its unimplemented save/resume slice 14 moves to
[campaign PC06](procedural_campaign_plan.md#pc06-campaign-save-and-resume), after
generation and persistent travel. Earlier next-slice directions below are historical;
13.3 acceptance and verification limits are unchanged.


Status: implemented, **Accepted and closed**, 2026-10-30. User authorized implementation
with “Implement the plan.” The user accepted after reviewing the capture gallery:
“I accept the slice, record it in the doc”. Slices 13.1/13.2 remain Accepted
and closed; save/resume remains slice 14 and has not started.

[Design](mcp_design.md) · [Installation, configuration and review recipe](mcp_usage.md) ·
[Capture gallery](screenshots/slice133/index.html) · [Automation roadmap](automation_plan.md)

## Delivered

An optional official-SDK stdio server exposes session lifecycle, stale-safe checked
play, observations/diagnostic inspection, action discovery/preview, bounded public
events, explicit experiment validation/jobs, status/cancellation/results, retained
trial inspection, strict rerun/replay, and application-owned capture/artifact access.
It reuses the existing Client and experiment CLI/report/policy implementation without
changing them. No gameplay rules, action semantics, policy behavior or balance values
were changed. Uses remain open-ended wherever relevant to project work.

Two sessions, one background job, bounded registry/events/I/O/output reservations,
exclusive artifact directories, cooperative/forced child cleanup and clear error
categories define ownership. Strict caller tokens are checked before the client
supplies its current tokens. Unknown tool fields and coerced scalars reject. Session
handles rotate on successful reconstruction. Uncertain cancelled actions invalidate
the session rather than being retried. Explicit close reports recording completeness
and runner exit status. Completed job status follows final artifact publication.

The C++ replay observer runs only after normal replay comparisons and verifies that
capture does not change normalized state. The optional backend uses the existing
prepared renderer and current HUD in a hidden application-owned window, at 1920×1080.
Trace executable identity remains strict. Capture manifests record provenance and
selected operation/tick/session/revision values. No OS input, focus or desktop control
occurred. Save/resume, hosted services and worker pools were not added.

All balance matrices, including smoke, remain explicit opt-in commands. The new
`mcp_adapter_tools` CTest entry is a short stdlib contract suite with owned fake
processes/data; it does not start a game or matrix. Optional SDK integration and
fixture playthroughs are separate commands. CMake never installs/imports MCP.

## Executed checks

| Check | Result | Evidence |
| --- | --- | --- |
| Stdlib adapter contracts | 19 passed | `python3 tests/mcp_adapter_tests.py`; `/tmp/blobber133-adapter-final.log` |
| Headless Debug focused CTest | 6/6 passed | `/tmp/blobber133-headless-debug-tests.log` |
| Headless Release focused CTest | 6/6 passed | `/tmp/blobber133-headless-release-tests.log` |
| Graphics Debug focused CTest | 7/7 passed | `/tmp/blobber133-graphics-debug-tests.log` |
| Graphics Release focused CTest | 7/7 passed | `/tmp/blobber133-graphics-release-tests.log` |
| ASan/UBSan headless automation/replay/allocation/CLI | 3/3 passed with leak detection | `/tmp/blobber133-sanitize-tests.log` |
| ASan/UBSan graphical replay/capture | Four selected captures and strict replay passed; no sanitizer errors reported | `/tmp/blobber133-sanitize-captures.log`; `/tmp/blobber133-sanitize-captures/` |
| Optional SDK stdio contracts | Current and legacy connections passed; discovery/schema errors, direct-client equivalence, stale/reset, incomplete/incompatible replay, cleanup | `/tmp/blobber133-stdio-final.log` |
| Optional SDK cancellation | In-flight unanswered action invalidated/reaped; SIGTERM cleaned up live runner | `/tmp/blobber133-cancellation-final.log` |
| Explicit generated-fixture integration | Complete encounter, experiment validation/run/reports, policy rerun and cancelled job passed | `tests/mcp_stdio_tests.py --playtest`; `/tmp/blobber133-fixture-final/` |
| Approved opt-in smoke comparison through MCP | 18 recorded victories, no other outcomes/artifact errors | `/tmp/blobber133-smoke-mcp-final/review.json` and run report below |
| Selected smoke trial reproduction | Strict engine replay and policy rerun matched | Same review record, trial `ab72e62090ec1359a8054aeb` |
| Diagnostic replay capture through MCP | Start, action and terminal state captured; PNG returned through MCP | Gallery: operations 1, 46 and 97 |
| Prepared visual play/replay/capture through MCP | Start, movement and contact captures passed | Gallery: operations 1, 2, 3 and 22; `/tmp/blobber133-visual-final/review.json` |
| Missing graphics configuration | Correct `graphics_unavailable`, distinct from combat result | `/tmp/blobber133-capture-review/` (SDK client omitted DISPLAY) |
| Hidden GPU lifecycle | Passed on NVIDIA GeForce RTX 3060/OpenGL | `/tmp/blobber133-gpu.log` |
| Hashed optional installation | Passed on Python 3.14.8; universal Python 3.11+ lock generated | `/tmp/blobber133-install-final.log` |
| Codex configuration parsing | Local command/argument overrides accepted without modifying installed configuration | `codex -c … mcp get blobber --json` |
| Syntax/whitespace/doc links | Passed | Python compile checks, local-link checks and `git diff --check` |

The CPU suites include the existing allocation guards; they do not assert that
Python, protocol JSON, image export, graphics drivers or third-party internals never
allocate. The new C++ observer test covers consecutive operation boundaries across
reset and deliberately mutating observer rejection. Fast Python contracts cover
revision admission before forwarding, failed replacement, capacity/busy/expiry,
event eviction, reservation accounting, artifact escape/FIFO rejection, streamed
inspection, process failure, bounded pipes, deadlines, cooperative/escalated
cancellation and shutdown. They own their fixtures and temporary directories.

Exact focused suite selections:

```sh
ctest --preset headless-debug \
  -R '^(automation|headless_cli|arena|arena_experiment_tools|automation_allocations|mcp_adapter_tools)$'
ctest --preset headless-release \
  -R '^(automation|headless_cli|arena|arena_experiment_tools|automation_allocations|mcp_adapter_tools)$'
ctest --preset debug \
  -R '^(automation|automation_visual|automation_allocations|headless_cli|commands|hud|mcp_adapter_tools)$'
ctest --preset release \
  -R '^(automation|automation_visual|automation_allocations|headless_cli|commands|hud|mcp_adapter_tools)$'
```

The sanitizer builds used `-fsanitize=address,undefined -fno-omit-frame-pointer
-fno-pie` and `-no-pie`. The headless run explicitly enabled leak detection. The
optional SDK client did not forward the invoking shell's ASAN_OPTIONS override to
the graphics process, so graphical captures ran with the sanitizer runtime's default
options; no leak/runtime diagnostic appeared. No graphics-driver allocation claim
is made. Builds reuse existing local toolchains/dependencies.

## Review artifacts and interpretation

The explicit smoke output is:

`/tmp/blobber133-smoke-mcp-final/experiment-e3cdc9dbd07547cbad4dfed43a3b9ea6/run`

Read its `report.md`, then `summary.json`, `trial_index.json` and the selected trial's
decisions. This is an 18-trial adapter/workflow review using the accepted parties,
policies and seed matrix, not a new balance conclusion. All victories at this small
sample do not establish class viability, optimal play or universal encounter wins.
No exploratory matrix, tuning sweep or benchmark was run in this slice.

The gallery retains seven 1920×1080 PNGs and metadata. Image filenames in copied
manifests have a mode prefix to avoid collisions; `original_file` retains the actual
exported name. Scene/action/terminal captures were visually inspected. The retained
HUD intentionally shows fresh current-state presentation, including the normal
initial combat-log hint; it does not claim to reconstruct historical UI interaction.
Rendering uses actual prepared imagery and normal arena-completion presentation.

The initial graphics attempt had no DISPLAY inside the SDK-launched process and
correctly failed. Explicit display-variable forwarding enabled later hidden GPU
checks and captures. The unavailable first attempt is not counted as a graphics
pass; the separately successful checks above are the evidence. The usage guide
includes required host environment forwarding and the matching-executable restriction.

## Limitations and next action

- Actual interaction through the user's configured Codex host is **unverified**.
  Configuration parsing and official-SDK current/legacy interoperability passed;
  no user configuration was installed or changed automatically.
- Interactive desktop review is **unverified** and was not attempted. All exported
  images came from application-owned hidden windows. No input/focus automation.
- Python 3.11–3.13 and other operating systems were not executed. The universal
  dependency lock targets Python 3.11+; runtime verification used Linux/Python 3.14.8.
- Strict recordings may become incompatible after rebuilding the executable or
  changing content. A headless-only trace cannot gain graphics by switching binaries.
- Capture represents current scene/HUD at command boundaries, not arbitrary tick
  checkpoints, a full historical UI replay, or a save/resume facility.
- Full unrelated test suites, new performance benchmarks and balance exploration
  were not repeated. Focused checks above are the claimed verification scope.

The [review recipe](mcp_usage.md#review-and-validation) remains available for later
manual verification.

## Accepted closeout and next-agent handoff

2026-10-30: The user reviewed the capture gallery and explicitly accepted the slice:
“I accept the slice, record it in the doc”. Slice 13.3 is **Accepted and closed**.
The checkpoint, tracker, execution record, automation plan and system overview are
synchronized. Prior slices remain closed.

Verification scope and limitations above remain unchanged. Gallery review does not
establish live Codex-host interaction or interactive desktop verification. This
closeout changes documentation only; no tests, experiments or captures were rerun.
Tooling use remains open-ended wherever relevant to project work. Every balance
experiment, including smoke matrices, remains opt-in and outside default tests,
builds, CI and automatic unrelated-change checks.

Slice 14 (save/resume) is next eligible for separate planning and remains unstarted.
When requested, begin in a clean context by reading AGENTS.md, the system design,
roadmap and this handoff, then inspect the relevant implementation and present its
detailed plan for review before implementation. Do not start it automatically.
All working-tree changes are preserved; no commit was requested or created.

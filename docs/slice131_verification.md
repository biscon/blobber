# Slice 13.1 verification and handoff

Status: implemented, **Accepted and closed**, 2026-10-30. Implementation was authorized
by “Implement the plan.” The user accepted the result with “slice accepted, update
the docs.” Slice 13 and milestone C remain accepted and closed; 13.2/13.3 remain
Planned and unstarted. Acceptance does not change the recorded verification scope
or turn unexecuted interactive checks into passing checks.

[Design](headless_design.md) · [Protocol and client recipe](headless_protocol.md) ·
[Automation roadmap](automation_plan.md) · [Demo checkpoint](dnd5e_demo_design.md#current-checkpoint)

## Delivered

`blobber_headless` runs existing level-one ArenaScenario documents through GameSession,
with transactional start/reset, versioned NDJSON, revision-checked typed commands,
fixed 10 ms simulation steps, bounded waiting/decision stops, observations/discovery,
explicit diagnostic inspection, event streaming and strict record/replay.

Commands cover movement, weapon/compound spell/feature/item/Aid actions, inventory and
equipment, drop/pickup, rest/preparation, earned advancement and partial-victory
continuation. There is no development XP, teleport or arbitrary-state verb. A terminal
arena requires reset. The Python stdlib client fills identities/revisions and runs
scripts; it does not choose tactics or run experiments.

Diagnostic mode needs catalog/NPC JSON only. Metadata parsing is shared with normal
image loading, preserving canonical fingerprints, definition order and timings.
The optional visual mode loads prepared scene/NPC/loot banks, uses CPU FramePipeline
picking, filters hidden enemy/private state and suppresses pre-impact result events.
The common visibility helper retains stale world/revision checks and also rejects
hidden back-row melee and Spiritual Weapon follow-up targets. The live frame scheduler,
prepared-image approach, presentation size and movement rules remain unchanged.

Recording is exclusive and streamed, with executable/content/mode/timing identity,
ordered requests, normalized events, responses and diagnostic boundary states. Replay
reconstructs from the start and reports the first differing JSON field. Request limits
remain 2 MiB/32 levels; trace envelopes reserve additional space/depth so rejected
boundary-sized requests can be replayed. Trace files, across resets, cap at 256 MiB.

## Verification results

All commands ran from the repository root on Linux. Logs below are local `/tmp`
artifacts, not durable test fixtures; the tests and reproduction commands live in the
repository.

| Check | Result | Local log |
| --- | --- | --- |
| Full graphics Debug CTest | 48/48 passed | `/tmp/blobber-131-debug-tests.log` |
| Full graphics Release CTest | 48/48 passed | `/tmp/blobber-131-release-tests.log` |
| Final focused Debug/Release after visibility/discovery guards | 6/6 each | `/tmp/blobber-131-{debug,release}-focused-tests.log` |
| Graphics-disabled Debug CTest | 30/30 passed | `/tmp/blobber-131-headless-tests.log` |
| Graphics-disabled Release CTest | 30/30 passed | `/tmp/blobber-131-headless-release-tests.log` |
| Focused ASan/UBSan with leak detection | 7/7 passed | `/tmp/blobber-131-sanitize-tests.log` |
| 256-actor, 60,000-tick typed capacity exercise | Zero tracked C++ allocations; 71 accepted actions, 698 observed events | `/tmp/blobber-131-capacity.log` |
| Shipped catalog/prepared banks; both modes, no DISPLAY/WAYLAND_DISPLAY | Victory and fresh-process matching replay in each | `/tmp/blobber-131-shipped-smoke.log` |
| Hidden raylib/OpenGL resource lifecycle | Passed on NVIDIA RTX 3060 | `/tmp/blobber-131-gpu.log` |

Final focused suites cover automation, visual automation, automation allocations,
combat, commands and CLI. The sanitizer suite covers automation, metadata, CLI,
combat, commands, progression and arena in a graphics-disabled build. The full suites
also retain the existing renderer, picking, image preparation, NPC animation,
progression, recovery, inventory and frame-allocation checks where graphics is enabled.

Routine fixtures own their catalog, NPC metadata, generated images and isolated
temporary files. Coverage includes:

- Malformed/unknown/versioned input, duplicate keys, nesting/byte bounds, negative
  ticks, stale revisions and reset handles, invalid replacement atomicity, read-only
  observation/preview, movement during transitions, frozen/explicit time, the full
  360,000-tick run ceiling and resetting that budget.
- Complete deterministic victory and defeat, a normal-controller/fixed-step trace
  comparison, spell impacts with three recipients, invalid dart assignments with no
  cost, recovery feature/rest, inventory split/drop/pickup, and earned level-two
  advancement after a ranged kill, retreat and safe recovery.
- Partial-victory/continue uses a dedicated test setup that moves one fixture guard
  beyond alert range through ActorWorld before play; the protocol exposes no such
  operation. This isolates the boundary without an expensive large encounter in the
  routine suite. Complete-fight and replay fixtures use unmodified arena generation.
- Same-process and fresh-process replay, resets, input/state/content mismatches,
  incomplete footers, no-overwrite, trace exhaustion and near-limit/deep rejected
  requests inside trace envelopes. CLI tests also exercise the example Python client.
- Exact target membership against ordinary FramePipeline/UI validation with walls,
  overlap, offscreen enemies, corpse removal and movement; filtered observations,
  explicit-inspection provenance and suppression of unresolved attack rolls.

Graphics-disabled builds were configured in fresh build directories. They do not
create raylib/image/baking targets; `ldd` on the runner lists standard C/C++ libraries
only. CLI fixtures deliberately reference nonexistent images and run without display
environment variables. Shipped-content smoke runs in both modes used the same default
party, one goblin and seed 5: 34 scripted decisions, 977 ticks, victory; diagnostic
output contained 14 events and visual output 9, reflecting pre-impact filtering.
Both traces replayed exactly with their respective mode and the same executable.

Allocation checks cover engine-owned C++ `new`/aligned allocations in prepared typed
stepping, actions, validation, event staging and observation work. Graphics builds
also exercise prepared visual observations. JSON serialization, trace I/O, setup,
C allocations and driver internals are outside that claim. The separate maximum-roster
exercise establishes bounded work/storage under load, not a complete 256-enemy victory
or a production throughput benchmark.

## Review and repeat

```sh
cmake --preset headless-debug
cmake --build --preset headless-debug -j 6
ctest --preset headless-debug
cmake-build-headless-debug/blobber_automation_allocations_tests --stress
python3 tests/headless_cli_tests.py "$PWD/cmake-build-headless-debug/blobber_headless"
```

For a concrete saved scenario, follow the script/record/replay recipe in the
[protocol guide](headless_protocol.md#client-and-replay-recipe). Use `--mode visual`
with `cmake-build-debug/blobber_headless --assets cmake-build-debug/assets` to select
prepared CPU visibility. Use a fresh recording filename. Rebuilding the executable
or changing its content can invalidate earlier traces by design.

To repeat focused sanitizer verification, configure a separate Debug Ninja directory
with `BLOBBER_BUILD_GRAPHICS=OFF`, `BUILD_TESTING=ON`, C++ flags
`-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie` and executable linker
flags `-fsanitize=address,undefined -no-pie`. Build the corresponding test targets plus
`blobber_headless`; run CTest with `ASAN_OPTIONS=detect_leaks=1` and regex
`^(automation|headless_cli|npc_metadata|combat|commands|progression|arena)$`.
The checked directory is `/tmp/blobber131-sanitize`.

## Limits and next step

Interactive desktop review is unverified and left to the user. No OS input, focus
changes or desktop automation was used. Hidden GPU checks create only their own
application window. There was no presentation redesign, so no new screenshot gallery
was needed for this slice.

Visual mode exposes structured current picking and known party state, not a persistent
fog-of-war map or screenshot stream. Full traces remain diagnostic even for visual
play. Scenario starts remain level one; later levels require normal earned progression.
Replay is strict same-build/content reconstruction, not cross-platform/save-game
compatibility. No speed multiplier or balance conclusion is claimed. MCP, tactical
policies, composition matrices and reports remain later slices.

## Accepted closeout and next-agent handoff

The user accepted slice 13.1 on 2026-10-30: “slice accepted, update the docs.”
No corrections were requested. The verification results and limitations above remain
unchanged. No review is pending for 13.1.

Next action: plan **13.2 — Automated players and balance experiments** in a fresh
context. Read AGENTS.md, the [system design](system_design.md),
[demo checkpoint/workflow](dnd5e_demo_design.md#current-checkpoint),
[automation scope and acceptance criteria](automation_plan.md#slice-132-automated-players-and-balance-experiments),
[headless design](headless_design.md), [protocol](headless_protocol.md), and this handoff.
Inspect `src/automation/`, `tools/headless.cpp`, `tools/headless_client.py`, the arena
scenario/party builders and automation tests before choosing the design. Preserve
existing working-tree changes.

13.2 adds repeatable local player policies, bounded party/roster/policy/seed matrices,
reports with explicit failure classification and uncertainty, and reproducible trial
traces. Include a no-mage party and multiple policies, label information/reaction
assumptions, and benchmark before adding concurrency or claiming throughput. Preserve
13.1's checked commands, observation modes and strict replay; level-one scenarios
remain the accepted starting format. Follow the roadmap's plan-review-implementation
workflow for 13.2 only. MCP and capture tools remain 13.3; save/resume remains slice 14.


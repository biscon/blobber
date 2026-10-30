# Arena automation: headless play and balance testing

Status: **13.1 Accepted and closed** (2026-10-30); its [detailed design](headless_design.md)
is implemented; [verification/handoff](slice131_verification.md) and the
[protocol guide](headless_protocol.md) are available. The user accepted 13.1 on
2026-10-30: “slice accepted, update the docs.” Slice 13.2 is next eligible for separate
planning; 13.2/13.3 remain Planned and unstarted.

## Delivery order and tracking

Insert three slices after accepted slice 13 and before slice 14 (save/resume).
The [demo roadmap](dnd5e_demo_design.md#current-checkpoint) remains authoritative
for active work and acceptance. Its one-slice planning, implementation, verification,
and human-review workflow applies here. Accepted milestone C stays closed; these
slices form a tooling interlude rather than reopening gameplay acceptance.

| ID | Slice | Prerequisite | Status |
| --- | --- | --- | --- |
| 13.1 | [Headless arena sessions and reproducible play](#slice-131-headless-arena-sessions-and-reproducible-play) | 13 | Accepted |
| 13.2 | [Automated players and balance experiments](#slice-132-automated-players-and-balance-experiments) | 13.1 | Planned |
| 13.3 | [MCP play and investigation](#slice-133-mcp-play-and-investigation) | 13.2 | Planned |

Each slice receives its detailed design before implementation and its own durable
verification/handoff record. Update the corresponding record below and the demo
checkpoint/tracker together. Slice 14 follows acceptance of 13.3.

## Purpose and existing foundation

Support repeatable arena play now, while the dungeon is still a test level, and
extend the interface alongside later gameplay. Compare party compositions,
equipment, tactics, and encounter rosters over many trials. Let an agent play
individual sessions and investigate outcomes without desktop input automation.

Inspected foundation (2026-10-30): `blobber_core` and `blobber_rules` have no raylib
dependency. `GameSession` owns simulation state, accepts checked requests, advances
explicit time, and exposes bounded event batches. Arena scenarios retain independent
party builds, monster quantities, and a seed. Existing tests drive sessions without
a window. Full UI captures instead run engine/rendering code through a hidden
window; they are not a general headless player interface.

Read [system design](system_design.md), [arena design](arena_design.md),
[hybrid encounter design](hybrid_encounter_design.md), and the
[slice 13 handoff](slice13_verification.md) before detailed planning. Relevant code:
`src/game/session.*`, `combat.*`, `command_types.hpp`, `commands.*`, `arena.*`,
`src/app/gameplay.cpp`, `src/assets/npc_loader.*`, and `CMakeLists.txt`.

## Proposed shared architecture and contracts

- **Simulation:** reuse the actual C++17 game rules and session operations. No
  alternate combat implementation. Gameplay systems remain independent of MCP,
  reporting, and player policies; expose small shared commands/queries where needed.
- **Session host:** own immutable catalog/map/placements/timing data before the
  borrowing session, destroying the session first. Reset reconstructs from the
  recorded scenario. Keep protocol-visible session identities separate from
  process-local actor/item handles; reject stale requests after reset.
- **Headless executable:** load rules and gameplay metadata without a window or
  prepared-image banks. Separate metadata loading/build dependencies where needed.
  Use a versioned JSON request/result interface over standard input/output, with
  diagnostics on a separate channel. Reuse the existing JSON dependency.
- **Observation/action boundary:** expose structured party/enemy state, positions,
  resources, conditions, turn ownership, action eligibility, and events. Actions
  use the same authoritative validation and costs as normal play. Define the
  initial supported operation matrix in 13.1; unavailable operations must be
  explicit, never silently approximated or implemented by arbitrary state mutation.
- **Visibility:** distinguish unrestricted diagnostic observations from
  player-equivalent observations and targeting. Current UI visibility comes partly
  from rendered body picking, beyond core range/line-of-sight checks. Resolve this
  in 13.1's design: exact CPU picking may require an optional asset-backed mode;
  any cheaper visibility query needs parity checks. Do not call an omniscient or
  approximate mode player-equivalent, or quietly change normal targeting rules.
- **Time:** process bounded, consistent simulation steps without wall-clock sleeps
  or frame presentation. Preserve movement duration, attack impacts, effect expiry,
  cooldowns, and automatic-action ordering. Current updates intentionally limit
  catch-up; one huge delta is not equivalent to repeated smaller updates. Define
  decision stops during both real-time approach and initiative, including explicit
  waiting. Record policy reaction timing so zero-delay play is identifiable.
- **Reproduction:** record initial scenario/builds, seeds, build/content identity,
  protocol/timing/policy versions, ordered actions with simulation timing, and
  outcomes. Replay reconstructs from the start; normalize process-local identities
  when comparing events/state. Define compatibility checks and mismatch reporting;
  do not promise identical outcomes across arbitrary versions or platforms.
- **Storage/performance:** retain contiguous bounded simulation storage. Consume or
  copy each borrowed event batch before the next session operation invalidates it.
  Define limits for requests, queued work, traces, trials, ticks, output, and workers,
  with explicit exhaustion/cancellation results. Prepare simulation buffers outside
  stepping; protocol serialization and report I/O are separate diagnostic work.
  Stream long traces rather than retaining unbounded histories in memory.
- **Automation:** deterministic local policies execute repetitive trials; an LLM
  can select experiments or individual actions without being required per turn.
  A thin Python MCP adapter wraps the runner; it owns no gameplay rules. Select and
  pin a compatible official SDK during 13.3 planning.
- **Presentation:** retain prepared-image rendering, discrete movement, and fixed
  1920×1080 presentation. Optional capture/replay uses the existing rendering path.
  No OS input injection, focus changes, or control of other windows.

These are proposed boundaries. Per-slice design must specify concrete APIs,
capacities, ownership, failure behavior, and validation before implementation.

## Slice 13.1: Headless arena sessions and reproducible play

**Deliver:** A headless executable that constructs existing arena scenarios,
observes state, submits checked gameplay requests, advances to bounded decision
points, and reports terminal outcomes. Scripted play must support real-time approach
and initiative, not only a fight prearranged by test-only state mutation. Record
and replay complete runs through the versioned JSON interface. Establish the
visibility contract and actionable command coverage for current arena play.

**Boundary:** No general tactical bot, batch-analysis suite, MCP dependency, or
mid-fight snapshot format. Initial party inputs reuse existing validated builds;
explicitly document supported levels and any setup limitations. Do not silently
extend the level-one arena scenario format to represent advanced live characters.
Replay does not implement slice 14's run saves or change the character library.

**Acceptance:** A script completes an encounter and a reconstruction produces
matching normalized gameplay events/final state. Compare selected action/timing
traces through headless and normal gameplay orchestration. Exercise invalid/stale
requests, reset lifetimes, malformed/version-mismatched input, visibility, movement,
impact/effect/cooldown ordering, victory/defeat, and bounded termination. Use
fixture-owned CPU tests and allocation guards; retain separate optional rendering
checks where the chosen visibility mode requires them. A clean headless target
must build/run without creating a window or baking graphical assets.

**Execution record:** Implemented, **Accepted and closed**, 2026-10-30. The user approved
implementation with “Implement the plan.” Both diagnostic and prepared CPU visual
modes, filtered observations/explicit inspection, level-one scenario starts, checked
commands, fixed-step advancement, strict recording/replay and a scripted client are
delivered. [Design](headless_design.md), [protocol/client guide](headless_protocol.md),
[verification/handoff](slice131_verification.md). The user accepted the slice with
“slice accepted, update the docs.” Recorded verification results and limitations
remain unchanged. 13.2 is next eligible for separate planning; 13.2/13.3 remain unstarted.

## Slice 13.2: Automated players and balance experiments

**Deliver:** A small set of understandable, repeatable policies (for example,
focus fire, conservative recovery/resource use, and aggressive spell use) using
the established action boundary. Define supported decisions for current arena
mechanics, including feature use and compound spell targeting; report unsupported
situations explicitly. Run party/roster/policy/seed matrices and retain trial
identity plus reproduction traces for failures and selected outliers.

Reports include trial counts, victory/defeat, casualties, remaining/consumed
resources, rounds, simulated duration, and wall-clock throughput. Separate
timeouts, stalls, invalid-policy actions, and unsupported runs from combat losses.
Report uncertainty for aggregate comparisons. Reuse seed sets across comparable
configurations without claiming different action sequences consume identical rolls.

**Boundary:** Policies are test players, not new enemy AI or optimal-play claims.
No LLM call per simulated action. Benchmark first; add bounded independent worker
processes only where useful. Do not promise a speed multiplier before measuring.
Full dungeon expeditions and rest pacing remain future work.

**Acceptance:** One command compares several party compositions, including a
party without mages, across repeatable trials and emits a report with reproducible
individual runs. Use comparable level/equipment and more than one policy; label
information/reaction assumptions. Test policy competence on representative healing,
targeting, resource and feature situations; test report aggregation, denominators,
failure classification, repeatability, and cancellation. Keep a small routine
matrix separate from larger statistical/performance runs. Report policy coverage
limitations so strategy defects are not presented as game-balance evidence.

**Execution record:** Not started. Detailed implementation plan, verification,
and user acceptance pending.

## Slice 13.3: MCP play and investigation

**Deliver:** A thin local MCP adapter exposing session start/reset, observations,
eligible actions, action submission, bounded advancement, event inspection, batch
launch/status/cancellation/results, and replay inspection. Use the established
runner and reports; avoid a second command or rules implementation. Support
application-owned captures of selected replay points through the existing optional
rendering path. Keep graphics unavailable errors distinct from simulation failures.

**Boundary:** Local process integration first; no hosted service, remote deployment,
or networking inside the frame loop. Long experiments use bounded jobs and polling
rather than an indefinitely blocking tool call. SDK/protocol work stays outside
simulation allocation guarantees. This slice does not add arbitrary checkpointing.

**Acceptance:** An agent can complete an arena encounter through tools, launch a
composition comparison, and reproduce an interesting trial. Test schemas, invalid
commands, process/session lifecycle, stale sessions, tool errors, job limits and
cancellation, and equivalence with direct runner requests. Verify selected captures
when graphics are available without desktop input automation; explicitly report
unexecuted checks. Document installation, invocation, and a concrete review recipe.

**Execution record:** Not started. Detailed implementation plan, verification,
and user acceptance pending.

## Extension policy and deferred work

For later gameplay features, assess command/observation/event coverage and add
replay cases and relevant policy behavior alongside the feature. Keep diagnostics
separate from player knowledge; explicit unsupported coverage is preferable to
misleading results. Avoid forcing every subsystem to know about automation.

Deferred: arbitrary mid-fight snapshots, event-deadline skipping until equivalence
is demonstrated, sophisticated search/learned policies, full dungeon expeditions,
and balancing content not yet implemented. Slice 14 still owns player-facing
out-of-combat save/resume. Arena automation can later exercise those operations
through their normal interfaces.

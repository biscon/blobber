# Live diagnostics and encounter reconstruction: delivery roadmap

Created: 2026-11-11. Last updated: 2026-11-11.
Status: **Deferred roadmap; LD01–LD04 Planned. No active slice.**

**Scheduling instruction:** The user requested this document for later work and
explicitly said other tasks will come first. Finding or reading this roadmap does
not select it as the current task. Do not start planning a slice, implement it, or
run diagnostics merely because a new agent context opens. Wait until the user
explicitly selects this work. No date or automatic resumption is scheduled.

This document follows the tracking and review structure of the
[campaign roadmap](procedural_campaign_plan.md). It records four proposed agent
implementation slices, each intended to fit a substantial task invocation with
its own design, implementation, verification and handoff. These are scope estimates,
not guarantees about model calls or context length. Integration corrections may
need another invocation within the affected slice; no fifth feature slice is
automatically authorized. No humans are expected to implement the code; user review
and acceptance remain separate from agent implementation.

## Current checkpoint

| Field | Current value |
| --- | --- |
| Existing foundation | Shared GameSession, owned headless tests, JSON runner/client, strict replay, local MCP adapter and hidden replay captures |
| Active slice | None; this roadmap is deferred |
| Awaiting user review | Proposed roadmap only; no delivered feature or detailed slice plan |
| Assigned next task | None from this document; the user's other work takes precedence |
| Entry point when explicitly selected | LD01 detailed planning against the then-current code |
| Implementation authorization | None; this request authorizes writing the roadmap only |
| Blocking technical uncertainty | Complete mid-encounter state inventory and continuation parity must be established in LD03 |

Update this checkpoint, tracker and the affected execution record together when
work is explicitly started. Do not infer acceptance from a passing test, a roadmap
discussion, or a later unrelated request.

## Intent and boundaries

Let an agent inspect the game the user is actually playing, then reproduce its
encounter in an isolated headless process. The user supplies the symptom and
expected behavior; diagnostics supply authoritative positions, awareness, turn
ownership, timing and recent decisions. The intended investigation workflow is:

1. Request a consistent diagnostic bundle from the running game.
2. Inspect the state, matching screenshot and recent history.
3. After LD03/LD04, restore the encounter in an isolated runner and investigate
   through the existing checked actions and explicit time advancement.
4. Reduce the failure to an owned regression fixture, implement the correction,
   and validate the original behavior and affected callers.

The live endpoint exposes inspection and capture only. It does not move, attack,
cast, inject input, change the user's state, remotely pause/step the game, or offer
an arbitrary state editor. Ordinary gameplay remains under the user's control.
Commands used for reproduction operate on the isolated headless copy. No hosted
service, general telemetry platform, dashboard or new gameplay mode is required.

Preserve prepared-image rendering, fixed 1920×1080 presentation, discrete movement,
existing save safety, campaign progression and current combat rules. Diagnostic
checkpoints are separate from player saves; they must not enable mid-combat saving,
overwrite a campaign, or weaken existing save validation. No artwork is required.

### Three different artifacts

| Artifact | Purpose and guarantee |
| --- | --- |
| Inspection snapshot | Readable state at one consistent boundary. Useful for diagnosis; does not claim resumability. Delivered in LD01. |
| Recent history | Bounded observations of what led to that state. Includes declared gaps/evictions; does not claim complete replay from game launch. Delivered in LD02. |
| Encounter checkpoint | Versioned logical state sufficient for supported simulation continuation, with checked ownership and compatibility. Delivered and tested in LD03, integrated into tooling in LD04. |

A screenshot shows the live presentation, including current UI where available.
Headless restoration guarantees simulation state for the declared supported scope;
it does not promise to restore cursor position, held physical keys, GPU objects,
audio playback or every UI animation. Simulation-affecting pending commands and
transitions cannot be discarded as presentation details.

## How agents must use this document

1. Apply the scheduling instruction above. Only after the user selects this work,
   read [AGENTS.md](../AGENTS.md), [system design](system_design.md), this checkpoint,
   and any preceding accepted slice's handoff. Inspect current code and preserve
   existing working-tree changes.
2. Work on one selected slice at a time. Mark it **In progress — planning** and
   record a practical design before architecture changes. Put a detailed design
   under `docs/` when warranted and link it from the system overview and tracker.
3. Obtain review of the slice plan before implementation, following the existing
   roadmap convention. Explicit authorization already given in the conversation
   suffices; do not ask again for authorized work.
4. Implement the approved scope through shared engine facilities. Do not introduce
   alternate AI, combat, serialization policy or a second gameplay command system.
5. Run focused checks, record exact commands/results and material limitations,
   and update the design, checkpoint, tracker and execution record together.
6. Stop at **Ready for review**. Only explicit user acceptance closes a slice;
   do not automatically begin the next one. Corrections remain in their slice.

Status meanings match the campaign roadmap: **Planned**, **In progress** (name the
phase), **Blocked** (state the dependency), **Ready for review**, **Accepted**, or
**Superseded / Retired**. Accepted historical automation and campaign slices stay
closed. No commits, package installation, agent configuration changes, balance
experiments or live endpoint activation are implied by this roadmap.

## Current engine findings and proposed responsibilities

Inspected 2026-11-11. Recheck these findings when work begins; later feature work
may add state or change ownership.

| Existing facility | Integration consequence |
| --- | --- |
| [GameSession](../src/game/session.hpp) and [gameplay orchestration](../src/app/gameplay.cpp) | The real game and headless tests share simulation code. Live capture belongs at an application boundary where updates and player-command submission are complete. Keep IPC/export implementation outside the main gameplay loop. |
| [Observation encoder](../src/automation/observe.cpp) | Already exposes substantial diagnostic actor, AI, effect and party state. Reuse shared queries/encoding where suitable; additional combat membership, initiative cursors and decision causes need explicit diagnostic coverage. |
| [Floor export/import](../src/game/session_transfer.cpp) and [campaign checkpointing](../src/game/campaign_checkpoint.cpp) | Floor state includes RNGs and many subsystem values, but restore deliberately requires safe exploration. It is not an arbitrary combat checkpoint, and simply relaxing its phase check is insufficient. |
| [CombatController](../src/game/combat.hpp) | Pending impacts, initiative order/cursors, membership/surprise, round deadlines, active movement budgets and action continuations have state beyond CombatSnapshot and the existing Saved record. Inventory them explicitly. |
| [Ordinary navigation](../src/game/navigation.hpp) and [tactical navigation](../src/game/tactical_navigation.hpp) | Pending work affects which update can move an actor. Rebuilding a search may preserve the destination while changing timing; exact continuation must account for active searches/routes and bounded work. |
| [Replay](../src/automation/trace.hpp) and [headless design](headless_design.md) | Existing strict replay reconstructs from a recorded start; it does not import arbitrary live checkpoints. It uses a fixed stepping contract, while normal gameplay receives frame deltas. Preserve existing replay semantics. |
| [Replay captures](../src/automation/capture.cpp) | The renderer and presenter already export application-owned screenshots. Reuse that capability for the existing live frame rather than recreating a diagnostic scene. |
| [MCP adapter](mcp_design.md) and [session adapter](../tools/mcp_adapter/sessions.py) | MCP currently owns separate headless children. Add a small local inspection client; do not pretend those handles refer to the user's running graphical game. |

### Ownership, consistency and maintenance

Subsystem owners provide typed diagnostic/checkpoint records. Shared export/import
code and normalized comparisons should consume those records where practical;
MCP forwards requests and artifacts without owning another state schema. Extend
the owning records and coverage when new simulation state is added. Avoid both
unrestricted private-member dumps and a speculative reflection framework.

The game thread is the sole authority for reading mutable live simulation state
and capturing its corresponding presentation boundary. Transport workers may
exchange bounded requests and write already-owned output, but never walk mutable
game objects or call graphics APIs concurrently. Requests must either complete at
a defined boundary or return an explicit busy/unavailable/timeout result; never
silently wait for combat to end and capture a different situation.

LD01 chooses the smallest suitable local transport, endpoint discovery and instance
identity scheme. Prefer a per-user local endpoint, explicitly enabled for diagnostics,
with no network listener or automatic attachment to an arbitrary instance. Define
restart, level-transition, shutdown, stale-request and multiple-instance behavior.
The CLI must work without the optional MCP SDK. Normal startup must not launch
another game, an experiment, or an asset bake to answer discovery.

Diagnostic capture may allocate and perform I/O in an explicit export phase.
Steady-state collection uses prepared bounded storage; record typed events rather
than allocating strings or serializing JSON during frames. Detailed designs set
request, event, memory, byte, file and duration limits and exhaustion behavior
before implementation. Never block the game loop on a slow reader or unbounded
queue. Publish complete bundles transactionally; partial output is labelled failed.

### Continuation and compatibility

Use logical values and checked references, never process-memory images, raw pointers
or serialized GPU resources. Retain generations and remap world/session identities
consistently across actors, effects, pending attacks, items and campaign ownership.
Construct and validate a replacement before publication; failed import must leave
the live game and any existing headless session unchanged.

Record bundle/checkpoint schema, simulation/content/timing identities, source build
and relevant world definitions. The graphical application and headless executable
are different binaries: compatibility needs a shared simulation/content identity,
with both executable identities retained for provenance. Do not require their
whole-file hashes to be equal or relax existing strict trace identity checks.

Exact continuation means matching normalized state and gameplay events when both
instances receive the same subsequent engine operations, ordering and time deltas.
It does not mean equal results under arbitrary frame rates or replacing the live
frame schedule with fixed 10 ms ticks. LD03 defines the in-process oracle; LD04
defines tooling support for its recorded operation/timing schedule while retaining
the ordinary runner's fixed-step mode.

Declare the supported reconstruction boundary: the active arena or campaign gameplay
session, including its authoritative campaign context needed for progression, travel,
rewards and inactive floors. Menu/loading/transition states outside that boundary
remain inspectable where possible but must reject exact import with a specific
reason until supported. Do not silently flatten a campaign into an arena, cancel
pending attacks, clear navigation, reset initiative, or erase an unsupported effect.

Reject incompatible/corrupt/incomplete checkpoints explicitly. No general migration
or cross-platform exactness promise. Continuing under a changed build after a fix
is distinct from verifying reproduction under the original compatible build;
report provenance and compatibility, never label divergent builds exact replays.

## Slice tracker and milestones

Dependencies describe order only after the user elects to start this roadmap.

| ID | Slice | Prerequisite | Status | Milestone |
| --- | --- | --- | --- | --- |
| LD01 | [Live inspection and matching captures](#ld01-live-inspection-and-matching-captures) | Explicit user selection and reviewed detailed plan | Planned — deferred | LD-A |
| LD02 | [Bounded recent diagnostic history](#ld02-bounded-recent-diagnostic-history) | LD01 accepted | Planned | LD-A |
| LD03 | [Complete encounter checkpoints](#ld03-complete-encounter-checkpoints) | LD02 accepted | Planned | LD-B |
| LD04 | [Headless import and investigation workflow](#ld04-headless-import-and-investigation-workflow) | LD03 accepted | Planned | LD-B |

| Milestone | Acceptance experience | Status |
| --- | --- | --- |
| LD-A — Inspect the actual problem | Agent requests a live bundle and explains a symptom from matching state, screenshot and recent history, without controlling gameplay. | Planned |
| LD-B — Reproduce in isolation | Agent restores the encounter in a separate headless process and demonstrates matching continuation under controlled operations/timing. | Planned |

### LD01: Live inspection and matching captures

**Deliver:** A consistent structured inspection snapshot and matching native game
screenshot, a small local read-only endpoint/client, explicit instance discovery,
and thin MCP access. Include map/geometry, party/resources, actors/subcells,
awareness/last-seen timing, encounter membership, initiative and action state,
effects and relevant clocks. Distinguish authoritative state from presentation and
known/player-visible information. Diagnostics are explicitly privileged.

**Boundary:** No remote gameplay or pause/step commands, history recording,
resumability claim or player-save change. The game continues under normal user
control; extraction occurs at a consistent boundary without advancing simulation
on behalf of the diagnostic request. A short export stall is diagnostic work and
must be bounded and measured, not described as free.

**Acceptance / validation:** Owned CPU fixtures check snapshot consistency,
identity/revision admission, stale/busy/timeout/shutdown failures, bounded requests
and unavailable graphics. Reading state does not consume RNG, mutate gameplay or
advance clocks. Hidden application tests verify screenshot/state boundary metadata
and native rendering, with no OS input/focus automation. Verify ordinary gameplay
with diagnostics disabled and engine-owned frame allocation behavior when enabled.

**Review recipe:** User runs the diagnostics-enabled game and names the instance;
agent requests its bundle without touching input. Show which enemies are aware,
enrolled and still due to act, alongside the matching screenshot. Use a scripted
owned instance for automated review; human live-session review remains separate.

**Execution record:** Planned; no detailed design, implementation or verification.

### LD02: Bounded recent diagnostic history

**Deliver:** A preallocated typed event ring recording accepted/rejected gameplay
commands, relevant simulation timing, phase/turn transitions, awareness changes,
enrollment, movement/path decisions and action/impact outcomes. Preserve useful
reason codes at their actual decision points; the existing combat log alone does
not explain why an enemy failed to enroll. Bundle history atomically with LD01's
snapshot and capture boundary.

**Boundary:** Recent history is evidence, not an unlimited command journal or a
promise to replay from launch. Avoid per-frame full snapshots, redundant position
spam and broad logging frameworks. Declare history coverage, capacity, oldest/newest
sequence, dropped records and activation/reset points. Do not hide gaps or infer
events from screenshots. History cannot retrospectively recover an event from
before diagnostics were enabled or after it was evicted.

**Acceptance / validation:** Reproduce a small doorway scenario and distinguish
never alerted, alert expired, awaiting enrollment, turn already spent, blocked path
and full-front cases. Test wraparound, event ordering at equal timestamps,
world/instance changes, export while the ring is full, and explicit gap reporting.
Ensure warmed recording allocates no engine C++ memory and does not change
gameplay/RNG outcomes compared with recording disabled.

**Review recipe:** Inspect a doorway bundle and trace one hidden enemy's alert,
enrollment and movement opportunity. Show a deliberately evicted-history case so
the diagnostic explanation acknowledges the missing interval.

**Execution record:** Planned; LD-A is usable after this slice is accepted.

### LD03: Complete encounter checkpoints

**Deliver:** A subsystem-owned state inventory, versioned diagnostic checkpoint,
and checked C++ capture/restore for the supported arena/campaign gameplay scope.
Capture at an outer simulation boundary even during initiative, movement, attack
windup/projectile flight or pending navigation. Include all state affecting future
simulation: RNG streams, timers/remainders, cursors, membership, surprise, pending
impacts/casts and reservations, active movement budgets, AI/navigation continuation,
conditions/areas, party/inventory, loot/reward ledgers, fixtures and campaign context.
Identify safely derived values and rebuild their caches deliberately.

**Boundary:** Separate from ordinary save/load and its safe-exploration format.
No automatic migration, pointer dumps, restoring into the user's live process,
or claiming exactness from a visually similar reconstruction. Explicitly document
excluded UI/platform state and reject unsupported simulation states. Split LD03
with a reviewed suffix if the inventory exceeds one practical invocation; do not
quietly drop a subsystem to preserve the four-slice estimate.

**Acceptance / validation:** Capture owned sessions at selected boundaries, restore
independent instances, then apply identical operations/time deltas and compare
normalized events and state after each operation. Cover turns/round-end/facing,
movement arrival, a pending path search, accepted attack before impact, conditions,
area hazards, concentration and once-only rewards. Include a fractional-time
remainder and a nonuniform update schedule. Validate reference remapping, truncated/
corrupt/version-mismatched input, failed-import isolation, and proper resource
lifetimes. Existing player saves retain their safety/roundtrip behavior.

**Review recipe:** Restore three small cases: doorway reinforcements, an attack
before impact, and pending movement/navigation. Compare original/restored
continuation, then deliberately omit or alter a consequential field and demonstrate
that validation or the continuation oracle catches it.

**Execution record:** Planned; complete C++ checkpoint behavior precedes public
headless import. Detailed ownership/completeness design is the largest uncertainty.

### LD04: Headless import and investigation workflow

**Deliver:** Diagnostic-bundle import in the runner/client and thin MCP forwarding,
using LD03's checked importer and the existing gameplay action interface. Start a
fresh isolated session; identify its source instance/boundary and compatibility.
Support exact reproduction under a recorded operation/timing schedule separately
from ordinary fixed-step exploration of the restored encounter. Record new traces
rooted in the imported checkpoint with normalized identity/mismatch reporting.

**Boundary:** No live gameplay control or second action implementation. Retain
legacy scenario starts, safe-save starts and strict replay behavior. Runtime worlds
and pending references must belong to the new isolated instance. A read-only live
inspection request must never cause headless simulation to start implicitly.

**Acceptance / validation:** Exercise live export to a fresh headless process and
prove matching initial state and controlled continuation. Cover arena and active
campaign context, including a subsequent reward/travel interaction, incompatible
content/builds, absent graphics, stale identities and transactional replacement
failure. Verify CLI and MCP agree on the same bundle and failures. Confirm new
traces replay with declared compatibility and existing traces retain their contract.
Use a scripted application-owned export for automated graphical integration;
report human live-session review separately.

**Review recipe:** User points out an encounter symptom; agent reads the matching
live bundle, imports it, advances the isolated copy and explains the relevant state
transitions. Demonstrate that the original running game receives no gameplay
commands. Preserve a minimal owned regression rather than making a user save a
permanent test dependency.

**Execution record:** Planned; LD-B acceptance requires the complete workflow,
not only successful JSON loading or matching screenshots.

## Verification, artifacts and risk management

Follow [verification workflow](verification_workflow.md): focused CPU checks in one
configuration, at most two low-priority build workers and one intensive job at a
time. Keep contract checks small; no default benchmarks, balance matrices or broad
seed sweeps. GPU integration/captures are separate, bounded and application-owned;
unavailable checks are unexecuted. No OS mouse/keyboard/focus/desktop automation.

Every implementation slice creates an owned agent run for output. Promote only
selected review images under the existing retention limits, record commands/results
in its relevant design/handoff, and preview/apply cleanup before handoff. Preserve
the user's saves and unrelated runs. Runtime diagnostic exports need a bounded
output root, explicit ownership and cleanup policy established in LD01; they must
not silently accumulate in `docs/`, source assets or the repository root.

The main risks are omitted hidden state, mismatched timing, exporting across a
session transition, and making diagnostics alter the behavior being investigated.
Per-owner records, boundary identities, continuation tests and allocation checks
address these directly. A checkpoint format does not eliminate maintenance:
features adding consequential state must update its capture/restore and add a
representative continuation case. Keep normal save contracts independent.

## Execution-record template and deferred handoff

Use this template in the selected slice's detailed design or execution record:

```markdown
**Stage / last updated:** Planning, implementing, reviewing or accepted; YYYY-MM-DD.
**Approved plan:** Link/summary and explicit user authorization.
**Delivered:** Observable behavior, ownership and relevant entry points.
**Verification:** Exact commands/results; failures, skipped and unavailable checks.
**Review recipe:** Setup, steps and expected results.
**Deviations / limitations:** Supported capture/restore states and remaining gaps.
**Next-agent handoff:** Formats/identities, maintenance obligations and selected next action.
**Acceptance:** User statement/date, or explicitly pending.
```

**Current handoff — 2026-11-11:** Documentation only. The user requested the roadmap
for later, with other work first. All four slices remain Planned; no implementation,
endpoint activation, capture, test run or slice acceptance has occurred for this
feature. This document is indexed beside existing automation tooling. When and
only when the user chooses to start it, recheck the current code and plan LD01.

### Reusable prompt for explicitly starting this roadmap later

> I want to start the deferred live diagnostics roadmap now. Read AGENTS.md,
> docs/system_design.md and docs/live_diagnostics_plan.md. Inspect the current
> checkpoint and relevant code, preserve working-tree changes, and plan LD01
> only for review. Do not implement or change my running game.

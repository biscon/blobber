# Combat positioning for waiting enemies

Status: implemented and mechanically verified (2026-11-03); interactive acceptance remains user review.

The reproduced flank goblin received no moves or attacks over six rounds, although
direct approach calls reached the central front in two moves. The previous combat
loop filtered turns by frontal visibility and available local melee rank; round-end
roaming then stopped adjacent pursuers. Turn eligibility is now independent of
attack geometry: every living enrolled enemy receives its initiative opportunity,
with visibility/range/LOS/rank still required for attacks and spells. Unknown
participants remain absent from the read-only turn preview. All enrolled enemies
are frozen during round-end roaming to avoid extra moves or condition actions.

Enemies with a usable attack position hold it. Waiting enemies route into visible
near-row slots across the three front cells, then visible rear staging slots, then
ordinary adjacent contact if the front is inaccessible. Keep the existing paired
in-cell advancement/attack action. Other positioning steps, including repositioning
within a cell, spend a movement turn and never attack in the same turn. Preserve
caster/ranged firing policies, wounded retreat, surprise and condition costs.

## Responsibilities and interfaces

A small shared approach-slot query ranks a legal destination's free subcells:
visible near-row, visible rear-row, then hidden. It may include the moving actor's
own slot when evaluating arrival/holding. Within a category preserve the actor's
lateral lane and existing stable order. Navigation and movement commitment use
this query; combat retains ownership of attack protection and action rules.

Reuse Navigation's preallocated cell BFS, four-way edges, partial-cell occupancy
and existing 4096-node/eight-request update budgets. Preferred party approaches
evaluate the at-most-eight adjacent goal cells by slot quality, then holding the
current cell and centre preference. Finish when the best available goal is reached;
if it is inaccessible, select the best reachable goal from visited cells at search
exhaustion. The existing visited/first-step arrays prove reachability and supply
the first move; bounded goal queries need no new per-node or serialized storage.
Ignore old preferred fallback hints and revalidate live goals/next steps before
commit. Generic point/investigation/patrol routes retain their existing behavior.
No destination subcell is reserved by searching; ActorWorld checks commitment.

NpcAiController exposes a positioning result distinguishing Pending, Moved, AtGoal
and Blocked (difficult-terrain preparation counts as a movement action). The
existing bool approach entry point remains a compatibility wrapper. Pending search
work retains the active combat turn without consuming readiness or RNG; the next
update resumes within the usual budget. Blocked searches may spend the turn and
retry later. No positive result is inferred from adjacency alone. Same-cell
positioning uses ActorWorld walk interpolation; cell-entry effects only run when
crossing a cell boundary. Facing follows the actual subcell displacement.

Combat schedules living members independently of geometry. It attacks from a
usable current position, otherwise requests positioning before wasting an action.
Normal movement charges readiness once and holds presentation until arrival.
Preview lists scheduled known enemies without path searches or mutations. Existing
automatic facing/disengagement, encounter enrollment and awareness are unchanged.

Storage ownership, capacities and checkpoint shapes remain unchanged. Additional
work is fixed adjacent-slot scans plus the existing bounded BFS; no frame allocator,
half-cell path grid, formation manager, world-sized scratch expansion, or new save
format. Current schema/simulation remain 7/11; strict traces retain binary identity.

## Verification strategy

Owned no-damage/high-HP fixtures cover the six-round stall; four headings/both
flanks; hidden diagonal slots; blocked local rank with remote vacancies; detours
around full cells; partial cells; competing claims; same-cell motion; vacancies
after death; full/unreachable fronts and genuine chokepoints. Assert eventual
arrival/attack, occupied-slot uniqueness, stable attackers, one action per round,
no offscreen attacks and no round-end member movement. Keep handoff tests meaningful
when enemies can legally relocate. Exercise pending searches, pause, difficult
terrain, statuses, ranged/caster policies and restored navigation continuation.

Run AI/combat/hybrid first, then affected actors/effects/targeting/allocation/
automation/persistence callers once in Release. Verify a hidden scripted encounter
using generated mechanics and production artwork; no OS input/focus automation.
At most two low-priority build workers and one intensive job; no benchmarks or
full build/test matrix. Update these notes with actual checks and limitations.

## Delivered behavior and verification

`approach_slots` owns slot quality/selection; Navigation evaluates reachable goal
cells using it and its existing first-step workspace. The preferred approach no
longer accepts an unusable adjacent root as arrival while better slots are reachable.
`NpcAiController::position` distinguishes pending work from a spent move, a stable
hold and a blocked result. Combat's private enemy-turn call explicitly reports
pending navigation to its scheduler; no attack/error reason is repurposed for it.
The bool approach wrapper, serialized navigation layout and six-bytes-per-cell
workspace remain compatible. Legacy checkpoint fallback hints are retained in the
record but not trusted for preferred goal selection.

Every living enrolled member receives an initiative opportunity. A usable current
melee position attacks before route selection; ordinary movement/preparation spends
its action. All members are excluded from round-end roaming and duplicate recovery/
ground checks. Preview shows known scheduled members, including waiting actors,
without searching, reserving destinations or disclosing unknown actors. `ActorWorld`
walks interpolate same-cell subslot changes using existing travel records; ordinary
checked `move` still supports the paired advancement/attack rule.

Release application/headless, diagnostics and the selected tests built with at
most two low-priority workers and no compiler warnings. **21/21** affected suites
passed in **7.90 s**, including the separately labelled campaign-save integration
at 5.76 s: `combat_positioning`, `targeting`, `shaman`, `zombie`, `giant_rat`,
`bugbear`, `animation`, `expanded_spells`, `spells`, `effects`, `impact`, `hybrid`,
`ai`, `combat`, `commands`, `actors`, `automation`, `automation_allocations`,
`campaign_save`, `world_visibility`, `world_visual_cli`. After adding the final
same-cell hazard test, only `combat_positioning` was rebuilt and repeated; it passes.

The dedicated positioning tests verify both flanks/four headings, blocked melee-only
rear actors, detours initially away from the party, same-cell registered feet,
single actions/round, stable attacking anchors, no member movement at round end,
live vacancy competition, work-budget pending turns, pause and exact preferred
search checkpoint continuation. Allocation guards cover production turn/update
paths; existing broader guards also pass. Difficult-terrain preparation and 1200 ms
same-cell travel do not repeat cell-entry hazard saves. This is an engine C++
allocation claim, not a statement about graphics drivers or other libraries.

Older tests were adjusted to preserve their purpose under the new behavior: rank
advancement uses a real cross-shaped chokepoint, previews include waiting members,
front-row arrival prefers diagonal attack positions over centre rear staging, and
the stationary diagonal handoff arrangement is re-established after testing legal
movement. The seeded initiative/Defend oracle waits for an actual attack and stops
at RoundEnd, preserving its RNG/order checks without assuming every enemy attacks.

[Six inspected native captures](screenshots/combat-positioning/README.md) show a
fixed-camera four-goblin encounter: two initially visible enemies hold the centre,
both offscreen flankers walk into diagonal openings, and both attack on later turns.
The final frame has four CPU-picked bodies. The hidden diagnostic verifies GL
errors, movement/attack participation, stable camera/anchors and resource release
on NVIDIA RTX 3060/OpenGL 3.3. Safe generated mechanics use production world/goblin
art and placeholder party identities; no campaign save is loaded or changed.
Interactive gameplay/balance review remains unverified; no benchmark, stress run,
sanitizer or additional configuration matrix was run.

Logs: `/tmp/combat-positioning-{final-build,tests,hazard-build,review}.log`.

## Arrival facing follow-up — 2026-11-04

Implemented and verified: combat action completion turns a
living, able-to-act enemy with completed travel in the melee front toward the
current party before handing off the turn. Keep travel-facing through the final
walk frame. This uses the existing completion boundary and checked ActorWorld
facing mutation, with no arrival queue, extra action, RNG, frame allocation or save
state. Dead/incapacitated actors and arrivals outside the front are not redirected.
Owned regressions cover cell/subcell movement in all four headings, the last
millisecond, pause, exact arrival before any attack, unchanged round/gameplay clock
and zero C++ allocations. Positioning, combat and hybrid checks pass. One fixture
initially required every travel direction to differ from the arrival direction;
some diagonal moves already face the party, so that precondition was corrected
while retaining the arrival/timing assertions and requiring actual direction changes.

Release game and diagnostic are rebuilt. The existing hidden positioning review
passes with new [arrival captures](screenshots/combat-arrival-facing/README.md);
both inspected arrival frames face the party before those enemies attack.
GL error and resource-release checks pass on RTX 3060/OpenGL 3.3. No broader
configuration matrix or interactive desktop review. Logs are
`/tmp/combat-arrival-facing-{build,test-build,review}.log`.

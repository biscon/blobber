# Combat positioning for waiting enemies

## SRD combat movement — 2026-11-08

Status: implemented, mechanically verified and accepted by the user (2026-11-08).
The user reports better encounter pressure. Approved design was recorded before implementation.
This revision supersedes the movement-spends-action rules below.

Monster definitions own immutable integer `speedFeet` (nonnegative int32); catalog
schema 14 requires `speed_ft`, while older catalogs explicitly default to 30 feet.
Zombie is 20; other shipped species are 30, including authored goblin variants and
shaman. Each cardinal subcell step costs five feet. No wolf content is added.
Real-time roaming/pursuit, party combat movement lock and attack geometry remain
project adaptations. Arena setup exposes Speed and subcell allowance.

CombatController owns one transient active-enemy turn record: initialization,
movement spent, action spent, Dash and movement presentation. Selection resets it;
movement arrival resumes the same turn without advancing initiative or readiness.
Attack/cast completion ends the turn. Casting and existing ranged/retreat priorities
remain; healthy mixed attackers approach before fallback shots. A legal attack
position stops approach. Dash spends the action once for another current Speed,
only when ordinary movement cannot produce a usable action and a route progresses.
A retreat prefers reachable visible firing positions and stops when separation
is established, then takes its shot if legal. Active movement/action continuation
defers automatic facing/disengagement until the turn finishes. No discretionary
post-attack movement or bonus-action Dash is added.

NpcAiController owns a separate reusable TacticalNavigation workspace. Incremental
Dijkstra searches four cardinal nodes per dungeon cell with five/ten-foot terrain
edges. Preallocated distance, parent, indexed binary heap, route and generation
arrays use 24 bytes/subcell plus bounded metadata: one workspace for the active
session, rather than one per creature, with no frame allocation. Generations avoid
whole-map clears per turn; generation exhaustion clears incrementally. Search/reset
work is capped at 4096 nodes per update. Tactical and ordinary navigation run in
mutually exclusive simulation phases, sharing that per-update allowance; at most
one tactical step commits per update.
Goals preserve visible-near, visible-rear, hidden priority, cheapest route within
rank and stable ties. Occupied subcells/party cell block movement; cross-cell edges
use existing wall/elevation rules. Ally traversal and diagonals remain deferred.
Cached routes revalidate geometry, occupancy and ground revisions and each next
edge. Pending search does not consume RNG, movement or the action; a failed search
ends that turn's movement. Half the existing whole-cell walk duration presents each
step; there is no extra action delay between steps. Cell-entry effects run only
on cell boundaries.

Combat movement separates Speed from terrain cost: Ray of Frost subtracts ten
feet (minimum zero); destination difficult terrain doubles step cost without
stacking; restrained creatures have zero movement. Standing costs half current
Speed (integer feet, SRD rounding), retains the action, and requires positive
Speed and sufficient movement. Party standing also retains its action, but party
translation stays locked. Escaping Web spends the action and successful escape
allows remaining movement. Hazards/impacts recheck consciousness and mobility;
knockdown stops further travel until standing is affordable. Surprise/incapacity
preclude normal movement/actions. Existing real-time condition adaptations remain.

New movement/search records are transient: safe saves already exclude encounters.
Campaign schema remains 16, simulation becomes 20, and changed catalog fingerprint
rejects old campaigns unchanged. Arena scenarios retain their format and content IDs.

Validation uses owned 20/30/40-foot fixtures; exact budgets, movement-to-attack,
Dash exclusion, detours/occupancy/headings, conditions/entry hazards, pause and
readiness, pending search and allocation guards. Run focused CPU suites in one
configuration, affected callers/persistence once, and a hidden production-art
walk/attack diagnostic. Keep owned output in an agent run, promote at most six
review captures and clean disposable output. Interactive acceptance belongs to
the user; no desktop input or focus automation.

### Delivered verification

Both Release and CLion Debug games build without compiler warnings. Release was
the development/test configuration; Debug was built for the user's normal launch,
without repeating a test matrix. Resource-intensive jobs used low CPU/I/O priority,
at most two compiler workers and one test process; GPU library threads were bounded.

The final affected Release run passed **20/20 suites in 3.77 s**:
`combat_movement`, `combat_positioning`, `rules_content`, `ai`, `hybrid`, `combat`,
`commands`, `actors`, `effects`, `impact`, `spells`, `expanded_spells`, `shaman`,
`zombie`, `giant_rat`, `bugbear`, `arena`, `automation`, `automation_allocations`,
and `gameplay_startup`. Build their corresponding `blobber_*_tests` targets, then:

```sh
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-release -j 1 --output-on-failure \
  -R '^(combat_movement|combat_positioning|rules_content|ai|hybrid|combat|commands|actors|effects|impact|spells|expanded_spells|shaman|zombie|giant_rat|bugbear|arena|automation|automation_allocations|gameplay_startup)$'
```

The separately labelled extended `campaign_save` suite passed in **7.27 s** during
the integration checkpoint, including unchanged old-save rejection and new-save
roundtrip. It was not repeated after transient retreat/presentation corrections.
The new simulation-19 rejection has no implicit migration; shipped source, Debug
and Release catalogs all publish schema 14 and identical explicit creature speeds.
Arena reload/rematch uses unchanged scenario schema/content IDs. No user saves or
scenario files were modified.

Owned movement tests cover 0/1/20/25/30/40-foot allowances, ordinary-versus-Dash
boundaries, exact cardinal displacement, normal arrival plus attack, Dash arrival
without attack, frost and terrain both separately and combined, one cell-entry
hazard check plus one end-turn check, standing with/without sufficient Speed,
party stand-and-act, failed/successful Web escape, sleep, bounded pending searches,
unchanged turn tokens/round clocks and engine-owned C++ allocation guards. Existing
heading/flank, detour, blocked-rank, pause, stale request and navigation continuation
checks pass. This is not a no-allocation claim about drivers/third-party internals.

The opt-in hidden diagnostic passed with production art, no GL errors and matched
world GPU resource counts after release:

```sh
env OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 LP_NUM_THREADS=2 nice -n 15 ionice -c 3 \
  cmake-build-release/blobber_world_integration_gpu_tests cmake-build-release/assets \
  RUN_DIRECTORY/captures --combat-positioning-review
```

Its assertions bind each flanker attack to the same round **and turn token** as
movement; six captures were inspected and replaced the previous
[positioning review evidence](screenshots/combat-positioning/README.md).
The sandbox could not access X11; the authorized hidden-only check passed outside
it. No OS input/focus automation was used. The user accepted the movement revision
after their saved-arena playthrough on 2026-11-08; no benchmark, stress test or
sanitizer matrix was run.

Development failures exposed historical movement-spends-action assertions and a
same-timestamp continuation boundary; tests now assert legal geometry, one action,
subcell arrival and retained turn ownership. A pre-existing generated-appearance
fixture repeated names after eleven types despite thirteen definitions; its owned
name table now includes blacksmith and Shadowtart. Allocation integration passes.

Disposable logs/captures were owned by an agent run; the selected six images replace
superseded evidence, and the run was cleaned after runners closed. User acceptance
was recorded on 2026-11-08.

## Previous implementation — 2026-11-03

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

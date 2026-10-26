# Mutable monster world — slice 03

Implemented slice 08 extension: [shared appearance rules variants](hybrid_encounter_design.md#single-weapon-review-correction). Programmatic placements may carry an optional catalog-local MonsterId. ActorWorld validates it at construction; absent overrides retain appearance-name binding. All appearance defaults must still bind. The override is a small value, adds no runtime allocation, and does not change the JSON placement schema. Catalog lifetime must cover placements and their session.

Implemented slice 07 extension, accepted 2026-10-26: [moving goblin AI](goblin_ai_design.md).
ActorWorld adds a const subslot occupancy query and checked walk cue; AI remains a
separate session owner. Existing actor identity, occupancy and lifetime contracts remain.

Implemented slice 06 extension: [hurt/death presentation](goblin_reactions_design.md).
The original flash-only presentation below remains the compatibility path for missing
reaction art. Authoritative HP, occupancy, removal and identity contracts are unchanged.

Implemented extension: [slice 05 melee combat](melee_combat_design.md). Slice 05 consumes these checked operations from stationary combat and shares damage timestamps with session animation time. Occupancy/death/flash contracts remain unchanged. The remainder records this subsystem’s original slice boundary.

Status: implemented and accepted by the user (2026-10-25).
See [verification and review](slice03_verification.md).

## Ownership and identity

`NpcWorld` remains immutable placement/loading data. `ActorWorld` initializes
runtime actors from it and becomes the sole live occupancy authority. The
application owns the world, borrowing a map and immutable rules catalog that
outlive it and remain unmoved. The world itself cannot be copied or moved.
Appearance type strings resolve to catalog monster IDs at initialization; missing
bindings fail before scene preparation. Existing asset formats are unchanged.

Storage is 256 fixed records, 32 appearance bindings, and one preallocated byte
per map cell (four occupancy bits). Records contain appearance/transform/animation,
monster ID, current/max HP, lifecycle and last-damage presentation cue. Slots
never recycle or compact; there is no spawning. Actor IDs contain a slot and
unique world identity; removed, invalid and foreign IDs fail lookup. Dead records
remain inspectable but cannot move, turn or take further damage. World identities
are allocated only during initialization, never reused, and exhaustion fails.
Const snapshots are borrowed; callers cannot mutate records. Placement data and
snapshots are not persistent saves or transferable handles.

## Operations and data flow

Checked move, face, damage and remove operations return bounded result values.
Move validates a cardinal adjacent-cell step or same-cell subslot change, map
bounds/walls, subslot occupancy and the supplied current party cell before
committing both masks and transform. Same-position/facing requests are no-ops.
Party movement commits its logical destination at transition start; its old cell
is not reserved. Queued party moves recheck current runtime occupancy.

Nonnegative resolved damage clamps HP at zero using overflow-safe arithmetic;
zero damage is a no-op. Positive damage stores an explicit monotonic timestamp
for that actor's presentation cue. Earlier damage timestamps are rejected.
Death releases occupancy exactly once. Explicit removal clears occupancy, hides
immediately and invalidates lookup. Failed/no-op operations do not change the
revision. Mutations are single-threaded transactions, not concurrent operations.
No combat resolver, healing, rewards, AI or resurrection is added here.

Animation samples runtime slots after mutations on the same timestamp axis.
For 100 ms after damage, body pixels become white with unchanged alpha and wall
occlusion. Survivors keep animating; lethal hits hold their pose at hit time,
then hide body and shadow. Shadows are unchanged during the flash. Repeated hits
restart it; removal cancels it. Time jumps expire the effect without catch-up.
Without an animation snapshot only living actors render, using frame zero.
Animation snapshots identify their world and actor revision; stale/mismatched
snapshots are rejected rather than composing inconsistent state.

The renderer reads const runtime state and immutable prepared sprites; no rebaking
occurs. Frame invalidation observes world identity/revision plus movement and
animation revisions. Both transition endpoints use the same state. Ordinary
engine-owned frame processing and mutations allocate nothing; loading, diagnostics
and captures may allocate. O(1) occupancy/mutations and O(256) animation/visibility
scans retain the existing bounded workload.

## Validation and review

Generated catalogs/maps/images test binding and capacity failures, handles,
transactional movement, shared cells, party/queued movement, HP boundaries,
exactly-once lifecycle changes, revision reuse, flash timing and occlusion, and
all movement transitions. Extend the allocation guard through mutations and
playback. Run Debug/Release suites and separate available desktop/GPU checks.
Keep original capture names and pixels; append isolated scripted actor scenarios
for move/face/damage/removal and transition refresh. No interactive debug controls.

The approved user choices are placement-only actors, scripted review, and a
100 ms white flash on lethal damage before hiding, with immediate unblocking.
Hurt/death animation clips remain slice 06. Implementation was reviewed and accepted by the user on 2026-10-25;
slice 04 is eligible for its own planning and review.

## Integration contracts

Entry points are `src/game/actors.hpp` (owner and mutation results),
`NpcAnimator::setTime` (sample after mutations), and `FramePipeline::update`.
The renderer accepts a const actor owner and borrows its const snapshot; it does
not copy records or strings. Runtime C++ APIs formerly accepting `NpcWorld` now
accept `ActorWorld`; immutable placement loading/validation APIs remain unchanged.
All appearance type IDs, including unused declared types, must bind to catalog
monster IDs. No asset schema or catalog version was changed.

Application order is movement/commands, animation sampling, frame composition,
then presentation. Pass the controller's committed player cell to actor moves.
Damage timestamps and animation sampling use the same elapsed-millisecond axis;
normal playback is monotonic. `setTime` still permits deliberate diagnostic seeks,
which reconstruct visibility at that time. Independent capture scenarios create
fresh actor owners; handles must never be reused between scenarios or sessions.
Without an animation snapshot, dead actors hide immediately.

The normal application initializes actors but does not issue mutations yet.
Scripted captures and tests exercise the API; later command/combat slices must
call these checked operations rather than editing snapshots. No generic session
clock, RNG, event bus or animation scheduler was introduced.

## Slice 05 review: attack cues

ActorState additionally retains a checked attack timestamp independent of HP damage.
`attack(id, time)` rejects foreign/removed/dead IDs and timestamps older than that
actor's previous attack; it increments the actor revision. Combat emits the cue
for every attack, including misses. NpcAnimator selects one prepared attack clip
from elapsed cue time and then returns to the authored clip. See the
[melee design](melee_combat_design.md) for same-cell front-slot combat advancement and feedback.

## Slice 07 review: travel presentation

Checked cross-cell `walk` commits occupancy immediately, retaining one source
position and session timestamp per slot. `actorPosition` samples 600 ms linear
travel, includes subslot offsets, chains from the current visual position and
freezes on lethal damage. A successful position-changing `move` cancels the
segment. `actorWalking` lets contact wait for arrival without consulting graphics.
The animator publishes destination-relative offsets; revisions include position
changes even while a pose is held. See the [AI travel design](goblin_ai_design.md#review-correction-interpolated-walking).

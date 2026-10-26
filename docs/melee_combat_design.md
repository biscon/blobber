# First melee combat — slice 05

Implemented slice 07 review correction (2026-10-26): [attack presentation handoff](goblin_ai_design.md#review-correction-finish-attack-presentation-before-advancing).
Attack outcomes still apply immediately, but the acting turn remains locked until
its animation and damage/MISS marker finish. Victory/defeat overlays also wait
for an in-flight attack; the next automatic turn retains its 450 ms preparation
delay. This supersedes immediate post-attack turn selection described below.

Status: implemented and accepted by the user (2026-10-25), including combat
corrections and the dungeon-first UI. Slice 05 and milestone A are closed.
Verification and user review are recorded in
[slice 05 verification](slice05_verification.md) (2026-10-25).
The user approved the implementation plan with “Implement the plan.” Delivery
and acceptance belong to the roadmap, not the presence of this document.

## Gameplay and adaptations

Frontal cardinal contact starts a stationary encounter against one adjacent cell.
Contact at a committed movement destination clears pending movement before its
transition ends. Finish that transition before initiative. Movement, turning and
formation changes are locked during combat. There are no reinforcements, shared
maneuvers, retreat, side/rear threats, cross-cell AI movement, ranged attacks or spells.

Roll initiative once per character and monster-definition group. Ties use higher
Dexterity, party before enemies, then stable identity. Groups act in stable slot
order. Initiative numbers are hidden; round and active participant are visible.
Each conscious character gets one action. Automatic attacks and death saves are
spaced by 450 ms, at most one resolved automatic step per update, without catch-up.

Both sides use paired-slot protection: each conscious party front member shields
only the rear member in the same column (FL/BL and FR/BR). Each living near enemy
shields only the far subslot in its own column relative to the party's approach.
Rear members cannot attack or be attacked through that paired front occupant.
A rear melee goblin steps into an available front subslot at the start of its
next attack turn, preferring straight ahead and using the opposite slot if needed,
then attacks once. It can reposition even while protected in its original slot. The checked same-cell move preserves identity,
occupancy and initiative order; it is not pathfinding or an additional action.
Party members retain their formation slots. Sprite facing does not define ranks.
The Rogue exception remains slice 10. Body visibility additionally gates UI targets.
This replaces the original whole-row rule following user review on 2026-10-25.

Melee uses the equipped main-hand weapon, existing derived bonuses and dice;
goblins use their first catalog melee attack. Defend imposes disadvantage until
the defender's next turn or unconsciousness. Dodge's Dexterity-save/speed aspects
are outside this slice. Aid spends an action on a DC 10 Wisdom check to stabilize
another dying party member; skills are absent, so no Medicine proficiency applies.

Party health distinguishes conscious, dying, stable and dead. Death saves occur
once at turn start: 10+ succeeds, 1 adds two failures, 20 restores 1 HP and allows
that turn's action; three successes stabilize, three failures kill. Healing and
stabilization reset counters; stable characters remain unconscious at zero HP.
Damage at zero adds one failure (two for critical damage), removes stability,
and damage at least maximum HP kills. Excess damage after reaching zero at least
equal to maximum HP causes instant death. Dead members cannot be healed.

Goblins select uniformly among conscious characters in exposed slots. No
conscious party member while enemies remain ends the fight in defeat immediately.
Victory stabilizes surviving downed allies without HP recovery; Continue returns
to exploration with retained HP/deaths. This automatic stabilization and omitted
1d4-hour natural recovery are explicit checkpoint adaptations. Restart reconstructs
the session. XP, loot, resurrection, consumables and normal healing actions remain
deferred; tests/development scenarios exercise healing through the real API.

## Ownership and interfaces

A graphics-independent, immovable session owns Party, ActorWorld, movement,
seeded DiceRng, CombatController and gameplay elapsed time, borrowing immutable
map/catalog. Party owns checked resource mutation and exposes const snapshots;
pure health transitions belong to rules. Combat borrows session owners and owns
only encounter/turn state. UI retains selection, visibility gating and bounded
logging, and forwards typed requests with ally targets and world/turn tokens.
Invalid requests consume neither RNG nor actions. Fixed typed outcomes return
synchronously; there is no general event queue or bus.

One cell bounds the encounter to four enemies, four groups and eight initiative
entries. Fixed arrays hold membership, order and defense flags. Unsupported
monster melee content fails during session preparation. Actor IDs remain
world-scoped; character IDs remain party-local. Turn tokens include world identity
and a non-reused serial, rejecting duplicate/stale requests after advancement or
restart. No runtime storage growth; the existing log overwrites oldest entries
at its 32-entry limit. Names are borrowed, formatted messages use stack storage.

Gameplay time freezes on focus loss, map display and expanded-log browsing.
Damage and animation use this same time axis. A frame first applies movement and
automatic simulation, samples/composes for current picking, routes one UI request,
then resamples/recomposes if mutation changed the view. A confirmation sampled
against the previous presented world/turn/phase cannot execute a newly selected
turn or acknowledge a newly appeared result screen. No automatic action can
follow a newly submitted action in the same update. Rendering never resolves
gameplay. No general effect/cooldown scheduler is introduced.

The application owns assets/GPU resources separately. Restart destroys every
session borrower (commands, animator, frames) before replacing the session, while
reusing immutable prepared images. Startup/restart/capture preparation may allocate;
ordinary engine-owned simulation, commands, snapshots and composition may not.
--seed gives reproducible play; normal startup chooses/reports one seed. Restart
reuses that seed; --combat-demo defaults to seed 5. Interactive frame deltas cap at
100 ms; simulation calls reject nonfinite/negative deltas and cap at 60 seconds.
--combat-demo uses the same combat path beside the existing encounter. Existing
--ui-demo and authored animation captures stay isolated preview diagnostics.

## Presentation and validation

Preserve fixed 1920×1080, prepared images, art viewport and accepted typography.
Seven action rows include Aid; ally targets use the existing list/cards. Active
turn and inspection selection are distinct. Show death saves, stability, defense,
round, enemy action, outcomes and explanatory rejection messages. Back-row melee,
ranged, spell and item commands explain their deferred availability. Idle-only
production placements face the party at entry; existing 100 ms hit/death flashes
remain. Existing frontal attack art now plays once for every enemy attack, including misses.
New hurt/death art remains slice 06.

Test-owned fixtures cover pure health boundaries, deterministic initiative and
turns, both row collapses from every approach, visibility, stale/foreign requests,
invalid requests preserving RNG, Aid/Defend, complete wins/losses, queue/contact,
pause/large gaps, exactly-once death/outcomes and restart lifetimes. Extend the
allocation guard through first combat entry and subsequent turns. Run Debug and
Release CTest, deterministic combat/recovery captures where permitted,
and existing diagnostic viewport comparisons. Record actual checks and unavailable
hardware in slice05_verification.md. Interactive desktop checks must be performed
by the user: agents must not control mouse/keyboard, focus or desktop state. Stop at Ready for review; user acceptance of
slice 05/milestone A is required before slice 06.


## Delivered entry points and handoff

- `rules/health.*`: value-only health transitions and validation; `Party` commits
  successful changed results without exposing mutable snapshots.
- `game/session.*`, `game/combat.*`: session ownership/contact, clock, initiative,
  formation protection, authoritative action checks and exactly-once outcomes.
- `game/command_types.hpp`, `game/commands.*`: shared typed requests and reasons,
  current-visible-body gating, ally selection, bounded formatting and HUD state.
- `app/gameplay.*`: production orchestration/restart and explicit combat capture
  scenarios. GPU/prepared-image ownership stays with application/presentation.

Near enemy rows are selected by approach: north uses subcell y=1, east x=0,
south y=0, west x=1. Living near members shield their paired far member regardless of
sprite orientation. UI labels protected visible targets `/ rear`; clicking them
explains rejection. Invisible bodies never enter the target list. Combat itself
has no renderer dependency; bypassing UI requires the caller to supply its own
presentation eligibility gate. Authoritative rank/range/life/turn checks always run.

The health API supports later healing consumers; this slice exposes only Aid in
normal play. NPC death remains the existing immediate occupancy release and 100 ms
flash, independent of turns, rendering or hidden actors. Frame/animation borrowers
must be rebuilt on restart because actor IDs and revision domains change. Direct
actor mutations and health fixture setup are diagnostic preparation, not new
player controls. Future moving-AI/hybrid slices must explicitly revise membership,
movement locking and damage/phase-clock contracts instead of treating this limited
stationary controller as a complete encounter framework.

## Review corrections — design (2026-10-25)

Implemented. ActorWorld owns a checked attack timestamp cue,
independent of damage, incrementing its revision on each attack. NpcAnimator samples
the existing attack clip once from that timestamp, then returns to authored idle;
missing attack art falls back to idle. Snapshot clip selection lets the renderer
reuse prepared assets without mutating placement animation. Death freezes the
selected pose at death time; animation never changes rules or HP.

Picking's existing bounded scan also records visible-body bounding-box centers,
indexed by actor slot and scoped to the world/revision. CommandController copies
the current target center when recording an attack, before refreshing dead targets.
The resulting splash retains that point even after a killing blow hides the body.
It owns eight fixed feedback records (four monsters plus four party recipients).
Slice 06 adds eight separate pending records for enemy strikes; visible lifetime
and portrait highlight begin at the middle attack frame. See
[impact timing](goblin_reactions_design.md#attack-impact-feedback).
A later hit on the same recipient replaces that recipient's splash; if full, the
oldest record is replaced. Records expire after 1000 ms of session time, freeze
with pause, and clear on exploration/movement/restart. Party feedback uses character
identity to find the correct current portrait. Monster anchors stay within the
viewport; unknown anchors suppress the marker rather than invent a location.

HUD geometry draws a compact jagged red splat with white damage digits, or a white
splat with black MISS text, centered over the body/portrait. Text uses the existing
native-resolution bold atlas. Geometry is code-native, requires no new textures,
and preserves prepared image rendering. All storage and formatting are bounded;
no frame-path allocations or general effect scheduler are introduced.

Headless regression checks cover each vacant lane from all four approaches,
advance occupancy and one attack per turn, miss/hit attack playback and expiry,
missing-art fallback, clipping/body anchors, lethal-hit feedback, replacement,
pause/expiry/restart, and the existing allocation guard. Final GPU/visual review
is left to the user; desktop input or focus automation is prohibited.

## Front-slot review correction (2026-10-26)

Implemented and accepted with slice 06 on 2026-10-26. Reported in a user screenshot: a protected rear goblin never takes the
vacant opposite front slot. Keep paired protection for current melee targeting,
but separate it from an enemy's ability to move into melee on its turn.
CombatController owns a pure bounded query for the actor's usable front subslot:
front actors keep their slot; rear actors try the paired slot first, then the
other front slot relative to the party's approach. Both candidates must be free
of living occupants. Dead actors release occupancy immediately. No route search,
additional action, cross-cell movement, party rearrangement or art change.

Use this same query for initiative selection, the upcoming-turn preview and
execution. Recheck at execution, perform the checked ActorWorld move, and attack
only once movement succeeds (or the actor is already in front). First acting rear
actor claims a vacancy; later actors recompute occupancy. Existing identity,
initiative order, RNG and 450 ms pacing remain intact. Preview copies a four-bit occupancy mask and reserves each predicted destination
in turn order, so two rear actors are not listed for one vacancy. This never
mutates ActorWorld. Fixed two-candidate / four-occupant scans allocate nothing. Target exposure changes only after movement.

Regression checks use generated fixtures for both lanes and all four approaches:
straight preference, diagonal fallback, full front row, two rear actors competing
for one vacancy, one attack per round, occupancy, pause and mutation-free preview.
Run the combat and frame allocation checks plus full Debug/Release suites. Inspect
an application-owned hidden capture of before/after diagonal advancement; leave
interactive desktop review to the user.

# Hybrid encounter loop — slice 08

Status: implemented and Accepted, including all review corrections, 2026-10-26.
See [verification and handoff](slice08_verification.md).
User approved the planning conversation with “Implement the plan.” Slice 07 and
its two review corrections are accepted. The user has now accepted slice 08 and
milestone B. Discuss graphical changes before planning or starting slice 09.

## Ownership and boundaries

GameSession owns the party, ActorWorld, movement, presentation/gameplay clocks,
NPC AI and combat. NpcAiController replaces the goblin-specific controller:
guard/patrol/wander, perception and navigation are reusable; catalog traits and
small policy functions select actions. No per-species class hierarchy, scripting,
ECS, event bus or universal effects framework. Shared geometric queries serve
NPC/player LOS and forward-view eligibility. ActorWorld alone commits occupancy,
movement, health and cues; death releases occupancy immediately.

Fixed storage supports 256 actors, 32 monster-definition initiative groups, 36
initiative entries and eight HUD preview entries. Readiness, awareness, surprise
and spent actions are stable-slot arrays. Navigation retains its shared map-sized
workspace, 4096-node and eight-request per-update budgets; unfinished work waits.
No engine-owned allocations in normal updates. Preparation/restart/export may
allocate. All borrowers are reconstructed on restart. AI randomness is independent
of combat rolls. Invalid commands spend no RNG, time or resources.

Catalog schema 3 adds a typed trait set (initially adapted Nimble Escape) and an
optional ready ranged equipment reference. Schemas 1/2 remain readable with empty
traits/ranged slots. The Rogue readies the already-owned bow; ammunition is not
spent until slice 11. Spells/effects, inventories, loot, XP and saves are deferred.
Commands carry world and monotonic action serials.
Snapshots expose phases, readiness, automatic facing, known directions and new arrivals.
Attack outcomes carry kind/impact timing rather than making UI infer dagger timing.

## Time and action economy

Presentation time advances unpaused animations/feedback. Gameplay time advances
in real time during exploration/approach, freezes during turn decisions/playback,
and advances exactly 6000 ms at completed round boundaries. All pause overlays and
focus loss freeze both clocks. A ranged attack sets a persistent six-second
readiness deadline; remaining approach cooldown spends the first-round action.
Thereafter each participant gets one action per round. Contact transitions never reset
costs or initiative during the same threat episode. Death-save deadlines persist
across phases; active real-time danger permits one save per six seconds. Defend
expires at the next turn or its real-time equivalent. No elapsed-time catch-up burst.

Damage resolves exactly once at command execution. Preserve the acting participant
and serial until clip/feedback holds expire; then hand off in a separate update.
Terminal overlays also wait. Real-time attacks may overlap between participants,
but each actor waits for its own travel/shot presentation before moving/shooting.
Contact waits for committed travel and outstanding attack presentation.

## Ranged, awareness and membership

Bows use six-cell Euclidean range and conservative cell-center wall/corner LOS.
Creatures do not provide geometric cover and ranged ignores melee row protection.
Party attacks require a stationary party and current visible body. An adjacent
conscious hostile gives ranged disadvantage; advantage/disadvantage cancel normally.
Healthy goblins shoot while closing in real time. At most one automatic attack per
update with fair scheduling; no shot during walking or movement during bow playback.

Hit or miss alerts the target and living goblins within three cardinal walkable
steps of it, using a bounded local flood without recursively propagating alerts.
An unnoticed opening shot gains advantage based on pre-resolution awareness.
Party sight is a strict forward cone with six-cell range, conservative LOS and
six-second threat memory. The cone excludes distant 45-degree edge cells; the immediate three-cell melee
front is included explicitly, with edge presentation keeping its bodies visible. This predicate also gates both sides' ranged attacks,
enemy initiative eligibility and the HUD preview. Offscreen enemies cannot attack,
including during real-time approach. NPC detection retains its six-cell/90-degree
cone. Known encounter threats stay known while turning. Unaware enemies skip
their first action, sampled before contact reveals the party; this happens once
per threat episode. The party always receives normal opening initiative.

All clear eight-neighbor adjacency starts contact. Membership includes adjacent and actively
pursuing/investigating enemies. Roll per character and monster-definition group,
retain existing ties and stable actor ordering. New arrivals join at the next
round boundary, never retroactively; new groups roll once. Outside NPCs get at
most one movement opportunity per round boundary. Membership is separate from
current attack reach. No living threats means victory only if they were defeated;
expired awareness returns to exploration without stabilization. When no adjacent
threats remain, settled combat returns to Approach without charging a partial
round. Re-contact starts a pass through retained initiative, with readiness
deadlines preventing duplicate actions.
Victory retains stabilization; defeat retains existing no-conscious-character rule.

## Facing, formation and NPC decisions

Settled contact enters individual initiative directly. Keep facing while any tile
in the three-cell front band contains living enemies. Once the entire band clears, wait for committed
travel and attack/feedback holds, then face another occupied adjacent tile using
the shortest rotation, clockwise on ties. Rear contact uses two clockwise turns.
Movement uses the existing 160 ms quarter-turn presentation. This never spends an
action or gameplay time and preserves the active decision and initiative cursor.
There is no shared maneuver phase, Hold/Exit, retreat command, or party surprise
penalty. Player movement and rotation stay locked during contact and combat.

Combat owns the Facing phase, pending heading and resume phase/awaiting flag.
GameSession drives one quarter turn at a time and reports the settled heading
through the normal update. Facing invalidates stale confirmation tokens and
freezes gameplay time; pauses freeze presentation too. No safe-location metadata
or escape-spell implementation is introduced.

The front row always shields its paired back row. Party melee needs an exposed
attacker and an exposed enemy in any of the three cells immediately ahead. Enemy near/far protection
and checked same-cell advancement remain (straight preferred, opposite vacant
slot next). Enemies target conscious exposed characters uniformly; ranged may
target any conscious character. Offscreen adjacent enemies wait. Offscreen distant
actors can approach once at a round boundary; that move consumes their next action
readiness. Visible participants retain move-or-attack policy. New arrivals are
targetable as soon as faced, but receive initiative only at the next boundary.

An NPC turn permits one cross-cell move OR attack. Healthy distant goblins alternate
shooting and closing one cell (shoot first when legal), using melee at contact.
Blocked rear melee actors can shoot at close-range disadvantage. A Nimble Escape
actor with both weapons at half HP or less spends its turn moving away if adjacent: directly away,
then lateral alternatives in fixed order, requiring increased distance and checked
occupancy. Blocked escape falls back to attack. Wounded ranged actors do not
oscillate back into melee. Original SRD bonus-action Hide/Disengage is not implemented;
this is an explicit project adaptation, not tabletop equivalence.

## Presentation and asset work

Keep fixed 1920x1080, 84-degree projection, prepared images, discrete collision and
600 ms visual travel. Append optional RangedAttack clip without renumbering old
clips. Present bow clips require four views; absent clips use idle and ranged
feedback. Four transparent reference-matched sheets have two poses each: draw
600 ms, release 400 ms, feedback at release. Turn hold covers clip and full marker.
Use built-in imagegen, preserve selected sheets/prompts, uniform registration and
explicit pivots through the existing offline preparation tool. Inspect identities,
clothing, scale, handedness and grounding. Keep the 160 MiB cap with explicit
failure; preflight representative prepared storage before final generation.

Enable Ranged. In approach portraits select the actor, with a separate details
control; turns select by initiative. The regular action dock remains visible;
automatic turns show the destination heading and explain that the action is
preserved. Gate fresh input against the displayed phase/serial. Display cooldowns
and spent turns, reinforcements and known threat directions without revealing unknown identities.
Approach feedback must survive exploration refresh. Eight cosmetic feedback slots
retain deterministic oldest-entry replacement; simulation events are not dropped.

## Verification and handoff

Generated fixtures cover range/LOS, alerts including misses, legacy and current schema versions,
traits/equipment, fixed paired protection, all contact directions, NPC surprise,
reinforcements, blocked escape, partial-round re-entry, exact clock boundaries, saves, pauses, large
deltas and stale/duplicate requests. Preserve slice 07 holds/impact/travel/picking,
arrival gating and diagonal front-slot regressions. Extend allocation guards through
cold encounter/ranged/reinforcement paths. Separate maximum capacity/nav stress.
Run Debug/Release headless suites, focused sanitizers where supported, shipped
preparation, documentation links and whitespace. Add --hybrid-demo and
--capture-hybrid deterministic scenarios; inspect native/reduced captures and all
bow views. Hidden owned GPU checks only; no OS input/focus/desktop automation.
Record actual commands, results and unavailable checks in slice08_verification.md.
The implementation finished at Ready for review; the user accepted slice 08 and
milestone B on 2026-10-26.

## Implemented interfaces and storage

`combat.cpp` owns geometry, membership, grouped initiative, readiness and previews;
`combat_actions.cpp` resolves checked commands, attacks, saves and NPC turn policy;
`combat_loop.cpp` owns phase transitions and automatic scheduling. `session.cpp`
orders clock advancement, perception/contact, committed movement, AI and combat.
`perception.cpp` shares conservative LOS and range queries. `NpcAiController`
retains its map-owned navigation workspace; combat supplies movement locks and
uses its checked approach/escape operations. Catalogs and maps outlive sessions.
No new long-lived borrowed actor pointers or virtual dispatch were introduced.

`CombatSnapshot` is copied by value into the HUD; membership stores typed world
handles and fixed stable-slot arrays. The HUD's eight-entry turn preview is a
bounded projection, not the encounter's capacity. Pending eligible actors prevent
victory even before the boundary enrolls them. A command carries world/action
serials; invalid and repeated commands do not advance them. Absent bow
art falls back to idle, and the outcome carries zero impact delay. The production
bow clip releases at 600 ms and holds the enemy action through 1600 ms so its
1000 ms marker can finish. Damage itself resolves immediately and exactly once.

Bow canvases are 512×536 at width 0.6, preserving the 384×536 idle canvas's
0.45 world-width scale and shared grounding. Only ranged clips gain the 0.6
canvas allowance; collision remains the same discrete subcell. Eight prepared bow
frames bring combined shipped NPC source/prepared pixels to 160,526,125 bytes
under 167,772,160.
Registration and selected source sheets/prompts are stored under
`assets/npcs/goblin/animation/`; no generated art is created at runtime.

All combat tests/captures use GameSession directly, with no maneuver adapters.
Routine coverage includes all 256 actors and eight-entry preview bounds, but production
art performance at that population is not established by generated fixtures.

## Approved review correction: front-focused combat

The user rejected round-level maneuvers and approved steady frontal combat,
automatic facing, normal party initiative and no retreat with “Implement the
plan.” The implementation and current rules above replace the initial slice 08
maneuver, withdrawal, directional-exposure and party-surprise design.

Regression coverage includes the reported front-clear/side-arrival/fighter-action
case, front persistence, rear/tied rotations, pause/stale input during facing,
attack-marker and pending-arrival travel holds, hidden ranged/melee suppression,
paired protection, reinforcement timing and partial-round costs. Debug/Release,
allocation guards, focused sanitizers and hidden captures verify the correction.
See the handoff for results and the acceptance record. Slice 09 is unstarted.

## Single-weapon review correction

User authorized general melee-only/ranged-only rules and two bow-only goblins in
`--hybrid-demo`. Mixed-weapon goblin behavior remains unchanged; random preferences,
roles and caster policy are deferred.

Combat checks available catalog attacks. Melee-only NPCs close and attack, including
when wounded; the mixed goblin's bow-supported wounded escape does not apply.
Ranged-only NPCs hold a legal shooting position, approach when range/LOS is missing,
and spend an action trying to step away when adjacent at any health. If trapped,
they shoot with the existing close-range disadvantage. Movement and shooting remain
separate actions. Offscreen attack restrictions and cooldown/presentation holds remain.

NpcAiController caches a bounded ranged-only flag per actor at construction. Its
real-time/boundary pursuit stops at a legal shooting position instead of closing
into melee. Awareness/search/navigation remain shared. Combat retains its existing
cached attack handles and controls turn actions; no new per-frame allocations.

The demo needs two rules variants sharing one prepared appearance. NpcInstance
has an optional typed MonsterId override resolved by the caller against the
session's immutable catalog; ActorWorld validates it during construction and
otherwise retains appearance-name binding. No runtime equipment mutation or JSON
placement schema change. The catalog adds a bow-only goblin definition, and demo
placements resolve it once. Art preparation and its memory budget remain unchanged.

Generated tests cover ranged holding/closing/blocked LOS/escape/trapped shots,
melee closing/attacking at both health levels, unchanged mixed behavior, placement
binding validation and allocation guards. Debug/Release suites, the allocation guard and focused ASan/UBSan pass. Hidden
demo captures verify the two archers hold their shooting positions and reuse the
same prepared images. Keep this correction in slice 08 and await review.

The demo retains four mixed-weapon goblins and adds two `goblin.archer` placements
at (2,7) and (4,7), facing the starting party. They have only the shortbow attack
but share the goblin idle/walk/hurt/death/bow art; this is a rules distinction, not
new visual equipment. No extra prepared-image storage is required. Capture cases
`demo` and `ranged-only` cover the actual placement and isolated shooting behavior.

## Front-approach routing correction

The user reproduced a full-health mixed-weapon goblin leaving the frontal battle.
A headless demo trace confirms ordinary pursuit chooses a tied shortest route
from (4,9) through (4,10) to the party's right flank (4,11), rather than the open
front cell (3,10). This is path selection, not wounded escape.

NPC pursuit/turn approach from ahead of the party prefers its front cell, then
the diagonal front cells before considering a flank (extended by the three-cell correction).
Navigation's existing bounded BFS accepts an optional preferred adjacent goal;
it keeps the first ordinary adjacent result as fallback if the preferred cell is
unreachable. A full/solid center uses reachable diagonal front cells first; only an unavailable
entire front band falls back to other adjacency.
Root expansion prefers moving toward the front axis on tied routes, keeping the
reported actor in view while closing. Side/rear approaches and explicit escape
remain unchanged. Result metadata includes the preference so stale requests do
not cross a facing change. Storage adds fixed request/result values and one
fallback step; map-sized scratch, cleanup and per-update budgets remain unchanged.
An unreachable preferred goal can require exploring the reachable area; work
remains incremental and capped, and no frame-path allocations are added.

Living melee targets outside the three-cell front band return OutOfRange, with a
weapon-neutral range message. Dead/removed/foreign targets retain InvalidTarget.
Range rejection spends no action, RNG or cooldown. The initiative strip is
explicitly unchanged at the user's request.

Regression tests cover the reported route through multiple rounds, four
headings, blocked/full front destinations, navigation fallback/budgets, and melee
hover/log/command rejection. Debug/Release, allocation checks, focused ASan/UBSan
and actual-demo hidden captures pass. Keep this review correction inside slice 08.

## Three-cell melee front

User approved the three cells immediately ahead as one melee front and requested
AI use diagonal front tiles when the center tile's four slots are full. Shared
perception helpers define eight-neighbor contact (Chebyshev distance one) with
conservative wall/corner LOS, and a heading-relative front band (depth one, lateral
-1..1). Both sides use this band for melee; fixed paired protection is relative
to party heading across all three cells. Existing body picking still gates clicks.

Diagonal contact starts initiative. Keep facing while any reachable front-band
cell is occupied; automatic facing searches for another occupied band only after
it clears. Ranged disadvantage and archer/trait escape use the same close-contact
predicate. Escaping must increase Chebyshev separation beyond one, preventing a
sideways step that remains in the new melee zone. Single-weapon rules otherwise
remain intact. Movement stays cardinal and discrete.

Navigation's optional preferred front becomes a band: center remains first choice;
when unavailable or unreachable, reachable diagonal front tiles are preferred to
side/rear contact. BFS retains bounded fallback candidates and accepts diagonal
contact only with clear corner LOS. Shared AI contact stopping matches combat.
No new runtime allocations, unbounded loops or initiative-strip redesign.
The two diagonal cells retain four slots each; occupancy and same-cell movement
remain authoritative, with rows paired relative to the party heading.

Visibility preflight with shipped art found zero picked pixels for the outer near
slot in each diagonal cell (left subcell 0,1; right 1,1 when facing north). The other
six positions were visible. The user approved drawing edge NPCs slightly inward. PreparedNpcs shifts
the horizontal front-row layout inward to fit the outer actors' full pose canvases,
using a per-type maximum half-width across clips for stable positioning. Apply
a continuous lateral fit outside the center-slot pivots (±0.25 cells), leaving
the center-cell pair at ordinary perspective spacing. Distance beyond each pivot
is compressed toward the edge limit; adjacent diagonal subslots stay distinct
instead of clamping multiple bodies to the same edge. This supersedes the original
whole-row proportional compression, which squeezed center pairs together (8.3
review correction, 2026-10-27). Body,
shadow, picking and feedback anchors share that offset; world cells, depth, scale,
FOV and camera remain unchanged. Travel uses the same position-dependent
compression, blended at the row edges to avoid abrupt transitions. Verification
covers all headings/slots, blocked corners,
row protection, front persistence, diagonal archer escape, center-full routing,
allocations and hidden shipped-art captures.

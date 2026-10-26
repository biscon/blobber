# Moving goblin AI — slice 07

Status: implemented, accepted and closed (2026-10-26), including attack-turn
sequencing and interpolated-walking corrections. The user said: “slice accepted,
update the document”. See [verification and acceptance](slice07_verification.md).
User approved the plan with “Implement the plan.” Slice 08 is next eligible and
remains Planned; it has not started.

## Behavior and boundaries

Guards, looping waypoint patrols (at most eight waypoints), and local wandering
share deterministic directional awareness. Sight uses a six-cell Euclidean range,
a 90-degree forward cone and conservative cell-center wall/corner occlusion.
Actors do not obscure sight. No hearing, shared alerts or surprise mechanics.
Pursuit targets a free cardinal neighbor of the visible party. Lost sight retains
the last seen cell; investigate it (stop adjacent and face it if the party still
occupies that remembered cell), then turn every 600 ms for 2400 ms. Give up
after 6000 ms without sight even if travel/search has not finished. Guards return
home and restore facing; patrols resume their waypoint; wanderers return home.
Reacquisition interrupts investigation, searching or returning.

Pursuit/investigation steps take 600 ms; routine/return steps take 1200 ms.
Patrols pause 1200 ms at waypoints. Wanderers choose uniformly among waiting and
legal neighboring cells within Manhattan radius three of home. Side/rear adjacent
pursuers face the party and wait. Frontal contact alone starts the existing combat.
All AI work/time stops outside Exploration, including result screens; Continue
does not accumulate movement. Focus/map/modal pauses freeze both AI and session time.
Nimble Escape is inactive; slice 08 must decide its adaptation with disengagement.

## Ownership, interfaces and data

GameSession owns a graphics-independent, immovable GoblinAiController, borrowing
the map and ActorWorld. Fixed records indexed by stable actor slots retain routine,
awareness, home/waypoint, last sight and deadlines. Const snapshots support tests
and diagnostics. ActorWorld remains the only position/facing/occupancy/HP authority.
Placement AI settings are optional and default to guard. JSON parsing and static
route validation occur at preparation. No asset or catalog version changes.

AI has a separately seeded RNG, preserving the combat RNG stream. One tick per
100 ms of active exploration, no catch-up; at most one step per actor per tick.
Party movement/contact is processed first, then an AI batch, then contact again.
The party's committed logical cell is authoritative, including during transitions.

Navigation uses incremental cardinal BFS with one shared preallocated workspace:
one uint32 queue index, one visited byte and one first-step byte per map cell
(6 MiB at the 1024-square map limit), plus fixed actor requests. Search and cleanup
share a 4096-node budget per tick; at most eight requests finish per tick. Requests
are serviced round-robin and an active search keeps its destination until done.
Visited entries are cleared incrementally through the queue, avoiding full-map
clears or generation wrap work. No runtime growth, per-actor paths or heap requests.
Full cells, walls and the live party cell block traversal. Search observes current
occupancy; results are advisory and every committed first step is revalidated.
Prefer the existing subslot, then row-major free subslots. Rotate movement priority.
No path, stale results or exhausted work budgets mean wait/retry, never teleport.
Long searches can delay actors; the stress report must measure this explicitly.

Successful cross-cell AI movement records a checked 600 ms walk cue on session
time. Failed moves never cue walking. Attack/damage cancel walking; reactions keep
their existing priority and ownership. Missing walk art uses idle. Logical positions
stay discrete; visual travel now interpolates as described in the review correction
below. Prepared images, all four walk directions and the 160 MiB image budget are
retained. Animation is sampled
after mutation and never resolves simulation. Restart reconstructs all borrowers.

## Review and verification

Add --ai-demo (seed 5 by default) and --capture-ai DIRECTORY. A small deterministic
map demonstrates guard, patrol, wandering, corner pursuit and frontal contact.
Keep normal placements as alert guards and preserve stationary combat/UI review
setups. No OS input, window focus or desktop control; captures use hidden owned
windows and in-process simulation. Interactive review remains the user's task.

Headless fixtures own generated maps/catalogs/images and cover sight boundaries,
occlusion, memory/search/return, patrol/wander, contention, blocked/unreachable
routes, capacities, pauses, contact/Continue, lifecycle, restart and walk playback.
Extend allocation checks through first navigation, unfinished searches, contention
and rendering. Run Debug/Release suites and separate 256-actor/large-map stress
checks. Inspect deterministic captures and GPU lifecycle where available; report
unexecuted checks. Record commands/results in slice07_verification.md, update the
roadmap and overview, then stop at Ready for review. Milestone B stays open.

## Delivered integration

`src/game/goblin_ai.*` owns state/scheduling, `src/game/navigation.*` owns bounded
BFS, and `src/app/ai_demo.*` owns scenario/capture setup. Session integration runs
AI between party movement/contact and the final contact check. Passing
`enableAi=false` to GameSession preserves stationary combat captures; ordinary
play defaults to enabled. The AI clock is distinct from presentation/session time.

Optional placement JSON uses `"ai": {"routine": "guard"}`,
`"ai": {"routine": "wander"}`, or
`"ai": {"routine": "patrol", "waypoints": [[9,9],[12,9],[12,5],[9,5]]}`.
Omitted settings default to guard. Patrols require 1..8 statically connected,
walkable points; other modes reject nonempty waypoint lists. Loading computes
components once only when patrols exist. Runtime settings remain fixed values,
independent of authored animation loops and prepared appearance types.

ActorWorld exposes a const four-bit occupancy query and a checked `walk` operation.
A successful cross-cell move stores session time; no-op/failed moves do not cue
walking. Timestamp ordering suppresses a canceled walk after attack/damage while
retaining enough history to freeze the correct pose for legacy lethal flashes.
The renderer consumes interpolated offsets from the animation snapshot without
cache rebaking. AI has no animation timing/resource dependency.

The maximum-map stress result establishes capacity/work safety, not responsive
navigation at maximum actor/map scale. See the verification report for queue
latency measurements and unverified platform/interactive checks. No scope change
or slice 08 implementation was needed.

## Review correction: finish attack presentation before advancing

Implemented and reverified 2026-10-26 following the user's overlap report. Combat
previously selected the next player immediately when an enemy attack resolved;
its 450 ms delay gated only automatic actions. Keep the acting participant and
turn serial while a resolved attack is presenting. Reject further commands and
automatic attacks until a session-time deadline, then select the next turn on a
separate update. Automatic successors retain their existing 450 ms preparation
delay; player input becomes available at the handoff. Roll/damage/death/occupancy remain authoritative and exactly once
at action start; presentation never executes damage or signals completion.

Copy validated clip timings into fixed combat duration tables during preparation.
Enemy hold = max(450 ms, attack clip duration, impact offset + 1000 ms marker).
The shipped goblin therefore holds 1950 ms (1550 ms clip, strike at 950 ms).
Party melee holds max(450 ms, 1000 ms marker, affected hurt/death clip on a hit).
Missing art retains the marker wait. Defend/Aid/death saves retain existing pacing.
Victory/defeat presentation waits for an in-flight attack before showing its result.
Pause freezes deadlines; large gaps release once without executing the next auto
turn in the same update. Restart reconstructs all timing/turn state. Share impact
calculation and marker lifetime with UI to avoid independent timing assumptions.
Show RESOLVING while a player attack is finishing, rather than suggesting another
command or a death save. Keep the existing confirmation token/presented-turn gate.

Regression checks cover enemy-to-player and enemy-to-enemy handoff, hit/MISS,
player reaction/marker waits, terminal outcomes, absent/long clips, exact deadline,
pauses, large gaps, duplicate inputs, allocation behavior and deterministic captures.
This is a slice 07 review correction, not slice 08 work.

## Review correction: interpolated walking

Implemented, verified and accepted with slice 07 on 2026-10-26.

Keep grid occupancy and pathfinding discrete. A successful cross-cell `walk`
commits its destination immediately and retains one presentation segment per actor:
source world position, start timestamp and a shared 600 ms duration. Sample linear
travel from session time, including subslot offsets. A new step starts at the
current presented position; an explicit position-changing `move` cancels travel. Death freezes the
position at the lethal timestamp. Pause freezes session time. No source-cell
reservation or continuous collision is introduced.

`NpcAnimator` publishes destination-relative offsets with each frame selection and
invalidates presentation when they change, even within a held walk frame. Combat
contact locks further exploration immediately but waits for its living members'
travel to finish before facing the party and beginning initiative/actions. This preserves attack handoff
holds and prevents attacking an approaching body before it arrives.

The renderer projects the interpolated world position for facing, size, grounding,
sort order, wall clipping and picking. Stationary sprites retain the existing
prepared depth banks. In-flight sprites sample an additional immutable, alpha-cut
raster per pose, prepared at startup with its longest dimension capped at 256
pixels (never enlarged). This avoids rescaling an already viewport-cropped bank.
Per-draw 256-entry lighting tables shade these prepared samples without per-pixel
lighting arithmetic. The tradeoff is modest transient image filtering/detail loss
versus retaining full source images. All source, cached, travel and shadow pixels
share the existing 160 MiB preparation cap; exhaustion rejects loading.

Moving floor shadows sample one prepared 64×64 radial alpha kernel projected onto
the floor at the continuous position. Composition uses fixed 256-actor scratch,
viewport-clipped loops and no allocation. No assets or GPU resources are generated
per frame. Validate exact start/midpoint/end, chaining/teleport/death, held-frame
invalidation, wall occlusion/picking, arrival-before-combat and pause; rerun CPU
suites/allocation checks and inspect hidden deterministic movement captures.

Delivered in `game/actors.*`, `game/npc_animation.*`, `game/combat.cpp`, and
`render/npc_sprites.*` / `render/npc_travel.cpp`. Stationary composition remains
pixel-identical. The source cell is released at commit; presentation can therefore
cross an already-vacated source cell, and overlapping early diagnostic steps use
a direct segment rather than reserving a continuous swept path. Ordinary AI steps
are at least 600 ms apart. Front-slot combat shuffles still use instantaneous
same-cell `move`; this correction covers cross-cell walks.

# Slice 07 verification and review

2026-10-26 — Moving goblin AI **Accepted and closed**, including attack-turn
sequencing and interpolated-walking review corrections.
Design: [ownership, behavior and limits](goblin_ai_design.md).

## Executed checks

```sh
cmake --build --preset debug -j 4
env -u DISPLAY -u WAYLAND_DISPLAY ctest --preset debug --output-on-failure
cmake --build --preset release -j 4
env -u DISPLAY -u WAYLAND_DISPLAY ctest --preset release --output-on-failure
```

Both builds succeeded without compiler warnings. `git diff --check` and local
documentation-link checks passed. CLI checks passed for --help and rejection of
conflicting demo/capture modes. The final contention extension to the allocation
guard was rerun successfully in both configurations. All 19 suites passed in both
configurations (Debug 28.66 seconds; Release 3.97 seconds). The added AI suite
covers directional/range/corner sight, guards, patrol loops, seeded wandering,
memory/search/return/reacquisition, wall detours, unreachable and occupied goals,
bounded unfinished searches, fair queue service, cancellation, four-subslot
contention, pauses, side-contact waiting, frontal combat entry, Continue, restart,
death/removal and large time gaps. Independent AI randomness preserves subsequent
combat initiative and rolls.

Loader checks cover legacy guard defaults, optional patrol JSON, malformed modes,
waypoint capacity, wall/disconnected destinations and routine compatibility.
Animation checks cover real move cues, blocked steps, expiry, attack/hurt/death
interruption, missing walk art and legacy lethal-flash pose preservation.
Existing rendering/transition/picking and slice 06 reaction/impact/front-slot
regressions pass. Allocation coverage includes cold and ongoing navigation,
incremental cleanup, four-actor movement contention, patrol presentation, and the
existing combat/UI paths: zero engine-owned C++ allocations after preparation.

An early wandering test exposed misuse of the tabletop dice API for arbitrary
choice counts; replaced it with a separately seeded, unbiased bounded-choice
sampler. Capture review exposed pursuit stalling when a path turn lost sight of
the party while its remembered cell remained occupied. Investigation now approaches
that remembered cell's collision boundary and faces it. Both have regression tests.

Focused sanitizer check:

```sh
c++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Isrc -Itests tests/ai_tests.cpp src/game/goblin_ai.cpp src/game/navigation.cpp \
  src/game/session.cpp src/game/combat.cpp src/game/party.cpp src/game/movement.cpp \
  src/game/actors.cpp src/world/map.cpp src/world/npc.cpp src/rules/resolve.cpp \
  src/rules/health.cpp src/rules/character.cpp src/rules/content.cpp \
  -o /tmp/blobber-slice07-ai-sanitized
ASAN_OPTIONS=detect_leaks=0 /tmp/blobber-slice07-ai-sanitized
```

AddressSanitizer and UndefinedBehaviorSanitizer passed. The initial run with leak
detection enabled failed because LeakSanitizer cannot operate under this
environment's tracing/ptrace setup; leak detection is **unverified**, not passed.

## Navigation stress and limitations

```sh
./cmake-build-release/blobber_ai_tests --stress
```

Separate from routine CTest. Queues 256 actors' searches for a distant destination,
without moving actors between requests. Every request completed, every result was
reachable, every update stayed within 4096 search/cleanup visits and eight completed
requests, and storage never grew.

| Map | Queue-draining ticks | Scratch bytes | Measured wall time |
| --- | ---: | ---: | ---: |
| 25×25 | 63 | 3,750 | 2 ms |
| 1024×1024 | 130,306 | 6,291,456 | 5,532 ms |

These accelerated CPU measurements are not live gameplay latency. At the 100 ms
AI cadence, draining these artificially simultaneous queues would take 6.3 seconds
and about 3.62 hours respectively. The shared BFS deliberately bounds frame work
and memory; it is not a production-scale many-agent navigation solution. The
four-goblin demo has short local requests, ordinarily completing within its first
eligible tick. Larger maps/crowds need a separately designed navigation change
before claiming responsive pursuit at those scales. No measured work-budget or
capacity overrun occurred. No driver allocation guarantee is implied.

## Rendering checks

```sh
cmake --build --preset debug --target blobber_gpu_smoke -j 4
./cmake-build-debug/blobber_gpu_smoke
./cmake-build-release/blobber --capture-ai /tmp/blobber-slice07-ai-final
```

The sandboxed capture attempt could not connect to X11 display `:0`; the authorized
retry outside that restriction passed. GPU lifecycle and 40 deterministic native
1920×1080 AI captures passed on Linux/X11/OpenGL. All windows were hidden and owned
by the application. Movement scripts call session methods; no OS input, focus,
desktop switching or foreign-window control was used. Captures and graphics
resources shut down cleanly.

Inspected approach/walk frames, patrol approach, wandering, side-view corner
movement, searching and frontal contact. Bodies remain grounded, wall clipping and
the accepted HUD remain intact, and simulation positions match the exported trace.
NPC source/prepared pixel storage remains 118,463,632 bytes (112.98 MiB), below the
unchanged 128 MiB limit. No images were regenerated or enlarged.

Selected retained captures:

- [Approach/walk](screenshots/slice07/ai-approach-1000.png)
- [Frontal contact](screenshots/slice07/ai-approach-2500.png)
- [Corner investigation](screenshots/slice07/ai-corner-1900.png)
- [Deterministic state/clip trace](screenshots/slice07/ai-trace.tsv)

Trace awareness values: 0 routine, 1 pursuing, 2 investigating, 3 searching,
4 returning. Clip values retain the existing enum: 0 idle, 1 walk, 2 attack,
3 hurt, 4 death. Diagnostics report snapshots without mutating them.

Physical interactive play, Windows/macOS and physical high-DPI checks remain
unverified by the agent. User acceptance does not turn unexecuted checks into passes.

## User review

```sh
./cmake-build-release/blobber --ai-demo --seed 5
./cmake-build-release/blobber --combat-demo --seed 5
```

Use WASD to step/strafe, Q/E to turn and F1 to view the map (which pauses play).
The AI demo starts at (3,11), facing north, in a 15×15 room with a partial wall at
x=5, y=2..7. A north-facing guard stands at (3,6), a patrol starts at (9,5), a
wanderer at (10,12), and another guard at (7,3). The starting guard cannot see you
behind it; explore around it or cross its forward view to provoke pursuit.

The patrol loops (9,9) → (12,9) → (12,5) → (9,5), with pauses. The wanderer stays
within three Manhattan tiles of home until alerted. Move around the wall after
being seen to observe last-seen investigation, searching and return. Opening
map/history/details or losing focus freezes simulation. Side/rear pursuers wait
until faced; frontal contact starts the accepted turn-based fight and suspends
every other roaming actor. Normal attack/Defend/Aid controls and Continue/restart
remain available. The stationary combat demo preserves the earlier review setup.

The capture command is an alternative deterministic review: `routines-*` shows
the initial scenario over time; `patrol-*` and `wander-*` use closer cameras;
`approach-*` reaches frontal combat; `corner-*` moves the party behind a wall,
then shows investigation, searching, reacquisition and contact. See the trace for
actors outside the camera. Capture clocks advance explicitly even though these
are noninteractive scenarios.

## Handoff

Slice 07 is accepted and closed. Slice 08 is next eligible but remains Planned;
do not begin it automatically. GameSession
owns AI alongside party/actors/combat; its optional `enableAi=false` argument is
for stationary diagnostics. Normal play and --ai-demo enable it. AI elapsed time
advances only during unpaused exploration, while presentation retains session time
through combat/results. This temporary phase policy must be revisited explicitly
in slice 08, alongside reinforcement membership, side/rear contact and Nimble Escape.

Keep actor occupancy authoritative and revalidate advisory navigation results.
Keep combat RNG independent of wandering. Placement routines survive gameplay
copies, but authored animation loops remain isolated diagnostics. Successful AI
steps cue walking; timestamp ordering cancels it on attack/damage without losing
legacy death-pose history. Rebuild all borrowers on restart. Preserve the previous
slice's paired protection, diagonal advancement, impact timing and immediate dead
occupancy release. No loot, ranged combat, retreat, surprise or shared alerts were
introduced.

## Review correction: attack turn handoff

2026-10-26, implemented and accepted with slice 07.
The reported overlap was reproducible from the code: resolution immediately
selected a new conscious party member, bypassing the delay that only gated
automatic actions. Combat now retains the acting participant and turn serial
with `resolvingAction=true`, rejects further actions, and waits on session time
until both clip/reaction and marker have expired. The next turn is selected in a
separate update; automatic successors retain their existing 450 ms preparation
delay. Damage/death and occupancy still resolve immediately and exactly once.
The goblin's 1550 ms clip strikes at 950 ms, so its one-second marker holds the
turn until 1950 ms. Player attacks hold for at least their one-second marker, or
longer hurt/death art. Terminal overlays wait for the in-flight presentation.

Startup passes prepared timings into GameSession/CombatController and the HUD;
shared duration/impact helpers and marker constant prevent drift. Missing art
still holds for feedback; no renderer callback or dynamic allocation controls turn
advancement. Pauses freeze the deadline. The dock displays RESOLVING and does not
mark a party member YOUR TURN until that character can act.

Commands rerun:

```sh
cmake --build --preset debug -j 4
env -u DISPLAY -u WAYLAND_DISPLAY ctest --preset debug --output-on-failure
cmake --build --preset release -j 4
env -u DISPLAY -u WAYLAND_DISPLAY ctest --preset release --output-on-failure
./cmake-build-release/blobber --capture-hud /tmp/blobber-slice07-turn-handoff
```

Debug and Release builds passed without warnings. All 19 suites passed in both
(Debug 29.18 seconds, Release 4.00 seconds), including the allocation guard using
prepared combat timings. Added regressions cover enemy-to-player/enemy handoff,
strike/marker boundaries, pause, long/missing clips, large skips without a second
action, duplicate rejection, immediate lethal occupancy release, deferred victory
and defeat. Existing initiative-preview tests now check advancement only after an
actual turn change, retaining the same expected ordering. HUD tests verify that a
click consumes its action while retaining the old turn during presentation.

All 59 hidden-window HUD captures passed with clean resource shutdown. Inspected
hit/MISS strike, final marker frame and handoff at reduced 1280×720, with native
captures also exported. The final feedback frame retains ENEMY TURN / RESOLVING;
the handoff removes the splash and enables the next player's card and controls.
No OS input, focus or desktop control was used. Retained examples:

- [Final damage frame](screenshots/slice07/hud-impact-hit-feedback-tail-1280.png)
- [Player handoff](screenshots/slice07/hud-impact-hit-handoff-1280.png)

Review with `./cmake-build-release/blobber --combat-demo --seed 5`: use Defend to
reach enemy turns and try attacking during wind-up/strike/remaining splash. The
request must not execute; only after the splash and animation end can the next
player act. Player hit reactions likewise finish before the next attack, and
pausing midway cannot release the hold. Physical interactive review remains the
user's task. Slice 08 remains unstarted.

## Review correction: interpolated walking

2026-10-26 — implemented and accepted with slice 07.
User requested travel between cells using the existing walk animation. Checked
cross-cell moves now retain a 600 ms presentation segment; grid occupancy commits
immediately. Rendering follows the interpolated position for facing, perspective
size, grounding, shadow, sorting, occlusion and picking. Pauses freeze movement;
death freezes its position at impact. Contact locks exploration but waits for
arrival before facing the party and starting turns. Previous attack handoff holds
remain intact. No slice 08 work or asset regeneration was performed.

Design was recorded before implementation in the AI design. Two immutable image
representations are prepared: original stationary banks and full-pose travel
rasters capped at 256 pixels on the longest axis. Travel uses a fixed lighting
table and a prepared floor-shadow kernel. Combined decoded/prepared pixel storage
is 126,647,440 bytes (120.78 MiB), below 128 MiB; source images are released before
play. Additional storage is 8,183,808 bytes. Transient detail can differ slightly
from stationary art; cell collision remains discrete, without swept reservations.

Executed:

```sh
cmake --build --preset debug -j 4
env -u DISPLAY -u WAYLAND_DISPLAY ctest --preset debug --output-on-failure
cmake --build --preset release -j 4
env -u DISPLAY -u WAYLAND_DISPLAY ctest --preset release --output-on-failure
./cmake-build-release/blobber_npc_animation_tests --stress
./cmake-build-release/blobber --capture-ai /tmp/blobber-slice07-interpolation-final
```

Both builds passed without warnings; all 19 suites passed (Debug 30.61 s,
Release 4.22 s). Regression checks cover start/midpoint/arrival, subslot changes,
chaining and explicit moves, held-frame invalidation, missing walk art, pause,
death position, exact intermediate picking from all four headings, full wall
occlusion and exact stationary rendering on arrival. Combat begins exactly at
600 ms and cannot advance during pause. The allocation guard now renders visible
travel at 16 ms steps and asserts that a traveling body was actually picked;
zero engine-owned C++ allocations. That strengthened guard and animation suite
were rerun after the final diagnostic additions.

The separate generated-fixture stress case presents 256 traveling actors over
120 CPU compositions including picking: 87.948 ms total, 0.733 ms/frame on this
host. This is a rendering workload measurement, not a GPU, full-frame or
cross-platform performance guarantee; not every crowded actor is visible at once.
Navigation stress results above remain unchanged.

All 59 final native 1920×1080 application-owned hidden-window captures exported
and shut down cleanly. Inspected lateral start/midpoint/arrival, approaching
bodies, and the final travel frame versus combat entry. Trace confirms lateral
x=7.25 → 7.75 → 8.25 at 1200/1500/1800 ms. Frontal contact waits through 3050 ms
and starts turns at 3100 ms when the goblin reaches its destination. The earlier
capture pass sampled several times before walking started; final diagnostics now
explicitly sample the actual travel interval. No OS input or focus control.

Retained review evidence:

- [Lateral midpoint](screenshots/slice07/ai-travel-1500.png)
- [Arrival and player turn](screenshots/slice07/ai-approach-3100.png)
- [Logical/presented positions and combat phases](screenshots/slice07/ai-walk-trace.tsv)

Review with `./cmake-build-release/blobber --ai-demo --seed 5`. Physical interactive
play remains unverified by the agent. Slice 07 is accepted and closed.

## Acceptance record

Accepted on 2026-10-26. The user said: “slice accepted, update the document”.
This closes slice 07, including moving goblin AI, attack-turn presentation holds,
and interpolated cross-cell walking with arrival-gated combat. No additional
conditions or new work were requested. Existing verification and scale limitations
remain recorded; no unexecuted checks are reclassified as passing. Slice 08 is
next eligible, remains Planned, and has not started. Milestone B remains open.

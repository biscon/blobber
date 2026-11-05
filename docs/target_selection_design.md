# Enemy target selection

Status: implemented and mechanically verified (2026-11-03); interactive acceptance remains user review.

## Responsibilities and data

The visibility pipeline publishes two fixed-capacity lists in TargetSnapshot:
visible body IDs/anchors for pixel picking and presentation, and geometry-exposed
candidate IDs for commands. An alive actor is a candidate if an alpha-covered
billboard sample inside the viewport passes opaque dungeon depth, independently
of other actors. This uses the existing triangle walk and alpha mask; there is no
additional screen buffer or raster pass. Walls, doors and opaque props remain
occluders. Candidate bookkeeping is bounded by NpcWorld::maxInstances.

CommandController normalizes both lists against world/revision/life, unions visible
IDs into candidates (supporting existing input clients), and orders candidates by
squared subcell distance then slot. Combat retains authoritative range, LOS, rank,
turn and resource checks. Geometry candidates replace body visibility only for
command targeting. Visible anchors never become valid for a hidden body. The
windowless visual provider publishes the same snapshot as gameplay.

## Interaction

Tab / Shift+Tab cycle eligible individual enemy targets nearest first; Left/Right
remain aliases. No cycle commits or assigns multi-target recipients. Single-target
spell focus is sufficient for existing Enter confirmation. Ground/area/ally spells
and modal Tab controls retain their current controls. F2 replaces Tab as the
movement-animation toggle. Input edges suppress holds and focus-return replay.

A Targets button in the dock footer opens a non-pausing paginated popover with at most four rows
above the action dock, shrinking for shorter lists. The same ordered candidates feed cycling and rows, while
rows also explain ineligible candidates. Names include stable slot numbers and
party-relative front/rear rank. Hover outlines use only the existing body raster;
fully obscured selection uses text. Selecting a row previews a single target;
multi-target row activation uses existing toggle/dart assignment semantics.
Attack/Fire/Cast or Enter confirms. Rejection preserves the popover and reports a
reason. Escape/outside click/right-click outside recipient controls dismisses and
consumes input. Action/caster/turn/world/movement/focus/modal changes close it.
No save or asset changes. Fixed row/readout buffers and existing bounded controls
avoid frame allocations. Button identity includes actor world/slot and context
revision so a pending physical click cannot be redirected by a changed list.

## Verification strategy

Owned small generated fixtures cover fully covered actors, walls/doors, range and
front protection, forward/reverse eligible cycling, no candidates, paging, stale
IDs, selection preservation, single/multi-target spells, assignment limits,
dismissal, release activation, context changes and focus. Extend steady-state
allocation checks. Run affected visibility, commands, combat, spell, HUD/button
and headless observation suites sequentially in one configuration. Inspect hidden
application-owned captures at 1920x1080 and 1280x720; no desktop input automation.
Stress tests and benchmarks are not part of this correctness change.

## Delivered interfaces and compatibility

TargetSnapshot retains body `ids`/`count`/`anchors` and adds `candidates`/`candidateCount`.
PickingBuffer records geometry exposure during its existing raster pass; finish
publishes both lists with the same actor world/revision. CommandController owns
bounded four-row EnemyTargetPicker data, selection/cycling and eligibility queries;
the separate target_commands unit owns that behavior. The HUD draws the popover
from shared layout rectangles, with release-activated controls and consumed outside
input. Hidden candidates never acquire valid body anchors or floating tooltips.

Windowless observations and action descriptions expose the same candidates as
commands; their existing `targetable` flag now refers to geometry exposure. Existing
body-only internal input clients remain supported by normalization. Visibility
identity is world-visibility-v11; protocol/save schema, simulation, prepared asset
format and image content are unchanged. Strict older visual traces reject the new
visibility identity. F2 now controls movement animation; Tab is targeting input.

## Executed verification

Release game, capture diagnostic and affected test executables build successfully,
with two low-priority workers and no compiler warnings. Twelve selected suites
pass: targeting, commands, combat, spells, expanded_spells, hud, buttons,
visual_review, world_visibility, automation, automation_allocations and world_visual_cli.
The final targeted reruns cover the last picker feedback/readout changes and the
visibility-version replay expectation. No broad configuration matrix, sanitizer,
benchmark or capacity-stress investigation was run.

The owned targeting fixture verifies full sprite occlusion without direct picking,
bow/single-spell submission, multi-target focus and dart limits/removal, walls and
closed doors, protected melee ranks, out-of-range cycling, paging, empty/invalid
snapshots, stale/dead selections, focus edges, rejection/dismissal input ownership,
physical row release and cancellation after actor revision changes. Its C++ allocator
guard covers candidate refresh, eligibility, cycling and picker operations; existing
world and automation allocation guards also pass. This is an engine allocation
claim, not a claim about graphics drivers or third-party internals.

The opt-in blobber_capture_targeting diagnostic loads production art/rules in a
hidden window, checks rear archer bow eligibility and pending spell assignment,
and checks OpenGL errors. Four scenes export at 1920x1080 and 1280x720: bow picker,
Tab-selected bow target, blocked melee picker and partial Magic Missile assignment.
[Captures and reproduction](screenshots/targeting/README.md) record the visual review.
No OS mouse/keyboard input, focus selection or desktop automation was used. Native
and reduced captures were inspected; interactive acceptance remains unverified.

## Delayed creature hover — 2026-11-05 (implemented)

Creature tooltips and their hover health bars require 250 ms of uninterrupted
eligible hover on the same world-scoped ActorId. CommandController owns one optional
identity and a session-time timestamp; HudHover publishes a tooltipVisible flag.
Hover outlines, eligibility previews and command selection remain immediate.
Small pointer moves within the same creature do not restart the delay. Changing
creatures, leaving, suspension/modal UI, invisibility or an active attack resets it.
Any retained party attack presentation, or the hovered creature's attack presentation,
suppresses the tooltip until its existing endMs deadline; then a fresh 250 ms delay
begins. Walking and the existing damage flash also suppress it. Unrelated enemy
attacks do not suppress an idle creature. Session time provides deterministic,
pause-aware timing; no allocation, platform clock or gameplay mutation is introduced.
Other HUD tooltips keep their existing behavior. Verify delay boundaries, target
changes, attack acceptance, recovery, focus/modal reset and immediate selection.


Verification: Debug `hud`, `targeting` and `melee_swipe` CTest suites pass. The new
HUD regression uses real accepted party/enemy attacks and verifies 249/250 ms
boundaries, within-creature movement, target changes, leaving/re-entering, focus
reset, immediate suppression on acceptance, unrelated idle enemies, fresh delay
after attack completion, other HUD tooltips and allocation-free frame refresh.
Hidden GPU integration also passes. Hover health bars share the tooltip visibility
flag; outlines and click/keyboard targeting retain immediate feedback. Interactive
feel is left to the user. Builds used two workers, tests one process, at low
CPU/I/O priority. No benchmark or broad configuration matrix was run. Disposable
logs in run `20261105T164048Z-af4c87c526ce4a8e8cdac1f74ab09bd1` were previewed and
cleaned; no new screenshots or retained diagnostics were needed.

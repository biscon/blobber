# Enemy target selection

## Encounter-scoped picker — 2026-11-07

Implemented and mechanically verified; interactive acceptance pending. Targets is available only in Contact, Facing, Turns
and RoundEnd, with existing action/modal/input gates. Exploration, Approach and
terminal phases hide and close it. This applies to Tab/Shift-Tab and enemy arrow
cycling too; direct world clicks and ordinary visibility/inspection remain unchanged.
Hiding Targets during exploration does not restore the removed dock status line.

CommandController filters normalized geometry candidates through live
CombatController membership and actor life. EnemyTargetPicker owns a fixed
maxInstances array/count in nearest-first order, separate from renderer targets.
Rows, page counts/navigation, cycling and picker confirmation share this list.
Existing range, LOS, turn, rank and resource rules still decide eligibility.
No new distance cap or world visibility behavior is introduced. Filtering is one
linear scan of normalized candidates; no per-candidate visibility scan is needed.

Refresh clamps pages and increments the picker context revision when its list or
availability changes, even without an actor revision. Pending row presses then
cancel. Activation revalidates the captured actor before refreshing rows, so an
old row cannot redirect to its replacement. Picker confirmation revalidates every
hostile recipient; failures spend nothing and do not retarget. Deaths and enrolled
reinforcements update the list; leaving combat closes it. Storage is bounded by
the existing actor capacity, with no frame allocations, save or protocol changes.

Verification: owned fixtures cover combat phases, unrelated visible enemies,
filtered paging/cycling, hidden bodies, membership-only changes, reinforcements,
deaths, stale clicks/confirmation and outside-combat direct attacks. Run focused
Debug targeting/HUD/buttons/commands suites with bounded workers and hidden
production captures; interactive desktop review remains manual.

Verification: Debug game and the capture diagnostic rebuilt with two low-priority
workers. `ctest --test-dir cmake-build-debug -j 1 --output-on-failure -R '^(targeting|hud|buttons|commands)$'`
passes all four suites in 0.13 s, including the existing zero-allocation targeting
guard. Initial development failures exposed two fixtures that assumed map-wide
pagination; they now distinguish direct world visibility from enrolled picker
targets. Tests exercise six enrolled and two unrelated visible enemies, filtered
pages/cycling, multi-recipient rejection without mana spend, death/stale input,
reinforcement enrollment, victory with an unrelated survivor, Exploration/Approach
exclusion and a combat transition with an unchanged renderer snapshot. Existing
covered-body, range/rank, spell, focus/modal and button-release checks still pass.

`blobber_capture_targeting cmake-build-debug/assets RUN_DIRECTORY/captures`
passes with a hidden OpenGL window after granting display access. Production
checks confirm three visible geometry candidates but only two encounter rows,
outside-combat button removal, bow eligibility, partial dart assignment and no
OpenGL errors. Five scenes export at 1920×1080 and 1280×720. Native exploration/bow
and reduced spell captures were inspected; [six selected images](screenshots/targeting/README.md)
replace the previous targeting evidence. The earlier action-dock exploration image
was refreshed to reflect the combat-only button. Interactive desktop review remains
unverified. Temporary captures/build/test logs were previewed and cleaned, with no
retained diagnostic exceptions. No benchmark or additional build configuration ran.

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

During active combat, a Targets button in the dock footer opens a non-pausing paginated popover with at most four rows
above the action dock, shrinking for shorter lists. The same ordered encounter candidates feed cycling and rows, while
rows also explain ineligible candidates. Names include stable slot numbers and
party-relative front/rear rank. Hover outlines use only the existing body raster;
fully obscured selection uses text. Selecting a row previews a single target;
multi-target row activation uses existing toggle/dart assignment semantics.
Attack/Fire/Cast or Enter confirms. Rejection preserves the popover and reports a
reason. Escape/outside click/right-click outside recipient controls dismisses and
consumes input. Action/caster/turn/world/movement/focus/modal changes close it.
No save or asset changes. Fixed row/feedback buffers and existing bounded controls
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


## Spell targeting inspection — 2026-11-06

While choosing spell recipients, retain the normal delayed enemy health bar and
show current/max HP in the spell hover tooltip alongside the existing spell name,
eligibility and selection instructions. Keep the full enemy name/number. The
existing fixed `HudHover` lines and health ratio remain owned by CommandController;
remove only the renderer's casting exclusion on the health bar. No new storage,
allocation, picking, target mutation or clock is introduced. Ordinary 250ms hover,
visibility/depth gates and attack/resolution suppression remain in force.

Verify routed pointer hover during single-target, multi-target, dart allocation and
ground-area selection with a wounded enemy; hover must reveal health without
changing selected recipients, assigned darts, aimed cell or resources. Existing
HUD delay/attack-suppression tests cover the shared dwell behavior. Run focused
`targeting` and `hud` checks and rebuild the application; no new screenshots.

Verification: Debug application rebuilt; `targeting` (0.07s) and `hud` (0.02s)
pass. The new regression routes actual hover input while targeting Fire Bolt,
Acid Splash, Magic Missile and Sleep, checking the normal delay, enemy identity,
25/100 HP, health ratio, spell instructions and unchanged pending targets/darts,
ground choice and mana. Existing dwell/attack-suppression checks also pass.
No new captures or interactive desktop verification; temporary output was cleaned.

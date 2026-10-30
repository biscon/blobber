# Slice 13 verification and handoff

Status: **Accepted and closed**, 2026-10-30.
Implementation was authorized by “Implement the plan.” Prerequisites 12.4 and 12.5
were accepted. The user accepted slice 13 and milestone C, including the review
corrections below: “I accept the slice, record it in the documentation.”

## Delivered

[Progression design](progression_design.md) records the approved scope and final
ownership, interfaces, capacities and adaptations. The live party earns exactly-once
monster XP, split among non-dead companions in exact twelfths, capped at 900.
Thresholds 300/900 unlock the checked level 2/3 flow. Level-up preserves identity,
equipment, supplies, spell sources, resource deficits and existing spent charges.
It adds only earned HP/mana capacity, Hit Dice, features and spell selections.
Library and arena starting snapshots remain level one; a fresh run/rematch resets XP.

Character sheet and portrait eligibility lead to Overview → Class features → new
spell choices → Review. Existing learned/prepared spells are preserved and summarized;
only new choices appear in the advancement list. Wizard learns two spells per level; preparation may add
one. Life Domain grants its two new spells separately. Cancel is free, and changed
session/revision/level/safety state rejects stale commits. `--development` exposes
an explicit pause-menu XP grant for review, without automatic advancement.

Fighter gains Action Surge and Champion; Cleric gains Preserve Life; Rogue gains
Cunning Defend, Thief potion use and 2d6 Sneak Attack; Wizard gains Evocation/Sculpt
Spells and second-level casting. Optional activations work before or after ordinary
actions, with fresh command tokens and End Turn. Reopening input does not replay
turn effects or reset allowances. Existing rest restores the appropriate charges.

Aid, Hold Person, Lesser Restoration, Spiritual Weapon, Scorching Ray, Shatter,
Mirror Image and Web use the normal targeting, resource, effect and impact paths.
Mirror duplicate reservations resolve at impact; Web has bounded cells/restraints,
escape, movement penalties, concentration and delayed fire destruction. Spiritual
Weapon has a visible independent position and limited free follow-up. Shatter
includes friendly fire with Sculpt protecting the other three companions.

Schema 9 adds typed class progression and monster XP/humanoid fields; schemas 1–8
keep zero XP, non-humanoid and unsupported-progression defaults. Library/scenario
formats stay unchanged. Prepared bundle format/renderer revision is 3. Seven new
spell masters and two world-effect masters retain prompt provenance; Hold Person
reuses its existing image. The 36-tile creator atlas stays 16 MiB, with eight GPU
texture slots. Floor and spectral images are prepared offline.

## Verification

- Full Debug and Release CTest: **41/41 each**. Logs:
  `/tmp/blobber13-final-debug-tests.log`, `/tmp/blobber13-final-release-tests.log`.
  A final four-entry render-sort cleanup was followed by **5/5** focused render,
  prepared-asset, projectile and allocation checks in each build, plus regenerated
  captures; `/tmp/blobber13-render-{debug,release}-tests.log`.
- Focused ASan/UBSan with leak detection: **8/8** (`progression`, `level_spells`,
  `effects`, `spells`, `recovery`, `creator`, `library`, `arena`), in
  `/tmp/blobber125-sanitize`; `/tmp/blobber13-final-sanitize-tests.log`.
- Existing frame allocation guard and new first/repeated progression/spell/effect
  workload: **zero tracked engine-owned C++ allocations**. This covers checked
  preview/commit, stale rejection, modal open/close, sheet refresh, all eight casts,
  duplicate reservations and repeated Web creation/burning. It does not claim that
  graphics drivers or third-party C allocations are absent.
- Fixture-owned rules/progression checks cover all sixteen race/class combinations
  through level 3, spell grants, HP/mana deficits, identity/inventory preservation,
  rest-token invalidation, exact fractional XP, dead/unconscious shares, removed
  corpses and duplicate death credit; UI tests cover Wizard selection, cancellation,
  stale/double commit and new choices before locked spells.
- Spell/feature tests cover all eight checked commands and costs, repeated requests,
  three delayed rays, spectral movement/cooldown, Sculpt self-harm, Aid refresh,
  inventory derivation and advancement/expiry, duplicate reservation/cancellation,
  Web creation/restraint/burning/leaving, Preserve Life and both action orders,
  unchanged turn clock, stale tokens and Thief's shared potion/Defend allowance.
- Separate Release capacity stress: **256 actors, 16 Web cells, 64 restrained NPCs,
  full burn batch and exhausted 64-effect table**. Passes with 196 events in the
  measured batch; `/tmp/blobber13-stress.log`. The design records storage/event
  bounds; this workload does not establish all encounter-scale timing behavior.
- Hidden raylib/OpenGL GPU lifecycle check passes on NVIDIA RTX 3060, including
  eight-slot resource lifecycle; `/tmp/blobber13-gpu.log`.
- Initial verification: **69 application-owned captures**, 23 states at 1920×1080, 1280×720 and
  letterboxed 1024×768. [Gallery](screenshots/slice13/index.html), refreshed and expanded
  by the picker correction below. Capture trace
  exercises all four live identities through two advancements, level-three sheets,
  new/locked selections, rest preparation, Web/spectral weapon rendering and a
  fresh level-one/zero-XP rematch reconstruction. Selected review, spell selection,
  class-feature, character-sheet and world-effect captures were visually inspected.
  `/tmp/blobber13-captures.log`.
- Production bake: scene resident 168,288,576 bytes, total bake resident
  237,298,670 bytes, tracked scene loading high-water 189,434,280 bytes, within
  configured limits. Runtime loaded scene is smaller because loaded vectors use
  exact serialized sizes. `/tmp/blobber13-final-debug-build.log`.

During verification, an inventory re-derivation could drop Aid's effective maximum;
all inventory mutations now preserve the bonus. A Web created after earlier effect
clock advancement could reuse a past hazard deadline; creation now schedules a
future boundary. Regression checks cover both. The first final capture attempt
ran while the asset bake was replacing metadata and correctly rejected the incomplete
bundle; captures were rerun after the completed build.

Existing missing-field-initializer warnings in commands_tests remain.
Deterministic captures and automated checks do not establish encounter balance or
exhaustive interactive usability. Interactive desktop review remains unverified and
belongs to the user. No OS input, window-focus switching or desktop automation was
used. Unsupported features remain explicitly disclosed in the design/source ledger.

## Review recipe

1. Run `cmake-build-debug/blobber --development`. Start a normal or arena party,
   including a saved custom character. Pause → **DEV: Grant next-level XP**.
2. Open each Character sheet → **Level Up**. Review fixed class features, exercise
   Cancel, then confirm level 2. Wizard learns the two remaining level-one spells;
   existing learned/prepared spells are preserved and omitted from new choices. Verify identity, inventory and
   deficits remain, with no fresh starting supplies.
3. Grant XP again and advance to level 3. Wizard selects two of five new spells and
   may prepare one; Cleric receives Lesser Restoration/Spiritual Weapon separately.
   Review the named spell selections and level-three resources/features.
4. In combat, try normal action → optional activation and the reverse. Use Action
   Surge's second action, Cunning Defend or a Thief potion, and End Turn. Check
   Preserve Life's preview and half-maximum healing; rest restores spent charges.
5. Try the eight new spells: choose Lesser Restoration's supported condition, assign
   Scorching rays, inspect Shatter's Sculpt preview, move/attack with Spiritual
   Weapon, observe Mirror duplicate counts, and escape/leave/burn Web. Use Long Rest
   to replace preparations and try additional spells. Aid must not heal on refresh.
6. Defeat goblins for ordinary XP. Rematch/restart must restore the original
   level-one party. Review the full milestone C loop: menu/custom creation, combat,
   inventory/equipment, loot, rest and advancement.

Headless: `ctest --test-dir cmake-build-debug --output-on-failure`.
Capacity: `cmake-build-release/blobber_level_spells_tests --stress`.
Capture: `cmake-build-debug/blobber --capture-progression /tmp/progression-review`.

## Next-agent handoff

Review correction (2026-10-30): the documented `--development` flag was missing
from argument parsing, despite its session wiring and help text. Added the parser
branch and regression coverage for default-off behavior, the argument-free flag,
following options and unknown-option rejection. The actual executable also runs
`--development --help` in headless CTest. Rebuilt Debug and Release; focused
`options`, `development_cli` and `progression` checks pass **3/3 in each build**
(`/tmp/blobber13-cli-{debug,release}-tests.log`). Earlier full-suite results above
remain the preceding baseline. This correction was accepted with slice 13.

Read this handoff, the current roadmap checkpoint and progression design before
further work. Slice 13 and milestone C are accepted; slice 14 requires its own
planning and implementation approval. Keep live progression separate from
level-one library/scenario blueprints. Future save/resume must preserve XP fractions,
actual current build/grants/resources and permanent death credit, while rebuilding
transient effects/handles under its separately approved design.


## Spell-picker review correction (2026-10-30)

User approved keeping the new preparation optional and requested clearer slot
wording, only actionable advancement choices, blue card outlines without checkmarks,
removal of L controls and redundant Up/Down buttons, tighter description spacing,
and equal-height aligned list/detail panels. These changes apply to the shared
level-up, Long Rest and character-creation picker. An empty preparation explicitly
says **Leave slot empty**. Brunna's level-two page explains the additional slot,
unchanged preparations/domain grants, Sanctuary as the only remaining choice and
second-level spells unlocking at character level three. Existing preparations can
only be replaced during Long Rest.

Scrollbars appear only on overflow and support wheel scrolling, track paging and
captured thumb dragging, including clamping outside the track and cancel/release.
Description ranges use measured wrapped lines; short descriptions now fit without
controls. Editable whole-card clicks toggle and inspect; budget enforcement remains
in the rules layer. Grants use plain labels in creation/rest, with no L controls.

Focused Debug/Release **8/8 each**: creator, progression, progression_allocations,
creation, frame_allocations, recovery, menu and arena. Final cancel-during-drag
cleanup: **3/3 each** (creator/progression/progression_allocations). Final focused
ASan/UBSan with leak detection: **2/2**. Logs are
`/tmp/blobber13-picker-{debug,release}-tests.log`,
`/tmp/blobber13-picker-final-{debug,release,sanitize}-tests.log`.
Allocation guards still report zero tracked engine-owned C++ allocations, now
including shared list/detail scrolling. Regression tests cover whole-card actions,
scroll limits/drag lifecycle, omission of locked baseline cards, the sole Cleric
choice and leaving a slot empty without losing earlier preparations.

Refreshed **87 progression captures** and **187 creator captures** use hidden
application-owned windows. The gallery retains all 87 progression captures plus
12 selected creator spell-picker captures. Native/reduced/letterboxed empty and
selected advancement, level-two rest, Wizard choices and creator preparation were
inspected. The compact Web description fits fully; its capture verifies that
scrolling cannot move beyond the measured range. Overflow/drag behavior uses a
fixture-owned measured-line case in the headless suite. Logs:
`/tmp/blobber13-picker-captures.log`, `/tmp/blobber13-picker-creator.log`.
Both normal executables are rebuilt. The user subsequently accepted these
corrections with slice 13 and milestone C.

## Single selection indicator correction (2026-10-30)

The gold inspected-card border and additional focused-card outline made an
unselected spell look selected alongside the blue actual pick. Removed both:
only selected spells receive a blue outline, and recommendations have no separate
marker. In a one-slot picker, clicking another eligible spell immediately replaces
the pick; clicking the selected spell clears it. Recommended picks selects its
result and displays a selected spell's description. Multi-pick limits and locked
grants are preserved.

Debug/Release focused creator, progression and allocation checks pass **3/3 each**;
`/tmp/blobber13-selection-{debug,release}-tests.log`. Tests reproduce recommendation,
replacement without first deselecting, clear/reselect and repeated recommendation.
The test chooses an actual alternative from its fixture rather than assuming its
first card differs from the recommendation. Both executables are rebuilt. Hidden
progression/creator captures verify the single outline and synchronized recommendation
preview; `/tmp/blobber13-selection-{captures,creator}.log`. The gallery includes a
new Wizard preparation-replacement state. This correction was accepted with slice 13.


## Accepted closeout and next-agent handoff

The user explicitly accepted slice 13 on 2026-10-30: “I accept the slice, record it
in the documentation.” This closes levels 2–3 and milestone C's combined RPG loop,
including the development-flag fix, shared-picker presentation/scrolling changes,
optional preparation wording and single selection indicator/replacement behavior.

The user confirmed the existing Action Surge implementation is fine after the
class-feature activation path was explained. Preserve the static action bar and
explicit activation through Class Features: one additional normal action before
or after the ordinary action, once per Short/Long Rest; End Turn preserves an
unused charge. No further Action Surge UI revision is part of this closeout.

Recorded test results and limitations remain unchanged. This acceptance update
changes documentation only; no gameplay builds, tests or captures were rerun.
User acceptance does not claim exhaustive balance or interaction testing.

Slice 14 — Save and resume — is next eligible for separate planning and remains
Planned/unstarted. Read AGENTS.md, the system overview, roadmap, this handoff and
relevant code before proposing its plan. Obtain approval of that plan before
implementation. Preserve accepted creation/arena ownership, live progression and
spell grants, independent library/scenario snapshots, checked transactions,
generation-safe transient handles, prepared rendering and bounded allocation
contracts. This closeout does not implement or start save/resume.

## Enemy status-icon occlusion follow-up (2026-10-30)

Post-acceptance correction requested after the closeout above. **Accepted and
closed**, 2026-10-30. User: “yes that works, if this needs to be recorded as accepted
do so”. The prior slice 13/milestone C acceptance remains recorded. No review
remains pending; slice 14 has not started.

Enemy condition icons previously painted over the entire dungeon as HUD content.
They now use their owner's actual projected depth and the existing alpha-aware
body IDs to clip against nearer enemies (including visible corpses) and wall
columns. The owner and farther enemies do not obscure its icon; transparent gaps
remain visible. Native-resolution art, stable anchors, non-overlap layout and body
picking remain unchanged. The same path covers Sleep and every other enemy status.
Storage adds about 3 KiB per picking buffer; bounded tile masks/scissor runs use no
frame allocations or new GPU resources.

Focused Debug and Release suites pass **8/8 each**: actor_render, picking,
visual_review, frame_allocations, progression_allocations, render, npc and
loot_render. The allocation guards report zero tracked engine-owned C++
allocations, including status clipping. Regression fixtures exercise reversed
slot/depth order, transparent silhouettes, travel offsets, corpse occlusion without
corpse targeting, nearer walls, farther bodies, exact clip coverage, invalid bounds
and metadata reset. An initial test incorrectly assumed the center of a corpse's
bounds could not pick the living actor through a transparent hole; it now asserts
the actual target membership. Final logs:
`/tmp/blobber-status-depth-debug-final-tests.log`,
`/tmp/blobber-status-depth-release-tests.log`.

Hidden GPU lifecycle checks pass (`/tmp/blobber-status-depth-gpu.log`). No OS input
or window-focus automation was used. The user subsequently accepted the result.
The separate suggestion to allow rear-party melee spells to reach only front enemy
subcells was discussed; this rendering correction does not alter spell reach.

Both normal executables were rebuilt. The final hidden progression capture run
completed **96 images**, including six new occluded/revealed status captures at
1920×1080, 1280×720 and 1024×768. Native and letterboxed captures were visually
inspected: nearer silhouettes clip the overlapping icon portions, and removing the
front pair reveals the same four icons. The six new images are retained in the
[gallery](screenshots/slice13/index.html); log:
`/tmp/blobber-status-depth-captures.log`. The capture fixture uses a legal Sleep cast
before moving the standing pair into contact with checked one-cell moves; initial
fixture attempts rejected casting out of turn and moving multiple cells at once.
These fixture corrections did not change gameplay rules.

This follow-up acceptance update changes documentation only. Recorded verification
results and limitations remain unchanged; no builds, tests or captures were rerun.

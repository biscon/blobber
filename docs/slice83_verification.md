# Slice 8.3 verification and handoff

Status: **Accepted and closed**, 2026-10-27. The user approved implementation with
“Implement the plan” and accepted the completed slice with “slice accepted, update
doc”, including the front-cell spacing correction. Verification limitations remain
recorded below. Slice 09 remains Planned and is next eligible for planning.
See the [implemented design](projectile_impact_design.md). The initial verification
and capture sets below precede the [center-cell spacing review correction](#review-correction-center-cell-pair-spacing);
use that section for the latest placement screenshots.

## Delivered behavior

Party and enemy arrows travel in both directions with perspective scaling, a
small arc, distinct hit/miss endings and a neutral recipient portrait marker.
Archer release follows the authored 600 ms marker; flight lasts 180–450 ms by
distance. Party melee has a 200 ms windup and brief slash. NPC melee uses its
existing strike marker. Costs and rolls happen once at acceptance; health, life,
occupancy, reactions and result feedback commit together at impact.

Incapacitation cancels unreleased attacks without refund. Nonlethal hurt pauses
and resumes NPC windup, with overlapping hurt intervals counted once. Released
arrows survive shooter death. Movement and later cover never reroll an attack;
dead/removed targets are never replaced. Pending attacks and their presentation
tails gate turn handoff, facing and results. Pause freezes the timeline. Cosmetic
capacity cannot drop authoritative consequences.

Rendering retains accepted geometry, profile C, both appearance IDs, source-free
loading and the fixed 1920×1080 presentation. Optional attachment metadata shares
body projection, including inward diagonal placement. Original PNGs are untouched.
No spells, concentration, ammunition or general effects framework were added.

## Checks executed

From the repository root:

```sh
cmake --build --preset debug -j4
cmake --build --preset release -j4
ctest --preset debug --output-on-failure
ctest --preset release --output-on-failure
ctest --preset debug -R '^(ai|combat|npc|impact|projectile|frame_allocations)$' --output-on-failure
cmake-build-release/blobber_impact_tests --stress
cmake --build --preset release --target blobber_gpu_smoke -j4
cmake-build-release/blobber_gpu_smoke
cmake-build-release/blobber --capture /tmp/blobber83-broad
cmake-build-release/blobber --capture-projectiles /tmp/blobber83-review-captures
```

Full Debug **24/24 passed** (109.26 s); full Release **24/24 passed** (12.46 s).
After final feedback-anchor, attachment-validation and AI RNG-comparison fixes,
Release **24/24 passed** again (11.93 s) and affected Debug **6/6 passed** (25.85 s).
The separate stress entry point and capture cases were subsequently added; both
builds succeeded and routine impact tests passed again in Debug and Release.
The final startup budget guard rejects combined scene/projectile overflow before
subtracting the NPC allowance; both applications were rebuilt and source-free
startup was rechecked. Compiler logs contain no warnings. `git diff --check` passes.

Tests use generated, test-owned fixtures. Coverage includes exact strike/release/
arrival/hold boundaries; hit, miss, critical and lethal results; no early HP, log,
reaction or occupancy change; one-time costs and RNG; chronological simultaneous
impacts; repeated hurt/resumption; incapacitation; released shooter death;
removed/dead/downed recipients; movement/turning; pause; contact, cooldowns and
phase transitions; large updates; mutual elimination; Continue and restart.
Renderer checks cover both directions, wall occlusion, unchanged body picking,
all movement transitions, pause caching, expired effects, diagonal anchors and
independently exhausted cosmetic capacity. The allocation guard includes live
impacts, melee slash and incoming arrows on the loaded two-appearance bank:
**zero tracked engine-owned steady-state frame allocations**.

The optional `--stress` check is separate from CTest. It fills all **260 attacker
slots**, drains **520 release/impact events**, and confirms a second drain adds
nothing. It repeats 100 bursts with the maximum 256-actor population. The setup
visits different viewpoints before draining, deliberately exercising the storage
bound rather than claiming ordinary AI can put every actor in one firing cone.
Release drain mean **233.424 µs**, observed maximum **342.699 µs**, on this machine.
These are bounded-timeline measurements, not whole-game frame-time guarantees.

Focused sanitizer checks were rebuilt and rerun after the final fixes:

```sh
cmake --build /tmp/blobber-81-sanitizers --target \
  blobber_impact_tests blobber_projectile_tests blobber_combat_tests blobber_hybrid_tests -j4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/blobber-81-sanitizers \
  -R '^(impact|projectile|combat|hybrid)$' --output-on-failure
```

ASan/UBSan **4/4 passed** (27.79 s). Leak detection remains unverified and explicitly
disabled because of the previously established tracing/ptrace restriction.
Third-party raylib is not sanitizer-instrumented.

Hidden GPU lifecycle, rendering/readback and cleanup passed on the available
NVIDIA RTX 3060 / OpenGL 3.3 environment. Display access required an authorized
sandbox escalation after the initial sandboxed run could not open `:0`. No OS
input injection, focus changes or desktop automation were used.

The broad suite completed **292 PNGs**; the inherited victory-capture failure did
not recur, and its assertion was not weakened. All **183 shared preparation
captures** compared against `/tmp/blobber-82-captures` are byte-identical. The new
projectile suite produced **380 PNGs**, native and 1280×720, with an accompanying
[timestamp/HP trace](screenshots/slice83/projectiles.tsv).

The source-free tree `/tmp/blobber83-runtime-only` contains definition/level JSON,
prepared banks, rules, UI/fonts, preparation profile and maze; it contains **zero
NPC PNGs and no texture directory**:

```sh
cmake-build-release/blobber --assets /tmp/blobber83-runtime-only --check-prepared
cmake-build-release/blobber --assets /tmp/blobber83-runtime-only \
  --capture-projectiles /tmp/blobber83-runtime-captures
```

Source-free validation passed. All **380/380 PNGs and the timing trace** match
the normal run byte-for-byte. All eight native/reduced paused-frame pairs are also
byte-identical to their pre-pause flight frames.

## Memory and preparation

| Prepared resident category | Bytes |
| --- | ---: |
| Scene bank | 114,664,792 |
| NPC bank and attachment metadata | 66,637,718 |
| Immutable compiled projectile rasters | 1,012,160 |
| Combined | **182,314,670** |
| Resident limit | 536,870,912 |

The raster bank is under 1 MiB and does not allocate or resample at runtime.
Attachment metadata adds 2,048 bytes to the previous NPC accounting. Existing
prepared bank layout is unchanged; the NPC definition fingerprint changes because
its JSON now includes attachments. FramePipeline also owns a second reusable
500×282 picking scratch buffer for projectile occlusion during transitions. These
CPU buffers and fixed attack/event records are separate from prepared-image
accounting. There is no unbounded queue or per-shot allocation.

## Visual review

Selected native and reduced captures were inspected for direction, distance,
release placement, diagonal bodies, movement, hurt/resumption, lethal impacts and
portrait attribution. Incoming arrows grow toward the lower viewport; the neutral
blue recipient outline disappears at arrival, when the actual hit/miss result and
health are shown. Hurt/death flash and the existing damage splash can obscure a
small distant body briefly. The user has accepted the result. Static captures alone do not establish
interactive readability or pacing; no agent-driven desktop review was performed.

Every reduced projectile link below has a native sibling with `-720` removed.

| Case | Selected captures |
| --- | --- |
| Incoming far arrow and HP boundary | [Launch](screenshots/slice83/incoming-1-seed5-launch-720.png), [flight](screenshots/slice83/incoming-1-seed5-flight-720.png), [before impact](screenshots/slice83/incoming-1-seed5-before-impact-720.png), [impact](screenshots/slice83/incoming-1-seed5-impact-720.png) |
| Outgoing near arrow / far miss | [Flight](screenshots/slice83/outgoing-0-seed5-flight-720.png), [miss](screenshots/slice83/outgoing-1-seed12-impact-720.png) |
| Diagonal alignment | [Outgoing left](screenshots/slice83/outgoing-2-seed5-flight-720.png), [incoming right](screenshots/slice83/incoming-3-seed5-flight-720.png) |
| Movement during flight | [Party strafe](screenshots/slice83/incoming-4-seed5-moving-720.png), [NPC travel](screenshots/slice83/outgoing-6-seed5-target-moving-720.png) |
| Lethal / released shooter death | [Lethal impact](screenshots/slice83/outgoing-5-seed5-impact-720.png), [arrow survives shooter](screenshots/slice83/incoming-5-seed5-shooter-dead-720.png) |
| Windup interruption | [Hurt](screenshots/slice83/incoming-6-seed8-interruption-720.png), [resumed](screenshots/slice83/incoming-6-seed8-resumed-720.png), [canceled by death](screenshots/slice83/incoming-6-seed3-interruption-720.png) |
| Existing melee strike and portrait feedback | [Strike](screenshots/slice83/hud-impact-hit-strike-1280.png) |

Cases 0/1 are near/far, 2/3 left/right diagonals, 4 party strafe, 5 lethal outgoing
or released incoming shooter death, and 6 target travel or windup interruption.
Seeds 5/12 select ordinary hit/miss examples; incoming case 6 uses 8 for nonlethal
hurt and 3 for lethal cancellation. The trace confirms the nonlethal interruption
at 496 ms shifts release from 700 to 1000 ms, resumes at 796 ms, and arrives at
1396 ms. Seed 3 cancels at 496 ms. Case 1 also exports a paused frame after a
60-second paused update at the same session timestamp. Case 5 setup deliberately
reduces the outgoing target to 1 HP or kills the incoming shooter after release;
actual projectile consequences still come from the combat timeline.

For live review, run:

```sh
cmake-build-release/blobber --hybrid-demo --seed 5
```

Use Merrin's ranged action against an archer, watch both shot directions, then
approach for melee. Review arrow readability and recipient attribution, the
200 ms melee windup, pause/resume, and hurt before bow release. The deterministic
capture command above regenerates the exact diagnostic cases without OS input.

## Next-agent contract

Read the design before modifying combat. Consume **all** `CombatEvents`, not only
`step`; the batch is borrowed until the next session update. `AttackStarted` spends
costs and hides the saved result. `Attack` is the authoritative impact event with
`occurredMs`; release/cancel are separate. Resolve all due existing attacks before
movement/AI and phase handoff. Flying impacts win ties over unreleased windups,
then attack serial orders ties. New automatic actions remain one per update.
Session time drives pending impacts, travel and feedback; hybrid gameplay time
drives cooldowns and round boundaries. Do not implement effects against the wrong
clock or reintroduce UI-owned delayed damage.

Future visual kinds can extend the small copied attack visual record and raster
compositor; gameplay consequences stay in combat. Preserve source-free metadata,
projection and cosmetic/gameplay capacity independence. Slice 8.3 is accepted and
closed. Plan slice 09 in a clean context when requested; it has not started.

## Review correction: center-cell pair spacing

The user's screenshot showed the two melee goblins in the center front cell
squeezed together. The original diagonal edge fit scaled every lateral coordinate,
including the center pair. `projectNpc` now leaves lateral ±0.25-cell center pivots
unchanged and compresses only distance beyond those pivots. The mapping remains
continuous through travel/strafe and retains the existing edge and depth blends.
Body, shadow, picking and projectile attachment projection share the correction.
Camera, scale, world occupancy and combat reach remain unchanged.

The rendered-body regression failed against the old mapping, then passed with
the fix. It checks ordinary half-cell perspective separation in both center rows
and all four headings, alongside the existing six-target diagonal, wall and travel
checks. No extra frame storage or allocation was added.

```sh
cmake --build --preset release -j4
cmake --build --preset debug -j4
ctest --preset release --output-on-failure
ctest --preset debug -R '^(npc|projectile|picking|actor_render|frame_allocations)$' --output-on-failure
cmake-build-release/blobber --capture-hybrid /tmp/blobber83-spacing-captures
```

Release **24/24 passed** (11.86 s); focused Debug **5/5 passed** (32.33 s), including
**zero tracked frame allocations**. Both builds pass without compiler warnings;
`git diff --check` passes. The hidden capture suite completes **320 PNGs**, including
64 new center-pair images covering both appearances, both rows, all headings and
idle/attack poses. Existing six-target front-band assertions pass. Rendering and
readback used the authorized hidden application window with no OS input/focus control.
Selected east-facing center pairs, bow poses and full front-band images were
visually inspected; the reported center-pair overlap is resolved. All six targets
remain visible/pickable, although fitting the occupied diagonal cells still permits
partial silhouette overlap there. This is not a general silhouette packing system.

- Corrected melee center pair: [native](screenshots/slice83/spacing/archer-roles-center-melee-1-near.png), [1280×720](screenshots/slice83/spacing/archer-roles-center-melee-1-near-720.png).
- Center rear row: [1280×720](screenshots/slice83/spacing/archer-roles-center-melee-1-far-720.png).
- Archer attack pair: [1280×720](screenshots/slice83/spacing/archer-roles-center-archer-1-near-attack-720.png).
- Full front band: [idle](screenshots/slice83/spacing/archer-roles-front-band-1-720.png), [attacks](screenshots/slice83/spacing/archer-roles-release-1-720.png).
- [Appearance/picking trace](screenshots/slice83/spacing/archer-trace.tsv).

Slice 8.3 returned to **Ready for review**, then the user accepted it with
“slice accepted, update doc” on 2026-10-27. It is now **Accepted and closed**,
including this correction. No slice 09 work has begun.

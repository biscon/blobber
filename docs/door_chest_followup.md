# Door surfaces, sprite clearance and walkable chests

Status: **Implemented and verified; awaiting user review**, 2026-11-02.
The design below was recorded before implementation.

The sprite-fitting policy below is superseded by
[stable creature dimensions and scenery occlusion](creature_occlusion.md).
Door surfaces/chest behavior and the historical verification remain recorded here.

Keep the reviewed door dimensions and hinge motion. Mirror only the reverse broad
face in U (left/right), so both rings occupy the free edge opposite the hinge.
Square door artwork needs an aspect-preserving UV crop on the taller leaf: retain
the latch-side edge and crop the hinge side; center any vertical crop. A shared
door UV query derives spans from physical leaf dimensions and source aspect.
GPU colour drawing uses those spans; CPU/GPU IDs retain the same box triangles.

The existing bounded billboard fit checks grid solidity but misses door leaves and
thick jambs. Add oriented-box horizontal ray clearance for the prepared frame's
actual door boxes, including intermediate swing states. Reject vertically disjoint
boxes and distant boxes cheaply. Fit visible alpha extents against the resulting
clearance, uniformly scaling about the floor anchor. If the initial anchor is too
close to a door, evaluate at most five lateral anchors (original, +/-1/16 and
+/-1/8 cell), constrained to its current visual half-cell. Select the best fit,
preferring the original anchor on ties. Candidate shifts must also keep the body
axis inside the viewport's two-pixel inset, preventing a fitting correction from
pushing an actionable target offscreen. This door-only presentation adjustment
never changes actor positions, movement, slots or simulation eligibility. With no
nearby intersecting door, preserve current anchored fitting exactly. CPU picking,
GPU drawing, shadows and attachment positions use the same final fitted billboard.
Use fixed arrays and bounded scans; no new frame allocation or GPU readback.

Keep chest mesh source units and half-size scale. During scene preparation derive
a single wallward offset from the rear extent of both variants, leaving a small
gap for the fully open lid; opening must not move the chest base. Transform both
colour/picking triangles together. Cache site/facing, contents and saved immutable
records remain unchanged. Maintain a one-cell physical placement reservation for
chests, while allowing player movement onto that cell. Keep NPCs out of furniture
cells to avoid another model/actor intersection. Reuse the existing Map occupancy
byte for physical-reservation and player-blocking bits; the public queries hide
the representation. Generation/fixture protections still use the physical footprint,
preserving existing seeded placements and fingerprints. Cages/racks stay blocking.

On a chest cell, F/checked access is available as it is for loose loot at your feet;
adjacent interactions retain their current rules. Opening, sealed rewards, empty
inspection, pickup, saved state and turn safety continue through existing commands.
No save schema or generator bump: current Underkeep saves rederive the new occupancy
policy on load. Strict replay retains binary identity checks; update visibility
identity for changed output. Do not rewrite the user's save.

Owned checks cover reverse-face latch orientation, aspect-preserving UVs, door
clearance across headings/swing states/walking samples, preserved subslot identity,
CPU/GPU picking, and allocation-free changed/idle views. Verify wall-side closed/open
chests, step-on/F/pickup/restore, NPC exclusion, furniture overlaps and generation
stability. Use the saved door position via a read-only copy for hidden native views
from both sides; use generated rat fixtures for deterministic clipping captures.
One intensive job, at most two low-priority build workers. Interactive acceptance
remains user review; no OS input, focus or desktop automation.

## Delivered implementation and verification

`proxyMesh` mirrors only the reverse broad face. `doorUvSpan` supplies the physical/
source aspect ratio to the world shader's fitted-door mode; all frame/edge mappings
and leaf geometry retain their established behavior. No source bitmap/model edits.
`doorClearance` tests horizontal rays against the same oriented door boxes used
for drawing and IDs. `fitDoors` adds the bounded candidate fit to existing wall/
viewport fitting. NPC directions still derive from the unadjusted actor position;
the fitted feet drive billboard drawing, picking, badge anchors and attachments.
An originally offscreen partial body can retain normal clipping; an adjustment
cannot push an onscreen body axis outside the viewport.

Map's existing occupancy byte retains bit 0 for player blocking and bit 1 for
physical furniture reservation. `movementBlocked` checks bit 0, `npcBlocked` checks
the reservation, and NPC navigation/movement/restore admission use the latter.
Placement overlap, dimension checks, fixture/stair exclusions and generator
admission retain the physical footprint. `connected` includes the newly walkable
chest cells. Cages/racks remain blocking. This policy is reversible without changing
the stored world format. Same-cell container access and F reuse the existing safe
checked commands, including sealed/empty/partial contents and loot ownership.

The common chest instance offset is derived from both mesh variants' rear bounds,
leaving 0.0125 cells beyond the furthest open-lid point. Current source geometry
produces a 0.2476785-cell wallward shift. The closed base consequently retains the
clearance needed for the opened lid; state changes never move the base. Source
metric UVs, half-size scaling and painted shading remain intact.

Release application/headless and the user's Debug application build with at most
two low-priority workers. The affected integration selection passed **19/19 in
7.64 s**, including full campaign persistence at 5.77 s:

```sh
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-release \
  -R '^(dressing|dungeon_refresh|underkeep_render|world_visibility|world_cpu|world_environment|generation|population|fixtures|actors|ai|hybrid|combat|recovery|automation_allocations|generation_automation|campaign_save|gameplay_startup|world_visual_cli)$' \
  -j 1 --output-on-failure
```

The owned visibility fixture covers four headings, four subslots, three swing
states and five walk samples (240 changed views), checking silhouette/box separation,
within-subslot anchors and zero engine C++ allocations. The final viewport-edge
regression adds five onscreen walk samples with actual pixel/alpha target gating.
After the edge guard, only affected `world_visibility` and `world_visual_cli`
checks were repeated; final visibility checks passed in 0.24 s. New fixture setup
errors were corrected without relaxing admission: width belongs on clip overrides
(base appearances remain <=0.5), setup moves must be adjacent, and edge samples
must start with their original axis onscreen. Initial compilation also caught an
inconsistent auto declaration and a wrong renderer asset-owner reference.

Chest fixtures verify player stepping, same-cell F/access while turning, NPC
exclusion, empty/partial/sealed states and overlap rejection. Full campaign save/
Continue and inactive-floor revisit retain a party standing on the chest cell.
Two existing diagnostic generation identities remain exact: seed 0/depth 1
`15711381999283466297`, depth 3 `13984553738469637557`. Generator/catalog remain
9/7, campaign schema/simulation 7/11, visibility is now `world-visibility-v10`.

The final hidden review passed:

```sh
nice -n 15 ionice -c 3 cmake-build-release/blobber_world_integration_gpu_tests \
  cmake-build-release/assets docs/screenshots/door-chest --door-chest-review \
  /home/bison/.cache/blobber-door-chest-review/campaign.json
```

[Thirteen inspected native captures and five-sample animation](screenshots/door-chest/README.md)
cover the actual saved doorway from both sides, the wall-side chest before/after
stepping and opening, and production giant rats traversing an open leaf. After the
initial saved view, NPCs are omitted in the copied scene to isolate door/chest
review; rat walking is a separate controlled fixture. GL errors, CPU/GPU interior
ID agreement (<100 nonedge differences per 500x282 frame) and resource release
passed on NVIDIA RTX 3060/OpenGL 3.3. No exact cross-device raster claim or full
combat/balance route. The final guard left these inspected views pixel-identical;
their manifest records the final runner identity.

The user's original save and diagnostic copy remain byte-identical, SHA256
`3d1d6b49eec2a7bfb69376b54e6824f05f1deca94042cc546bdbee49271b0357`.
No save migration/rewrite, desktop input/focus automation, benchmark or unrelated
configuration matrix. Logs are `~/.cache/door-chest-{integration-build,tests,
edge-build,edge-test-build,gpu-final,debug-build,debug-final-build}.log`.
Earlier owned captures are archived outside the repository under
`~/.cache/blobber-door-chest-review/before-edge-guard/`. Visual acceptance and
interactive gameplay remain user review.

# Sanctuary smithy

Status: functional interior and equipment-scale correction accepted by the user (2026-11-07).
The [approved mockup](mockups/smithy-v1/README.md) is the art reference.
[Duncan Ironballs](blacksmith_design.md) now occupies the reserved counter position;
his production appearance and dialogue/trading await separate runtime review.

## Content and layout

One authored campaign location, separate from dungeon depth. City entry is the
existing smithy approach (15,12), east-facing; return faces west. Interior
entry/exit share (5,9), facing north/south. The map is 12×12 with solid padding,
modeled enclosure and an 8×8 floor (2,2) through (9,9). Ceiling 1.5 cells,
unchanged .5-cell eye and 1920×1080 presentation.

| Feature | Cells |
| --- | --- |
| East-facing forge | (2,2)–(2,3) |
| Shelves/tool display | (5,2)–(7,2) |
| Counter | (5,4)–(7,4) |
| Duncan Ironballs | (6,3), subcell (1,1), south-facing |
| Customer approach | (6,5), north-facing |
| Anvil / quench tub | (3,5) / (2,5) |
| Charcoal / iron stock | (2,7) / (2,8) |
| Weapon/shield display | (9,4)–(9,6) |

Columns 5–6 remain clear from the entrance to the counter. Column 8 connects to
the rear staff aisle; one-cell routes reach the forge and all usable cells.
Scenery reserves whole cells without becoming full-height sight walls. Mesh bounds
fit these footprints; no diagonal movement or squeezing through decorative gaps.
The smith position is occupied by Duncan. Focused owned fixtures verify existing
dialogue range/facing and routes with that cell occupied. His content, canonical
placement validation and resource results are documented in the linked design.
Workshop equipment is decorative. Ordinary rest rules apply; no free-rest policy.

## Assets and ownership

A dedicated sanctuary-smithy pack uses the inn's offline painted-mesh pipeline:
retained Blender/GLB, source textures and runtime meshes generated together.
Reuse city material masters and export helpers, metric 256 texels/cell and 16-pixel
atlas gutters. Counter height .30, anvil .27 cells; inspect adjacent legal cells.
Forge warmth is painted; the existing immutable AmbientFire anchor drives one
bounded 24-particle emitter, with pause/depth behavior and no gameplay/picking IDs.
No new lighting or frame-time resource allocation. Asset leases stage outside
frame work and must admit city/smithy overlap within existing budgets.

## Campaign and interfaces

Append Smithy=5, six fixed campaign locations. Each CampaignLocation owns up to
two canonical immutable DoorwayLinks plus count; overflow rejects in preparation.
City slot 0 stays inn, slot 1 smithy; interior slot 0 returns to city. Links have
static action labels. World/slot lookup plus campaign revision, exact pose/facing
and safety checks protect requests; slots never recycle, so generation is unneeded.
Links reconstruct from authored content, never disk data. Shared checked lookup
serves previews, travel and prompts. Destination map/session/view must prepare
before commit; cancellation/failure preserves the source, and the old view dies
before its session. Existing floor snapshots, effects and party transfer apply.

Authored smithy constructor/validator own geometry, fixtures and canonical restore
checks. City excludes both doorway arrivals from NPC placement. HUD/map/automation
identify Sanctuary smithy at depth zero. Inn and smithy require door exit for
recall/dungeon travel; no new travel kind. Schema/simulation advance 14/17 → 15/18.
The user chose a new campaign: old saves reject unchanged, no migration. Generator,
templates and NPC content are unchanged. Six-location storage adds one bounded
FloorState snapshot; it does not enlarge renderer admission budgets.

## Verification strategy and handoff

Use small owned CPU fixtures: real cardinal routes, blocked furniture, future
resident interaction, two city links, labels/facing, invalid/stale/foreign handles,
travel safety/cancellation/preparation failure and warmed allocation guards.
Check smithy save/Continue and inactive snapshots, discovery/drop retention,
canonical rejection, old schema rejection and inn regressions. Run focused smithy,
inn, campaign and save smoke in one configuration; extended persistence once.
Asset bounds/UV/publication checks and hidden production captures cover entry,
counter, forge, staff passage, equipment and exit/Continue; inspect fire pause/depth,
staged residency and lease release. No desktop input automation or benchmarks.

Use one intensive job, nice/ionice, at most two build workers and one test process.
Owned disposable output goes in an agent run. Retain at most six selected runtime
review images, record outcomes/limitations here and clean the run after preview.
Visual concept approval and runtime acceptance are separate.

## Verification results — 2026-11-07

The approved mockup and exact request are retained in `docs/mockups/smithy-v1/`.
The implemented exporter delivers 11 models / 2,996 triangles and a 4096² atlas.
All generated prop bounds fit their authored footprints; outward triangle winding,
texture references and UV edge lengths at 256 texels/cell passed static inspection
(rounding tolerance .015 texels). Counter slab is .30 cells, the small side chest
reaches .40, anvil .27; the customer/NPC centre distance is about 1.77 cells.

Focused checks in headless Release passed:

```sh
nice -n 15 ionice -c 3 cmake --build --preset headless-release --target \
  blobber_smithy_tests blobber_inn_tests blobber_campaign_tests blobber_campaign_save_tests -j 2
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-headless-release \
  -R '^(smithy|inn|campaign|campaign_save_smoke)$' -j 1 --output-on-failure
# After adding warmed-frame and shared-flame residency regressions:
nice -n 15 ionice -c 3 cmake --build --preset headless-release --target \
  blobber_smithy_tests blobber_world_residency_tests -j 2
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-headless-release \
  -R '^(smithy|world_residency|campaign_save)$' -j 1 --output-on-failure
```

The first four-suite run took 3.31 s; the final three-suite run took 6.96 s,
including the single extended persistence run (6.67 s). Six distinct suites passed;
only smithy was repeated, following the added frame-allocation assertion. Tests
use owned rules/resident data, not production NPC artwork. The future resident
fixture preserves every unoccupied walkable route and admits conversation from
(6,5) while rejecting the wrong facing. Warmed C++ simulation/command frames and
both-city-doorway queries allocate no C++ storage; this does not assert anything
about driver or third-party internal allocation. Full persistence and legacy schema
rejection pass with the six-location format.

The final Debug game and review-tool builds passed. The hidden production review
tool passed on NVIDIA RTX 3060 OpenGL, using the
Debug publication and standard fixed-viewport Presenter. The initial sandboxed
attempt could not open display :0; the retry with display access succeeded, with
FLAG_WINDOW_HIDDEN checked. No OS input or focus control was used.

```sh
nice -n 15 ionice -c 3 cmake --build --preset debug --target blobber blobber_review_smithy -j 2
# A new subdirectory inside an owned agent run is required:
OMP_NUM_THREADS=2 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 \
  cmake-build-debug/blobber_review_smithy cmake-build-debug/assets RUN/review
```

Inspection covered entrance composition, the low customer counter and empty NPC
silhouette space, forge containment/occlusion, equipment, staff aisle and the exit
following disk Continue. The two fire captures differ after 200 ms. Animated fire leaves picking revision
unchanged; pause
preserves the clock and rendered-image revision. GPU allocation failure and canceled
staging preserve the source view and release destination resources; successful
travel/Continue returns to the correct exterior pose. Resource measurements:

| Measurement | Result |
| --- | --- |
| City + staged smithy world GPU | 454.96 MiB (existing 1 GiB ceiling) |
| Staged world CPU | 43.16 MiB (existing 352 MiB ceiling) |
| Peak application GPU during review | 577.18 MiB |
| Smithy-only texture mip residency | 159.44 MiB |
| Final texture / alpha residency after view release | 0 / 0 bytes |
| Six FloorState snapshot size | 1,555,200 bytes |
| Active production smithy save | 2,252,296 bytes (existing 16 MiB limit) |

[Six selected captures](screenshots/smithy/README.md) retain visual evidence.
Interactive user review was pending at initial delivery; the functional interior
is now accepted, with the equipment correction below. The actual blacksmith, dialogue camera
appearance and commerce are intentionally absent; the synthetic resident establishes
movement/range feasibility, not future artwork acceptance. No benchmarks, stress
matrix or desktop automation ran. Prepared-image rendering, camera, dungeon
progression and resource ceilings remain unchanged.

Blender finished saving both source formats, meshes and the checked bounds report,
but stalled during audio shutdown and hit the 240-second timeout. The exported
files passed independent mesh/UV checks and full publication/rendering; no partial
asset was promoted. The pack README's reproduction command disables Blender audio.
Disposable exports, raw captures and logs belong to this task's owned run; only the
approved reference, required source assets and six selected images are durable.

Owned-run cleanup was previewed and applied after all runners closed. No disposable
diagnostic exception is retained. Required Blender/GLB/texture inputs, the approved
mockup and six selected review images remain. The functional interior is now
accepted; the equipment-scale correction below was subsequently accepted too.

## Equipment-scale review correction — 2026-11-07

Status: accepted by the user before authorizing Duncan's implementation.

The user accepts the functional interior and defers additional decoration, but
rejects the oversized rack weapons. Keep rack frames, cell reservations, camera,
map fingerprint and save compatibility unchanged. Scale each sword uniformly to
65% around its own grip axis, placing its guard just above the .4475-cell upper
rail. Target guard centre .456 cells: final tip/pommel heights .131/.54375 cells,
roughly 1.32 m total length at the existing .5-cell eye = 1.6 m reference. Reduce
the shield uniformly to 75% around its existing centre (.38-cell elevation).
Apply these transforms offline before metric UV packing; regenerate Blender, GLB,
runtime meshes and atlas together. No engine or gameplay change is needed.

Verify the exported equipment dimensions, unchanged non-equipment geometry and
normal/UV invariants; rebuild the Debug prepared publication and inspect hidden
equipment/entry captures. Reuse prior gameplay verification for this asset-only
correction. Replace affected review evidence, preserve the user's acceptance and
clean the task-owned disposable output.


The equipment correction is implemented and mechanically verified. Swords are
.41275 cells long (65%); the shield is .2625 cells tall (75%). A reconstructed
pre-correction geometry comparison confirms all six sword transforms, the shield
transform and unchanged rack/non-equipment positions. Recomputed normal components
vary by at most 0.000017 after float rounding under uniform scaling (checked within
0.0001); winding and metric UV edge lengths pass the existing .015-texel tolerance.
Footprint assertions, all 11 model bindings and 2,996 triangles are preserved.

Regenerated source formats/runtime meshes/atlas, then rebuilt the Debug game and
publication with `nice -n 15 ionice -c 3 cmake --build --preset debug --target
blobber blobber_review_smithy -j 1`. The focused hidden review mode passed on the
same OpenGL device, exporting entry, equipment and an adjacent-cell equipment
view with no desktop input. Inspected smaller weapons, guard/rail alignment,
shield and approach composition. It releases texture/alpha leases at completion.
Reproduce with `blobber_review_smithy ASSETS NEW_OUTPUT equipment` inside an owned
agent run, with the usual low-priority/thread bounds and display access.

Only entry/equipment evidence was replaced; the gallery remains at six images.
Existing CPU travel/save results are reused: this correction changes no gameplay,
map definition, cell reservations or save schema. No additional gameplay tests or
benchmarks ran. Decoration remains deferred; the functional interior acceptance
is preserved, and the corrected equipment was subsequently accepted by the user.

The exporter again finished writing all assets but stalled in the host audio
teardown, even with `-noaudio`, and reached the 180-second bound. Complete outputs
were independently verified and rendered. Temporary comparison output, exports
and extra captures were cleaned after all runners closed; no diagnostic exception
is retained.

# BR02: finished room kit and assembled review

Status: **BR02 accepted and closed, 2026-11-09. All four rooms approved by
the user; the final requested bed collision correction is implemented and verified.**
BR03 campaign generation remains separate.

## Bed collision and BR02 acceptance

The user accepts barracks and armory, requesting collision beneath the six beds.
Reuse the existing whole-cell scenery contract: set both sleeping-bay kits to
`blocking: true`, reserving only their existing 1×1 cells for party and NPCs.
Keep all other quarters props nonblocking and all accepted artwork unchanged.
Shorten the barracks row-3 lane to the three open interior cells (columns 1–3);
its former wall endpoints now contain the middle beds. The center aisle, doors,
encounter and reward space are unchanged. Update the review movement checks to
reject entry into beds while allowing decorative wall props, and walk the actual
interior cross aisle. This is catalog/review data maintenance, not a new runtime
collision system. Validate all variants, run the focused blueprint suite and one
hidden barracks movement check; existing accepted screenshots remain valid.
BR02 is accepted and closed; no further BR02 work remains.
BR03 is still proposed and is not started by this correction.

### Final collision verification

Configured Debug build passes. The focused owned-fixture suite
`ctest --test-dir cmake-build-debug -R '^blueprints$' -j1 --output-on-failure`
passes (0.08 seconds), covering blocking/decorative footprints and transforms.
`./review-rooms --room barracks --check` passes after production preparation:
144 room variants and four pair orientations validate, with finished bindings intact.
The hidden `blobber_review_blueprints capture --art finished --room barracks
--sockets north,south` check confirms six blocking beds, four nonblocking decorative
placements, and normal center/cross-aisle, doorway and reward access. All thirteen
views have zero interior CPU/GPU picking differences; preparation rollback and GPU
cleanup pass. No armory artwork or catalog placements changed, so its accepted
review was not repeated. No OS input/focus automation was used.

Retained the existing six accepted quarters images: this correction has no visual
change. Owned temporary capture/build logs were cleaned; normal build caches remain.
The sections below are historical iterations, superseded by this accepted closure.

## Six-bed layout and wall furniture tuning

User-directed follow-up: all furnishings in barracks/armory are decorative and
must not reserve party or NPC cells. Set their existing `blocking` flags false;
visual envelopes still protect content from mesh overlap. Increase sleeping bays,
lockers, rack and small wall items 20% from the last review (0.6 original native
scale). Increase the workbench itself 60% (0.8 original scale), with its helmet
shelf/contents at 0.6 scale. Lower the helmet and bedding shelf mounts to 0.65,
coat peg mount to 0.68, and both sconce flames to 0.58 cells. Use a named sconce
variant shifted 0.35 cells along its wall toward the armory table; its local fire
anchor follows the mesh. Shared accepted kitchen/hall models stay unchanged.

Place three inward-facing sleeping bays on each side at rows 1,3,5. Larger beds
use a 1×1 visual envelope, anchored at columns 0/4 and backed against the side
walls; their full-size lockers rotate beside the foot of the bed. This preserves
the 20% enlargement without reserving another visual cell; no collision footprint. Move barracks sconces to row 2.
Keep the north/south doors; the furnished middle side walls replace the unused
optional east/west doorway sockets. Keep the existing 3×2 encounter space at (1,2) in the open center, so decorative
beds do not overlap encounter visuals.
This changes the shipped socket/rotation review set from 192 to 144 variants.

Reuse existing authoring, immutable fire scale, nonblocking scenery and capacities
(21 kits of 24, ten barracks placements of 16). No renderer, save or frame-path
change. Update blockout representations and use focused blueprint/render checks,
production validation and game captures. Walk into every decorative placement to
verify party and NPC noncollision; inspect both new middle beds and the armory
wall as a group. Replace superseded evidence, retaining six images, then clean the
owned run. Revised appearance still needs user review.

### Six-bed tuning verification — 2026-11-09

Implemented and mechanically verified; visual re-review pending. Configured Debug
build passes. Focused command `ctest --test-dir cmake-build-debug -R
'^(blueprints|blueprint_render)$' -j1 --output-on-failure` passes both suites in
0.83 seconds. These cover the changed catalog/blockout behavior; no unrelated
matrix, stress run or benchmark was run.

`./review-rooms --room barracks --check` and `./review-rooms --room armory --check`
pass after production preparation, validating all 144 variants and four pair
orientations. The smaller variant count follows the removed barracks side sockets.
Native exports pass bounds, finite data, metric 256 texels/cell and triangle limits;
the 2048² atlas uses 1,218 rows. Accepted kitchen/hall mesh positions/normals and
sampled paint at every triangle centroid match the pre-task assets.

Hidden finished-art captures with north/south barracks sockets and south/west
armory sockets pass: thirteen barracks views and ten armory views, all with zero
interior CPU/GPU picking differences, successful preparation rollback and GPU
cleanup. Normal movement into every furnishing anchor passes (ten barracks, six
armory), and each is also clear in NPC collision data. Center/cross aisles, doors
and reward access pass. Inspected room compositions, both middle bed positions,
the maintenance wall and oblique shields; six selected images replace the prior
quarters gallery. Interactive desktop review remains unverified; no OS input or
focus automation was used.

Blender completed and exported valid artifacts, then hit the known PulseAudio
teardown hang and was terminated by its bounded timeout (exit 124). Production
preparation and game captures independently consumed the exported assets. Owned
scratch output was cleaned after evidence promotion; durable sources and normal
build caches remain. The following sections record superseded historical iterations.

## Quarters scale and joinery correction

Implemented and mechanically verified, 2026-11-09; revised scale awaits user review.

User review found the new quarters furniture oversized, with a shield intersecting
its rack and rear workbench legs outside the top. Reduce all eight new kit models
to 50% native dimensions with uniform scaling; keep accepted shared armor and
kitchen/hall models unchanged. Floor furniture stays on the floor and backs toward
the wall. Preserve wall mounting heights while shrinking sconces, coat pegs,
bedding/helmet shelves and their contents. Sleeping bays now fit one cell, moved
back to side-wall anchors (0,1)/(4,1)/(0,5)/(4,5); weapon storage fits two cells.
Update contracts and blockouts so former bed cells become walkable. Move shields
forward with a real gap from the rail; derive bench legs/braces from matching
front/rear support positions under the top. Correct exposed mattress intersections
at the side folds as part of the same bedding geometry repair.

Sconce flames must shrink too. Add immutable `AmbientFire::scale` (default 1,
finite and in (0,1]) and an optional fourth number in a kit fire array. Carry it
through assembly, map validation and the existing delta persistence visitor;
missing saved scale stays at baseline/default 1, preserving existing saves.
Only nondefault scales extend world fingerprints, leaving existing fire/map
identities unchanged.
Scale particle offsets and quad sizes together; leave animation timing, fixed
particle capacity and conservative visibility margin unchanged. Existing hearths
and braziers retain scale 1. This introduces no frame allocation or new render path.

Verify bounds/metric UV export, shield separation and top/support containment;
exercise reduced-fire containment at multiple angles, default-size compatibility,
invalid scale rejection, blueprint transforms and save roundtrip. Use focused
blueprint/render/world-integration/save checks, production bake and hidden game
side captures matching the user's views. Replace superseded quarters evidence
and clean the owned run. Visual approval remains pending.

### Scale correction verification

Configured Debug build and the focused `blueprints|blueprint_render|world_integration|
campaign_save_smoke` selection pass (four suites, 3.29 seconds). The fire regression
checks full-size and half-size bowls through eight viewing angles and sixty ticks,
compares every scaled quad vertex, rejects invalid scales, and retains allocation-
free bounded frame work. Blueprint tests cover optional/default scale and rotation;
save smoke covers scaled/default fire roundtrip and omitted-field compatibility.
No broad generation run or benchmark was needed for this localized correction.

Native exports pass bounds, finite data, metric 256 texels/cell and triangle limits.
Beds are 0.4025 × 0.275 × 0.95575 cells, including their locker, inside one cell;
weapon storage is 1.265 cells wide and 0.61 high. Sconce bowl diameter is 0.15 cells
and flame scale 0.5. Bench legs use symmetric front/rear support positions; its top
extends 0.02175 cells beyond the front/back leg edges. Round/kite shield backs
have 0.020/0.0175-cell clearance ahead of the rail face. Bed undersides now follow
the folded blanket surface instead of a detached flat panel; side folds pass
outside the mattress. New beds have 1,144 triangles; other counts are unchanged.
The existing 2048² atlas uses 1,099 rows. All twelve accepted shared models retain
identical positions/normals and sampled per-triangle paint after atlas repacking.

Production baking, both launcher `--check` commands and all 192 room variants/four
pair orientations pass. Hidden raylib/OpenGL review produced eleven barracks and
ten armory views, including both reported side angles, close sconces and oblique
shields. All 21 captures have zero interior CPU/GPU picking differences, no GL
errors, successful staged-failure rollback and GPU cleanup. Center/cross aisles,
doors and reward access pass normal movement; the new close bed poses occupy the
released former footprint cells. Six superseding captures remain in the quarters
gallery; accepted kitchen/hall evidence is intact. Interactive re-review is pending.

The background Blender exporter completed its outputs/checks and was then stopped
by the bounded timeout during the known PulseAudio teardown hang (exit 124).
Production bake and in-game capture independently consumed those exports. No OS
input or window focus automation was used. Owned temporary output was cleaned;
source artwork, models and normal build caches remain. Earlier verification below
records the initial larger version and is superseded by this correction.

## Barracks and armory implementation

Use the approved v1 mockups as the visual reference. Preserve the 5×7 barracks and
5×5 armory, existing ceilings, doors and reward/encounter spaces. Adapt furniture
orientation to match beds facing inward from the two side walls. Four sleeping-bay
instances occupy two cells each, at (1,1)/(1,5) east-facing and (3,1)/(3,5)
west-facing. Each grouped bay includes its own footlocker at the foot, within the
same visual/collision envelope, leaving the center column and cross-aisle clear.
Alternate burgundy/green blankets. Add a wall peg/coat kit, spare bedding shelf,
and two nonblocking conditional wall sconces; local fire anchors reuse the bounded
two-fire system. The default barracks review opens north/south sockets so a real
far doorway matches the reference.

Armory: a 3×1 rack centered on the back wall holds spears, swords, axes and shields;
a separate right-wall maintenance bench carries a sword, whetstone, oil jar and
cloth, plus a shelf of helmets. Reuse the approved nonblocking armor at the left
wall, add a closed footlocker by the bench and two wall sconces. The rack takes the
north end of the center lane; the remaining lane starts at row 1. Side entry,
encounter and reward access remain clear. The bench is at (4,1) west-facing,
approachable from (3,1); its chest sits in the far corner at (4,0). This avoids
trapping the bench between the rack and chest. Keep accepted kitchen/hall placements,
geometry and material appearance unchanged.

Add cohesive native geometry authoring in `tools/blueprint_quarters_models.py`,
called by the existing Blender exporter. Use retained timber/cloth/metal masters;
soft bedding is closed modeled geometry with folds, not flat slabs. Continuous
cloth/metal shading avoids polygon grids. Each kit remains at most 2,000 triangles,
native scale and 256 texels/cell in the existing 2048² atlas. Twenty concrete kits
require raising catalog capacity from 16 to 24; fixed storage remains preparation-
owned, with the existing 16 placements/room and 32 runtime scenery limit unchanged.
No new render path, runtime entities, frame allocation or save fields. Add blockout
forms for the new kit families. New source kits remain separately reusable.

Extend `review-rooms --room barracks|armory` (default remains kitchen-hall) so the
finished rooms are directly runnable. Extend deterministic captures with bed,
locker, rack, bench and wall-dressing views and normal movement around furniture.
Verify focused blueprint/render/scenery/fixture tests, all shipped rotation/socket
variants, production baking, hidden raylib/OpenGL captures and launcher checks.
Inspect models and game views against the approved concepts. Preserve accepted pair
evidence and retain at most six new room review captures in a separate feature
gallery; clean temporary output. In-game acceptance remains pending user review.

## Barracks and armory verification

Implemented 2026-11-09. Eight additional source meshes complete all twenty catalog
kits. The new meshes contain 178–1,168 triangles each: sleeping bays 1,060 each,
footlocker 204, weapon rack 1,100, maintenance bench 1,168, bedding shelf 382, coat
pegs 232 and sconce 178. All exports pass native contract bounds, finiteness and
per-triangle metric UV singular-value checks at 256 texels/cell. The 2048² atlas
uses 1,687 rows with four-pixel gutters. Packed Blender/GLB sources are retained.
All twelve accepted kitchen/hall models retain identical positions/normals and
identical sampled paint at every triangle centroid after atlas repacking.

Inspected source front/rear/underside views and actual game captures. Corrected
blanket/mattress intersection, improved the sword and axe silhouettes, and moved
the armory bench to (4,1) with its locker in the far corner at (4,0): the initial
capture exposed that the rack/locker left no valid front approach to the bench.
The final capture approaches the work surface from (3,1) and passes. No unresolved
capture failures remain. Source Blender export/render completes its files and
checks, then this installation hangs in PulseAudio teardown; bounded 10–45 second
timeouts end that process (exit 124). This is not reported as a clean Blender exit.
The resulting files are independently consumed by the production baker and renderer.

Configured CLion CMake/Ninja Debug build and production asset publication pass,
using at most two build workers and one heavy process at a time, at low priority.
Focused checks:

```sh
/home/bison/clion/bin/cmake/linux/x64/bin/ctest --test-dir cmake-build-debug -R '^(blueprints|blueprint_render|scenery|fixtures)$' -j1 --output-on-failure
/home/bison/clion/bin/cmake/linux/x64/bin/ctest --test-dir cmake-build-debug -R '^generation$' -j1 --output-on-failure
./review-rooms --room barracks --check
./review-rooms --room armory --check
```

The four affected suites pass in 1.12 seconds; generation regression passes once
in 1.45 seconds at this finished-kit checkpoint. Owned render fixtures cover all
new blockout families, contract bounds, finished publication and resource release.
Both launchers validate all 192 shipped room rotation/socket variants and four pair
orientations, then their requested finished bindings. The default kitchen/hall
selection remains unchanged. Campaign generation code, saves and frame processing
are unchanged; BR03 remains separate.

Hidden application-owned captures pass: barracks at 0° and 90° (ten views each),
and final armory at 0° (nine views). All 29 successful views report zero interior
CPU/GPU picking differences, no OpenGL errors, successful injected GPU preparation
rollback and GPU view cleanup. Final capture code walks the center and cross aisles
through ordinary movement, checks every declared lane, exercises doorway traversal
and opens the reward chest. Peak accounted application GPU bytes were 293,989,608 (barracks) and
293,845,464 (armory), below the existing budget; prepared scene CPU bytes were
959,192 and 815,032 respectively. Driver/process-wide memory is not included.
Selected final-publication views are retained in the
[six-image quarters gallery](screenshots/blueprint-rooms-br02-quarters/README.md).
Accepted pair evidence remains intact. No desktop input/focus automation was used;
interactive walkthrough and aesthetic acceptance remain with the user. The source
mockups are references, not a promise of identical illustrative lighting.

Run either room directly:

```sh
./review-rooms --room barracks
./review-rooms --room armory
```

The owned run is cleaned after selecting evidence; source artwork, prompts,
reusable authoring scripts, mesh/Blender/GLB exports and normal build caches remain.
Earlier pair-stage notes below are historical and do not supersede this status.

## Barracks and armory concept checkpoint

Two built-in imagegen mockups were produced and visually inspected, using the
approved interiors' stone, timber, muted metal and pixel-textured style:

- [Barracks v1](../assets/packs/underkeep/concepts/rooms/barracks-mockup-v1.png):
  four beds and footlockers, wool blankets, wall pegs and spare bedding storage,
  with an open central aisle.
- [Armory v1](../assets/packs/underkeep/concepts/rooms/armory-mockup-v1.png):
  weapon rack, shields, one armor stand, maintenance bench and equipment storage,
  with open center circulation.
- [Exact prompts](../assets/packs/underkeep/concepts/rooms/barracks-armory-mockup-prompts-v1.json).

These are approved visual references, not in-engine captures or implemented content.
They establish a direction to review, not a claim of exact blueprint scale, bed
orientation, runtime lighting or final geometry. Reconcile the approved composition
with actual room bounds and sockets in the later modeling plan. No barracks/armory
models, layouts, runtime code or asset bindings were changed at this checkpoint.
No build or gameplay tests were needed for these concept images. Source concepts
and prompts are retained; owned temporary staging files were cleaned. The user subsequently approved both mockups and explicitly authorized modeling.

## Design and ownership

### Armor finish, table-centered chandelier and kitchen shelves

Implemented and mechanically verified, 2026-11-09; visual review pending. Preserve
the accepted armor shape; remove
per-face dark edge painting from its steel/iron, retaining restrained metal grain
and broad directional shading. Other furniture materials keep their existing finish.
Move the chandelier over the actual table center (3,1.5): placement (2,1), 3×2
visual envelope, mesh-local center (1,.5). No runtime offset/scale or collision change.

Add one reusable nonblocking `pottery-shelves` kit, three wooden shelves with varied
open/lidded clay jars, at kitchen (0,1)/(0,2) east-facing and (2,0) south-facing.
The shallow rack stays against the wall, with no movement/NPC footprint. These
positions include optional west/north socket entries; the pair leaves both walls
solid. Add optional default-false Kit `closedWall`, restricted to nonblocking 1×1
kits: preparation requires wall backing and omits the placement when its backing
socket is selected. Socket validation permits only that matching conditional kit;
other furnishings still cannot obstruct entries. Assemble filters its owned room
copy before generating scenery and plans. This metadata is preparation-only: no
WorldDefinition, persistence, rendering or frame-work changes. Keep bounded catalog
and placement storage, and reuse the existing atlas, timber/clay masters and bake.

Verify conditional shelf presence/absence and movement in all four rotations using
owned fixtures; extend blockout coverage without increasing kit capacity. Validate
shipped variants, asset bounds/UVs and the actual launcher. Inspect hidden in-game
views of both marked kitchen walls, close armor, and chandelier/table alignment.
Retain six current images and clean this run; prior accepted fireplace remains intact.

### Shelving and finish verification

Configured Debug build passes. The focused selection `blueprints|blueprint_render|
scenery|fixtures` passes all four suites in 0.99 seconds. These cover the changed
catalog/assembly/blockout path and its collision/door callers; persistence and
runtime effects did not change in this revision. Owned fixtures exercise conditional
shelf inclusion/omission for five socket selections in every rotation, actual
solid-wall backing, nonblocking movement/NPC reservations, room metadata consistency,
and malformed conditions. Shipped validation passes 192 variants/four pair rotations.

The final exporter passes bounds and metric-UV assertions. Twelve models fit the
existing atlas/mesh budgets: shelves 1,748 triangles, armor 1,548, chandelier 1,924;
atlas 2048², 774 rows used, 256 texels/cell. Inspected front/back model views of the
shelves and the revised armor finish. Armor positions/normals are exactly unchanged;
accepted hearth positions/normals and 5,382 atlas sample colors also match the prior
revision. Table and chandelier exported horizontal centers both equal (3,1.5).
Background Blender writes complete checked output but stalls in PulseAudio shutdown;
bounded timers ended those processes with 124, not a normal Blender exit.

Production bake and `./review-rooms --check` pass. Publication fingerprint
`9352054891326440930`; unchanged 556 textures / 883,633,616 mip bytes /
64,967,422 alpha bytes. Hidden captures use `blobber_review_blueprints capture
--art finished --room kitchen-hall --assets assets
--prepared cmake-build-debug/assets/prepared --ui-prepared cmake-build-debug/assets/prepared
--output RUN/pair`, plus `--rotation 90 --output RUN/pair90` for the second run.
All **56 views** have zero nonedge CPU/GPU picking disagreements and no GL errors.
In-process movement traverses every shelf cell, both armor cells and all four sides
of the center table in both orientations. Door/reward interaction, GPU preparation
rollback and resource cleanup pass. No OS input or focus control was used.

Inspected both marked kitchen walls, the near armor pose matching the user’s
screenshot, chandelier/table alignment, and rotated wall/hall views. Shelves sit
against the intended walls; no rack occupies an open doorway in alternate fixtures.
Six native images replace the [current gallery](screenshots/blueprint-rooms-br02/README.md).
The fireplace and armor geometry acceptance is preserved; latest visual/interactive
acceptance remains pending. Chandelier candles remain static and unlit.

Pair residency: 51 textures, 114,800,228 mip bytes, 12,613,249 alpha bytes,
1,298,304 source mesh bytes; scene CPU/upload 3,677,624 / 3,676,848 bytes;
renderer CPU 26,043,137; world GPU 143,310,524; peak application GPU 296,707,848.
No capacity was increased. These are engine accounting, not process/driver totals.
No unrelated configurations, campaign sweep, benchmark or interactive desktop
play was run. All owned runners are closed and temporary authoring outputs,
model renders, extra captures and logs cleaned; durable sources and normal caches
remain, with no retained diagnostic exception.

### Central hall furnishing revision

Historical preceding revision; chandelier placement/finish are superseded above.

Implemented and mechanically verified (2026-11-09), awaiting user review. Add the third identical 1×2 table at
(3,1), equidistant from tables at x=0/6, with clear cells on both sides and ends.
Reserve side lanes x=2/4 and cross lanes y=0/3/4. Put matching nonblocking armor
at (2,0)/(4,0), facing into the hall, and a large iron candle chandelier centered
at (3,3), suspended from the two-cell ceiling. Both armor instances and the
chandelier have no movement or NPC footprint. Keep the accepted fireplace.
Reduce barrel height from .4104 to .34 cells with its diameter unchanged; tighten
additional brazier flame centers to 40% of their previous horizontal spread.
Legacy hearth flames retain their established spread.

Kit and SceneryPlacement gain default-zero `clearance`: minimum native mesh Y.
A positive clearance must be at least one cell and requires nonblocking scenery.
The chandelier uses a 3×3 visual envelope and clearance 1.25, height 2.0. Blueprint
validation checks horizontal bounds and vertical intervals; overhead furniture
may overlap floor furniture only with disjoint height intervals. Overhead objects
do not reserve floor lanes, encounters or reward space. Lanes use movement
occupancy, allowing walk-through wall displays. Map preparation tracks separate
floor/overhead visual layers, rejects same-layer overlaps and validates clearance
against local ceilings. No runtime allocation or new render path: the same static
meshes remain owned by the existing prepared publication. Save fields and tagged
hash extensions preserve existing identities when clearance is zero. Finished-art
validation enforces the lower mesh bound as well as the existing upper bound.

Author armor with shaped plate, articulated limbs, visor and boots; use retained
metal textures. Author a closed iron hoop, candle cups, wax candles and suspension
chains for the chandelier, within existing mesh/atlas limits. Candles are static;
braziers remain the animated fire sources. Geometry and UVs are final native size.
The armor kit is finished for these hall displays; the armory room remains deferred.

Verify owned-fixture overhead/floor overlap and rejection, no collision in four
rotations, minimum mesh height, save/hash round trips and brazier containment.
Then validate shipped layouts, explicitly walk around the central table and through
armor/under chandelier, inspect hidden GL overview/detail/rotated captures, update
six evidence images and clean the owned run. Interactive desktop review remains
with the user; visual acceptance remains pending.

### Central hall verification

Preceding revision: the configured Debug build and seven focused suites pass:
`blueprints`, `blueprint_render`, `scenery`, `fixtures`, `world_integration`,
`world_residency`, `campaign_save_smoke`. Run with the configured CLion CMake/CTest,
two build workers and `ctest --test-dir cmake-build-debug -R
'^(blueprints|blueprint_render|scenery|fixtures|world_integration|world_residency|campaign_save_smoke)$'
-j 1 --output-on-failure`. Six suites passed initially; the new flame containment
test initially assumed packet submission order, overlooking transparent depth
sorting. Corrected the test to match particles spatially; rebuilt/reran only
`world_integration`, which passes. The test covers 480 time/angle combinations.
Owned fixtures cover floor/overhead overlap, height intrusion, invalid clearances,
movement/NPC reservations in all rotations, lower mesh bounds and save/hash round
trips. Shipped catalog validation passes 192 variants and four pair orientations.

Source authoring produces eleven meshes; armor is 1,548 triangles and chandelier
1,924, under the unchanged 2,000-triangle limits. Shared atlas remains 2048² with
256 texels/cell and 684 used rows. Barrel height .34, diameter .2736. Armor height
.737; chandelier spans 1.23 horizontally, native Y 1.27–2.0. Front/rear/underside
emission views of both new models were inspected. The first chandelier review
framing cropped the hoop; the review tool now frames it at scale 2 and all three
views were regenerated. Background Blender wrote complete outputs, then stalled
in this environment's PulseAudio shutdown; bounded timeouts terminated it (124).
This is not reported as a clean Blender process exit. Geometry/UV assertions and
mesh exports completed before shutdown. Fireplace positions/normals and 5,382
interior atlas samples exactly match the accepted version despite atlas repacking.

Production preparation and `./review-rooms --check` pass. Publication fingerprint
`5748434849109123582`, unchanged 556 textures / 883,633,616 mip bytes /
64,967,422 alpha bytes. Hidden captures with `blobber_review_blueprints capture
--art finished --room kitchen-hall --assets assets
--prepared cmake-build-debug/assets/prepared --ui-prepared cmake-build-debug/assets/prepared
--output RUN/pair` and the same command with `--rotation 90 --output RUN/pair90`
pass **50 views**, all with zero nonedge CPU/GPU picking disagreements and no GL
errors. The in-process closed walk loop goes around every side of the center table,
through both armor cells, and under the chandelier; movement and NPC availability
assertions pass in both orientations. Storage traversal, door/reward interaction,
injected GPU acquisition rollback and renderer/asset cleanup also pass.

Inspected normal/rotated hall overviews, central table, armor close-up, barrel and
both brazier animation phases. The chandelier is visible from the hall entrance,
with its hoop, candles and suspensions above the clear floor. Candles are unlit
static geometry, not additional fire effects. Six current native captures replace
the prior evidence in the [gallery](screenshots/blueprint-rooms-br02/README.md).

Pair residency: 51 textures, 114,800,228 mip bytes, 12,613,249 alpha bytes,
1,130,496 source mesh bytes. Scene CPU/upload: 2,796,632 / 2,795,856 bytes;
renderer CPU accounting 24,993,977; world GPU 142,429,532; peak application GPU
295,826,856. Existing capacities are unchanged. These are engine accounting,
not driver or whole-process memory measurements. No benchmark, other build
configuration, full campaign test or desktop interaction was run. User visual
acceptance and interactive review remain pending; accepted fireplace status is
preserved. All owned runners are closed and disposable output cleaned, with no
retained diagnostic exception. Sources, packed models and prepared caches remain.

### Furniture proportions and room dressing

Approved implementation: preserve the fireplace, floor areas and camera; rebuild
supports to put dining tabletop/bench seats/kitchen worktop at .28/.17/.32 cells.
Translate tabletop objects without stretching them. Remove the provisions pallet;
independent crates/sack grow 14%, barrels grow 14% in diameter and become 1.5 diameters
tall. Five nonblocking storage placements: large crate (1,0) and barrel (3,0) north;
small crate (0,3) and barrel (0,4) west; sack (1,4) south. Two .65×.9 heraldic
hangings begin .65 above the north hall floor at (1,0)/(5,0). Two blocking 1×1
braziers occupy (0,0)/(6,0), .30-wide bowls with .36-high rims, animated with existing
flame artwork. Woven burgundy/ochre heraldry uses retained imagegen masters.

Kit and SceneryPlacement gain default-true `blocking`. Visual validation bounds
remain independent of movement/NPC/reservation occupancy; all placed geometry is
still checked for bounds/terrain/other placements. Kit gains optional local fire
anchor, transformed with room/model and floor elevation. Runtime WorldDefinition
retains legacy hearth plus a preparation-owned vector of at most two additional
AmbientFire records. Map rejects excess/nonfinite/out-of-map anchors; frames read
the stable collection with per-fire visibility and at most 24 particles per fire.
No per-frame allocation, spawning, new lighting or interactions. Residency requests
include flame artwork when the map has fires. Persistence/defaults and hash extension
tags preserve old content identity when these optional additions are absent.

Keep existing bounded kit/placement/scenery/mesh/atlas/effect capacities; no generic
entity framework or runtime scaling. Recompute metric UVs on final geometry. Keep
all sources/editable models reproducible and fireplace geometry/material appearance
unchanged. Update blockouts for new forms. Room, lane, socket and reward contracts
remain; barracks/armory and campaign integration stay deferred.

Verification uses owned fixtures for decorative collision in four rotations, visual
bounds/overlap/attachment, two-fire visibility/caching/pause/invalid capacity, asset
residency, persistence compatibility and allocation-free frames. Build affected
Debug suites with configured CMake/Ninja, <=2 workers and one intensive job. Inspect
front/side/underside model views and hidden in-game close/overview/turn captures;
explicitly assess proportions, barrel silhouettes, separate storage and hall art.
Passing technical checks do not establish visual acceptance. Confirm traversal of
storage cells, doors/rewards, GL/picking/lease cleanup; retain six refreshed images,
clean owned temporary output, and deliver `./review-rooms` directly to the user.

### Scale and hearth correction

The user's review requests approximately 50% smaller furnishings, a coherent
hearth interior, matching wall materials and more geometric detail. This supersedes
the four pair-stage kit dimensions in the initial BR01/BR02 handoff. Author the
four models at half their initial linear dimensions, baked into source vertices;
runtime instance scale remains 1. Collision becomes 1×1 for hearth/work/provisions
and 1×2 for table-and-benches; height caps become .5/.175/.225/.2 respectively.
Whole-cell movement still requires rounding fractional furniture depth upward.
Wall-backed pieces sit against the back edge of their cell. Move kitchen hearth
to (0,0), kitchen/armory work-surface to (4,0), and right hall table to (6,1),
preserving corner/wall composition while releasing the old occupied cells.
Sockets, room footprints, ceiling profiles and reserved encounter/reward lanes
stay unchanged. Later unbuilt barracks/armory kits remain at their old contracts
until the pair's revised scale is reviewed.

Rebuild the hearth with fitted chamfered masonry, a shallow stone arch, lined
back/side surfaces, ash bed, supported grate, and rounded thick-rimmed pot with
handles/feet. Sample the existing dungeon wall artwork for the stone palette and
detail. Increase useful geometric detail within the existing prepared-mesh limits;
the earlier crude silhouettes were not an intentional pixel-art requirement.
Keep the existing atlas size, native texel density and unlit material pipeline.
UVs are computed after the half-size transform so texture detail does not double
in density. Validate the smaller blockout forms as well as finished models.

Focused checks: owned compact-kit transformed bounds/collision fixtures, full
authored socket/rotation validation, one Debug blueprint/render/scenery/fixture
selection, final production bake and matched hidden pair/90° captures. Inspect
the hearth at adjacent and overview distances and in an emission model view.
Replace superseded pair evidence, record limitations, and clean the owned run.

### Original pair design (superseded dimensions above)

Preserve all [accepted BR01 contracts](br01_blueprint_rooms_design.md): immutable
blueprints, whole-cell scenery occupancy, native pivots/scale, rooms, sockets,
rotations, encounter/reward reservations and ceiling profiles. Runtime receives
ordinary maps and scenery. No campaign generation, population, save version,
renderer schema or frame-time interpretation changes.

Author eight independent kits using imagegen concepts/material masters and a
dedicated reproducible headless Blender exporter. First checkpoint: hearth
(2×1×1), work-surface (2×1×.35), provisions (2×1×.45), table-and-benches
(2×3×.4). Later: sleeping-bay (1×2×.6), footlocker (1×1×.2), weapon-storage
(2×1×.8), armor-display (1×1×.8). Bounds are width×depth×height in cells;
origin is anchor-cell floor center, +X right/+Y up/+Z backward. No overhangs.
Hearth is cold; furnishings are static/decorative. Existing doors, pillars,
terrain and chest variants retain their owners and bindings.

Use Underkeep's named model bindings and existing scenery transforms, painted
triangles, picking, mips, scene preparation and CPU/GPU leases. Keep the four
legacy furniture slots unchanged. Review assembly gains an explicit pack
argument, defaulting to blueprint-blockouts; launcher gains `--art blockout|finished`
with compatible blockout default. Finished selects underkeep and checks every
referenced kit; missing/unbuilt rooms fail contextually, never fall back silently.
Preparation validation reads model payloads temporarily and verifies the
complete grouped bounds against the kit contract before starting the view.
Offline export validates the same bounds before registering files.

## Artwork and bounded preparation

Follow [the accepted prop guide](prop_modelling_guide.md): true thickness, fitted
joints, restrained grain, dull iron, dark recesses, painted shading and emission
preview. Occupied cells must look occupied; grouped tables/benches and storage
bases must not suggest traversable holes. No artificial blockout-color plates.

Maintain 256 texels/runtime cell with four-pixel gutters, checking both UV axes
and area. Each kit is one mesh of at most 2,000 triangles. First four kits share
one 2048² opaque atlas; reserve a second for the remaining kits. Full added mip
budget is about 42.7 MiB. Packing fails instead of rescaling, and existing
scene/resource/count limits remain unchanged. All authoring and validation
allocations occur outside frame processing. Retain prompts, concepts, texture
masters, scripts, packed blend, GLB and mesh JSON under durable source paths.

## Review and verification plan

Use the existing isolated launcher through GameSession/GameView/WorldRenderer/
Presenter. Add deterministic close/turn views and report art mode, publication
identity and resident counts/bytes. Review both rooms, their shared doorway,
floor attachment, ceiling clearance, door sweeps, reward access and table aisles.
Use hidden application-owned captures and in-process commands only; user operates
interactive walk. At pair checkpoint deliver selected captures and a walk command.

Extend owned small fixtures for pack selection, missing bindings, transformed
footprints/heights, invalid prepared data, lifetime/rollback and zero-allocation
CPU frames. Run focused Debug blueprints, blueprint_render, scenery, fixtures,
underkeep_render and world_residency tests plus a generated publication fixture.
At final BR02 checkpoint run generation regression once. Production captures
check GL errors, CPU/GPU IDs and cleanup; fixtures never depend on production art.
No benchmark, broad seed sweep, sanitizer/configuration matrix or desktop input.

Use configured CMake/Ninja, <=2 workers and one intensive job, low CPU/I/O
priority and bounded Blender threads. Own disposable output in an agent run,
promote <=12 selected images while awaiting review (<=6 at closeout), preserve
BR01 acceptance, then preview/apply cleanup. Record executed commands, results,
limitations, review status and any implementation changes below.

## Implementation and reproduction

User launch correction: the repository-root `./review-rooms` command opens the
finished kitchen/mead hall with normal controls. It resolves paths automatically,
updates only its required Debug targets using the configured CMake/Ninja, and
owns/cleans its temporary walk output. `--check` performs the same build and
headless content validation without opening a window; `--combat` adds the preview
encounter. The longer commands below remain developer reproduction details.
Launcher `--check` passed against the configured Debug build and finished catalog;
no interactive window was opened by the agent. Builds use one worker so asset
preparation jobs stay sequential. The launcher-change artifact run was cleaned.

Nine pair-stage kits are registered as Underkeep named models: hearth 1,794 triangles,
work-surface 1,496, table-and-benches 1,176, barrel 436, each crate 528, sack 174,
tapestry 260 and brazier 596. The obsolete grouped provisions model is removed.
The shared 2048² atlas uses 649 packed rows; its full mip chain costs
22,369,620 bytes (21.33 MiB). One atlas remains reserved for the later kits.
No production map, generation, save identity, resource ceiling or renderer format
changed. Adding unused pack bindings does increase the prepared Underkeep palette;
resident usage is measured in the hidden review rather than inferred from total
installed publication size.

`blueprintCatalog` is an optional source pack field restricted to
`blueprints/NAME.json`, included in build dependencies and both provenance scans.
The baker checks each registered kit, allowing the pair-stage subset. Finished
walk/capture validates every used binding before opening a window; finished
`validate` checks the selected room's published art in addition to the full catalog's
geometry variants. Default blockout assembly and launcher behavior remain compatible.
There is no runtime fallback for missing finished art.

New validation lives in `src/assets/blueprint_art.*`. The publisher retains ordinary
finite/normal/winding/UV validation; kit validation adds exact bounds, opaque/static
mode, single-part grouping and the 2,000-triangle cap. The exporter independently
checks positions, per-axis UV density and triangle count. Allocation and ownership
are unchanged: temporary reads end before scene startup, views own the existing leases.

Configured tools for this checkout are `/home/bison/clion/bin/cmake/linux/x64/bin/cmake`
and `/home/bison/clion/bin/ninja/linux/x64/ninja`; reread the cache if using another
build tree. All commands below run from the repository root.

```sh
BR_RUN=$(python3 tools/agent_artifacts.py create)
nice -n 15 ionice -c 3 /home/bison/clion/bin/cmake/linux/x64/bin/cmake \
  --build --preset debug --target blobber_review_blueprints blobber_bake_world_assets

cmake-build-debug/blobber_review_blueprints validate --art finished \
  --room kitchen-hall --prepared cmake-build-debug/assets/prepared

# User-operated interactive review; no campaign save owner.
cmake-build-debug/blobber_review_blueprints walk --art finished \
  --room kitchen-hall --catalog assets/blueprints/underkeep.json --assets assets \
  --prepared cmake-build-debug/assets/prepared \
  --ui-prepared cmake-build-debug/assets/prepared --output "$BR_RUN/walk"

# Hidden, application-owned images; no OS input or focus control.
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_blueprints capture \
  --art finished --room kitchen-hall --assets assets \
  --prepared cmake-build-debug/assets/prepared \
  --ui-prepared cmake-build-debug/assets/prepared --output "$BR_RUN/pair"
```

`--rotation 90` checks the rotated pair; `--combat` adds the existing fixed
goblin/archer encounter. `--room kitchen` and `--room mead-hall` work independently.
Finished barracks/armory intentionally reject missing kits until the review gate
is passed; blockout mode remains available through the BR01 separate publication.

Rebuild authoring outputs into a new run subdirectory, inspect them, then copy
selected source outputs to the pack before baking. Never regenerate into a capture
gallery. The authoring script does not update the pack manifest automatically.

```sh
nice -n 15 ionice -c 3 timeout -k 5s 30s blender --background --factory-startup \
  -noaudio -t 2 --python tools/export_blueprint_rooms.py \
  -- --output-root "$BR_RUN/authored"
nice -n 15 ionice -c 3 timeout -k 5s 30s blender --background --factory-startup \
  -noaudio -t 2 --python tools/review_blueprint_rooms_blender.py \
  -- "$BR_RUN/models"
```

Blender in this environment can stall in PulseAudio shutdown after successfully
writing every output, even with `-noaudio`. A timeout/interrupt is not a successful
process exit: confirm the completed export report, output files and independent
published geometry checks. Only task-owned Blender processes were stopped. Sandbox
thumbnail/config cache writes also fail harmlessly; all retained assets are in the
workspace and all textures are packed in the editable/interchange models.

## Furniture and room dressing verification

Implementation uses one matched heraldic tapestry design for both hangings. Its
[built-in imagegen master](../assets/packs/underkeep/source/rooms/heraldic-tapestry-master.png)
and [exact prompt](../assets/packs/underkeep/source/rooms/tapestry-prompt.json) are retained.
All nine models, the packed Blender/GLB and the atlas are reproduced by the exporter.
The accepted hearth's vertex positions and normals compare exactly with the preceding
revision; only atlas UV placement changes. No atlas or mesh capacity was raised.

Model review exported 27 front/rear/underside views. Inspected work surface, table,
barrel, tapestry and brazier fronts, table rear, and work/table/barrel/brazier undersides.
The brazier underside revealed an open outer bottom; added its cap, regenerated sources
and inspected the corrected underside. As documented below, Blender completed every
export/render but stalled during audio shutdown; bounded timeouts closed owned processes.

Configured Debug targets built with at most two workers. Seven affected suites pass
serially in 3.64 seconds: blueprints, blueprint_render, scenery, fixtures,
world_integration, world_residency and campaign_save_smoke. Tests cover decorative
footprints and local fire transforms in every orientation, visual-overlap rejection,
malformed/capacity rejection, two-fire animation/pause/independent culling and
allocation-free frames, explicit flame residency, and persistence of decorative scenery
and multiple fires. Existing legacy hearth behavior and ordinary save round trips pass.
The full slow campaign suite, second configuration and benchmarks were not run.

```sh
nice -n 15 ionice -c 3 /home/bison/clion/bin/cmake/linux/x64/bin/ctest \
  --test-dir cmake-build-debug \
  -R '^(blueprints|blueprint_render|scenery|fixtures|world_integration|world_residency|campaign_save_smoke)$' \
  -j 1 --output-on-failure
```

The default `blocking=true` and absent additional fire collection do not append hash
extensions. Sparse persistence inherits these defaults for missing fields, and fire
collections reject sizes above two before resizing. No save version or generated-world
identity changed, and no user saves were read or written. Current catalog geometry
passes 192 room variants and four pair orientations.

Final serialized UV density is 255.9911–256.0113 texels/cell. Sampled triangle-centroid
colors on the hearth match the accepted atlas exactly after repacking. Final production
bake succeeded: 556 textures, 883,633,616 mip bytes and 64,967,422 alpha bytes;
publication fingerprint `12382331285521188286`. Finished catalog bindings pass.

Two hidden runs (0°/90°, 21 views each) pass all 42 nonedge CPU/GPU ID comparisons,
GL checks, door traversal, reward access, explicit movement into decorative storage,
injected GPU acquisition rollback and complete view/renderer lease cleanup. Both
flame frame pairs change visibly (32,814/18,837 viewport pixels after .15 seconds).
Inspected kitchen overview/work/storage/barrel, table front/end/turn, tapestry,
brazier close-up, and hall composition in both orientations. Taller supports now
read clearly at the unchanged game camera height; barrel bodies are upright.

Each pair holds 51 textures: 114,800,228 mip bytes, 12,613,249 alpha bytes and
797,184 source-mesh bytes. Renderer CPU accounting is 23,617,772 bytes; world GPU
141,388,604, peak application GPU 294,785,928. Scene CPU/upload are
1,755,704/1,754,928 bytes in both orientations. No limits were raised; counts exclude
process-wide/driver memory. Fire uses existing artwork, with no new dynamic lighting.

`./review-rooms --check` passes with the final Debug executable, production assets,
portrait and creator caches ready. Interactive desktop input/play remains unverified;
no user window was controlled. The [six refreshed review images](screenshots/blueprint-rooms-br02/README.md)
retain current evidence while explicitly preserving fireplace and historical BR01
acceptance. Barracks/armory remain deferred. All owned runners are closed; the owned
run was preview-cleaned and removed after promotion, with no diagnostic exception.
Source artwork/editable models and normal build caches remain.

## Previous scale/hearth revision verification (fireplace accepted; furniture superseded)

All four source meshes now bake in the 50% linear-size correction. The hearth
has nine arch stones, fitted masonry courses, a lined firebox, ash floor, a
supported seven-bar grate and a 24-sided curved pot with rolled lip and handles.
Timber members have small geometric chamfers. Twelve emission previews were
exported; hearth front/rear/underside and the other three front views were inspected.
The Blender processes wrote their reports and outputs but required bounded shutdown
termination for the PulseAudio stall described above.

Configured Debug review/baker and four affected test targets built with two workers.
The focused `blueprints|blueprint_render|scenery|fixtures` selection passed serially
in 0.81 seconds. Its owned fixture checks that reduced footprints release the former
blocked cells for both party and NPC movement in all four rotations. Compact generated
blockouts and finished-mesh bounds use the same catalog contracts.

```sh
nice -n 15 ionice -c 3 /home/bison/clion/bin/cmake/linux/x64/bin/ctest \
  --test-dir cmake-build-debug \
  -R '^(blueprints|blueprint_render|scenery|fixtures)$' -j 1 --output-on-failure
```

Serialized UV Jacobian checks now measure 255.9829–256.0086 texels/cell; exporting
eight decimal places preserves tiny bevel-face UVs. Unrounded authoring checks
retain .001 tolerance. The four triangle counts are recorded above; no atlas,
mesh, scene or residency capacity was raised. The publication fixture and unrelated
renderer/residency suites from the initial delivery were not repeated: publisher,
resource ownership and runtime rendering are unchanged by this correction.

Final production bake succeeded using the configured CMake with one worker.
Publication fingerprint is `10468492512477437446`: 556 textures, 883,633,616 mip
bytes and 64,967,422 alpha bytes. Finished validation passes 192 room variants,
four pair orientations and every selected art binding. `./review-rooms --check`
passes with no further build work, so the normal walk command uses these assets.

Two hidden production runs (pair 0° and 90°, 15 views each) passed all 30 picking
comparisons with zero nonedge differences, GL checks, door traversal, reward access,
injected GPU preparation rollback and lease cleanup. Inspected both hearth close-ups,
room overviews, work/provisions and the table close/turn views. The reduced furniture
leaves more open floor inside the unchanged room dimensions, as expected.

Current empty-pair accounting: 114,669,160 resident mip bytes, 12,588,673 alpha,
710,784 source-mesh bytes, 23,135,629 renderer CPU bytes, 140,889,280 world GPU bytes
and 294,286,604 peak application GPU bytes. Scene CPU/upload are
1,387,448/1,386,672 bytes in both orientations. This includes the extra geometry
without changing existing resource limits; it excludes driver/process-wide memory.

[Six refreshed images](screenshots/blueprint-rooms-br02/README.md) replace the initial
eight. User visual acceptance and interactive play remain pending; no campaign,
benchmark or new configuration was run. Barracks/armory remain behind the pair
review gate. All task-owned runners are closed and disposable run output was
cleaned after promotion, with no diagnostic exception. Durable sources and normal
Debug caches remain.

## Initial delivery verification (artwork subsequently rejected)

Configured Debug targets built with two workers. Six focused suites passed in
1.03 seconds: blueprints, blueprint_render, scenery, fixtures, underkeep_render,
world_residency. Owned fixtures cover all kit orientations/raised-floor anchors,
art selection preserving collision, missing pack/model, height/footprint/triangle
failures, invalid UV/normal rejection, lifecycle/rollback and allocation-free CPU
frames. No second configuration, benchmark or broad generation sweep ran.

```sh
nice -n 15 ionice -c 3 /home/bison/clion/bin/cmake/linux/x64/bin/ctest \
  --test-dir cmake-build-debug \
  -R '^(blueprints|blueprint_render|scenery|fixtures|underkeep_render|world_residency)$' \
  -j 1 --output-on-failure
nice -n 15 ionice -c 3 env TMPDIR="$BR_RUN" python3 tests/blueprint_art_bake_tests.py \
  cmake-build-debug/blobber_bake_world \
  --cmake /home/bison/clion/bin/cmake/linux/x64/bin/cmake \
  --ninja /home/bison/clion/bin/ninja/linux/x64/ninja
```

The optional generated bake fixture passes: one tiny actor and kit, no production
artwork, no-op publication, catalog dependency invalidation, mesh/height rejection
and preservation of installed payload/metadata on failure. Its initial encounter
fixture was too small for BR01's 2×2 contract; corrected to a small 4×4 room and
reran only that check. The older broad world-build script was not run.

The first hidden pair review passed 14 views with zero nonedge CPU/GPU picking
differences, GL checks, door traversal, chest access, injected preparation rollback
and lease cleanup. Emission review of twelve front/rear/underside model views
exposed a coplanar overlap at the hearth's back/jamb seam; the exporter now fits
the back between the jambs. Source outputs and the publication were regenerated.
No layout/ceiling/collision changes were needed.

### Initial pair checkpoint

The corrected hearth's three emission views pass visual inspection. The other
nine original emission views remain applicable. Serialized final mesh UV Jacobian
checks measured 255.667–256.179 texels/cell on both axes (tolerance .5 texels/cell
for six-decimal serialized UVs on tiny faces). Export-time unrounded checks use
.001 tolerance. All four model bounds and winding passed the production baker.

Final publication: **556 textures, 883,633,616 mip bytes, 64,967,422 alpha bytes**;
fingerprint `2318549320953968193`. Total installed size includes unused other packs
and actors; it is not active GPU residency. Four final hidden runs passed:
pair 0° (14 views), pair 90° (14), standalone kitchen (8) and fixed pair combat (1).
All **37 views** have zero nonedge CPU/GPU ID differences and no GL errors. Both
room reward-access positions and the shared/exterior doors work. GPU acquisition
failure preserves the active image and byte counts. View teardown releases
texture/model leases; renderer teardown returns to the Presenter baseline, and
font accounting returns to zero before registry/window destruction.

| Measured bytes | Empty pair | Pair with goblin/archer |
| --- | ---: | ---: |
| Resident mip textures | 114,669,160 | 167,884,072 |
| Resident alpha | 12,588,673 | 22,583,929 |
| Resident source meshes | 388,032 | 388,032 |
| Renderer CPU accounting | 22,162,029 | 32,158,885 |
| World GPU accounting | 140,238,448 | 193,453,360 |
| Peak application GPU accounting | 293,635,772 | 346,850,684 |

Pair scene storage is at most 736,616 CPU bytes (90° case); mesh upload is 735,840
bytes. These remain below existing 64 MiB CPU/40 MiB GPU scene limits, 352 MiB world
CPU, and the current source-defined 928 MiB mip/96 MiB alpha/1024 MiB world GPU
ceilings. No capacity was raised. Accounting excludes driver/process-wide memory.

The sandbox's initial hidden capture could not open X11; authorized display access
was used for the successful hidden runs. No interactive desktop play, focus/input
automation, full campaign run, save migration, benchmark or second configuration
was performed. Generation regression is deferred to the final BR02 checkpoint as
planned; generation and campaign source code are unchanged.

The initial eight native images were presented for user review and have since been
replaced by the scale/hearth revision in the [current gallery](screenshots/blueprint-rooms-br02/README.md). **Next action: review kitchen/mead hall proportions, materials and
circulation readability; incorporate feedback before authoring barracks/armory.**
BR02 overall and BR03 are not marked complete or accepted. All runners are closed;
owned disposable captures/logs/exports were cleaned, with no diagnostic exception.
Durable art/prompts/models and the normal Debug prepared build cache remain.

After future review runs, close the launcher before cleaning:

```sh
python3 tools/agent_artifacts.py clean "$BR_RUN"
python3 tools/agent_artifacts.py clean "$BR_RUN" --apply
```

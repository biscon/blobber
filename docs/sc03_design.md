# SC03 — Vegetation, market, fountain and service signs

Status: **Ready for review — user corrections implemented** (2026-11-05).
The user approved the shorter representative tree: “beautiful tree, I approve,
continue”. That checkpoint is accepted; continue the already-approved SC03 scope.
The complete dressed city and eight-frame water animation are delivered for final review.
Campaign cutover remains SC04.

## Content and sequence

Preserve the accepted SC02 town, 1920×1080 presentation, camera, discrete movement,
supersampling, material batching and chunk/frustum culling. First deliver a modeled
low-poly trunk/branches with fixed clustered alpha-cutout foliage cards in the
courtyard. Review all sides, overlap, near/far silhouettes and turning. An opaque
canopy with edge cards is a prototype fallback, not a second foliage framework.

After approval, add a smaller eastern tree, bushes and planting beds in the three
reserved areas, selective facade ivy, two timber stalls with muted cloth canopies
in the market reservation, and a modest stone fountain in its 2×2 reservation.
Integrate double-sided timber signs/brackets in the service building meshes:
tankard for inn, anvil for smithy, provisions bundle for provisioner, no required
text. New painted art uses ImageGen and retains prompts and masters. Preserve
approximately 256 texels/cell, padded atlases and reproducible Blender/GLB/runtime
exports. No services, interiors, NPCs, gate operation or campaign cutover.

## World ownership and placement

WorldDefinition owns up to 32 SceneryPlacement values independently of solid
building shells. Each stores model name, anchor, local rectangular footprint,
cardinal facing and width/depth. Use small parallel building/scenery value types, sharing prepared transforms and
the existing named multipart pack catalog.
Resolve names only during preparation. Scenery bases occupy level open terrain,
reserve player/NPC movement, and reject overlap with other physical reservations.
They do not create solid terrain, hide generated floor/walls or block cell LOS;
actual painted geometry controls visible depth/picking occlusion. Canopies may
overhang clear routes above head height. No per-triangle movement collision.

City validation checks installed bases remain within their named SC02 reservations,
preserves protected approaches/anchors/loops and validates connectivity with all
future reservations blocked. Prototype tree uses a 1×1 base within courtyard
planting. City metadata remains preparation-only; scenery placements enter the
world fingerprint, bounded save codec and byte accounting immediately.

## Prepared geometry, alpha and limits

Reuse view-owned contiguous scene geometry and location asset leases. Add an
explicit cutout flag on named model mesh parts and prepared ranges; existing parts
default opaque. Fixed foliage threshold is 128/255. Bake cutouts with retained base
alpha masks and premultiplied mips; the opaque cutout shader unpremultiplies sampled
color. CPU raster and GPU color/ID passes use the same base-level alpha test before
writing depth. Export both card windings; do not add camera-facing tree behavior.
Cutout hits are noninteractive opaque depth, holes preserve underlying IDs/depth.

Group ranges by material/painted/cutout/variant, expand bounded range-count scratch
and fixed draw storage accordingly, and include all overhangs in chunk bounds.
Scenery-only scenes use architecture's existing candidate search radius. Preserve
static raster caching and moving-frame work avoidance. No frame allocations, asset
reads or mesh uploads. Existing active/staged rollback and resource release apply.

SC03 allows 24 city models (engine 32), 32 scenery placements, existing 128 building
placements (city architecture stays <=96), 32 materials, eight parts/model,
6000 vertices/part, 24000/model and +/-4 horizontal / 0–4 vertical model bounds.
New art ceilings: one opaque 4096² atlas, one cutout 2048² atlas and one water 2048²
sheet. Existing 64/40 MiB scene CPU/GPU, 8 MiB view scratch, 512 MiB mip textures,
64 MiB alpha and 352/608 MiB aggregate CPU/GPU ceilings stay unchanged. Preparation
rejects exhaustion clearly without replacing the active playable view.

Version changed publication/save/visibility identities in this slice; old saves
reject without modification or migration. No delayed SC04 codec work.

## Optional pixel-art water (second checkpoint)

Try eight painted frames at 8 FPS on fixed opaque basin-water/stream geometry,
with stepped highlights/ripples and no frame blending. Bake padded frames/mips
offline; use FrameInput::now and its existing pause policy. Change texture selection
only, invalidating color when visible water changes without rerasterizing picking
or uploading meshes. No particles, transparency sorting, reflections or fluid
simulation. After one focused art correction, defer to static painted water if
the result is unconvincing or needs broader renderer infrastructure. The user
explicitly authorized postponing animation in that case.

## Verification and handoff

Owned small CPU/publication fixtures cover footprint movement/sight separation,
overlap/bounds/capacity failures, city routes, cutout holes and covered depth,
front/back/oblique cards, chunk bounds, persistence roundtrip and malformed input.
Hidden GPU checks cover pixel ID/depth agreement, staged failure/cancel/release,
stationary/moving warmed allocations and cache/upload behavior. Production tree
captures are separate visual evidence, not correctness fixtures. Second checkpoint
adds market/sign/fountain views and water timing/pause/cache tests if implemented.

Use one Debug configuration, focused affected suites, one relevant hidden GPU pass,
and save smoke for schema changes. One intensive job at a time, low priority,
<=2 build workers, one test process, bounded Blender threads; no routine benchmark
or desktop input automation. Record exact checks, captures, totals, limitations,
regeneration commands and next action in sc03_verification.md. First stop is user
tree review; final stop is SC03 Ready for review, never automatic SC04 continuation.

## Tree review correction — 2026-11-05

The user liked the initial in-engine tree but requested a shorter silhouette so
more canopy remains visible nearby. Lower the trunk/crown and reduce clump size,
spreading clump centers to retain overall canopy width and metric texture detail.
Keep the 1×1 base, head clearance, renderer and camera unchanged. Capture matching
views before requesting representative-tree acceptance.

Implemented interfaces at the tree checkpoint: publication 7, appearance 4,
campaign schema/simulation 9/12, generator/templates 9/7, visibility v15 and
world fingerprint 4. See [verification/handoff](sc03_verification.md) for delivered
checks, resource totals, shorter-tree captures and the approval boundary.

## Implemented dressing and water — 2026-11-05

Reuse the accepted courtyard tree unchanged inside a 3×2 garden model; add a
2×1 lane bed and a 2×2 eastern bed with a smaller tree and low bushes. Their
physical bases exactly fill the three planting reservations. Two east-facing
1×2 stalls fill the market reservation. A 2×2 fountain uses low modeled stone,
opaque basin water and fixed narrow falling-water surfaces. No routes move.
Two facade variants add sparse ivy; the three service model bindings gain
modeled double-sided signs with generated tankard/anvil/provisions artwork.
Offline dressing export retains Blender, unlit GLB, runtime meshes and source
art, sharing padded atlases at approximately 256 texels/cell.

Water adds one explicit opaque architecture-part flag, carried through publication
8 and prepared range grouping. The material is a padded 4×2 atlas of eight
512-pixel tiles; authored UVs occupy the first tile. GPU color sampling offsets
UVs to `(FrameInput::now / 125) % 8`, using the existing paused gameplay clock.
The view stores current/drawn phase and whether culled ranges include water.
Only a visible phase change invalidates color caching. CPU visibility, picking
identity and uploaded geometry remain static. No frame allocations or uploads;
normal geometry uses the same shader with water selection disabled. Range-count
scratch and draw storage include the extra bounded grouping bit. Add generated
small water fixtures for phase boundaries, cache invalidation, pause, unchanged
IDs and warmed allocation/upload invariants. Static water remains the authorized
fallback if the bounded attempt requires broader rendering changes.

Variant bindings reuse existing mesh paths. The offline baker interns named parts
by source path within each pack, so ivy/garden variants share published mesh IDs
and resident storage with their original models. This lookup is preparation-only.

Final outcome: all six reserved scenery bases, two ivy variants and three service
signs are installed. Eight water frames and cache/pause tests pass; static fallback
was not needed. Shared opaque atlas fits 2048², cutouts 1024², water 2048×1024.
Cutout/water flags default false. Water is rejected on nonarchitecture/cutout parts
or UVs outside the first 4×2 tile; shader offsets apply only to color. Noninteractive
geometry-only views keep phase zero. Conservatively treat a visible chunk with a
water range as animated, even when actual water pixels are occluded. Final content,
resource totals, checks and limitations are in [verification](sc03_verification.md).
The first tree approval is satisfied; the final SC03 acceptance is pending.

## User review corrections — 2026-11-05

The user supplied seven in-game screenshots identifying oversized/back-alley
stalls, ivy above the roof, crowding between the dungeon entrance and inn,
floating bushes, paving recolored as soil, low fountain/ground contrast and
edge-on disappearance of flat water streams. These are corrections within SC03.

Move dungeon approach/fixture from (14,6) to (13,6) and derive its landing from the
new doorway, opening two clear cells beside the inn while retaining a one-cell
lane beside the sanctuary. Move the market reservation to (8,15), 2×1, south-west
of the fountain square. Two north-facing 1×1 bases hold stalls authored at 50%
linear size, centered within each base. Retain the fountain's two-cell circulation
margin, both named loops and gate/rest/dungeon access; update placement tests.

Keep ivy inside each facade's actual eaves and horizontal extent, using the
existing artwork's UVs for the retained section rather than stretching the entire
vine. Low-house ivy ends below 1.20-cell eaves; tall-house ivy stays below 2.15.
Lower and shorten bushes, placing foliage through the soil surface so transparent
card margins cannot leave crowns hanging. Add small modeled woody stems rooted
in the bed. Replace recolored paving with a dedicated generated earth material,
and use generated pale marble for fountain stone, all in the existing atlas.

Replace each flat stream with a fixed six-sided swept tube following the existing
curve, with closed end caps and continuous distance-based water UVs. This gives
nonzero width from every azimuth without camera-facing transforms, per-frame mesh
uploads or changes to CPU/GPU depth agreement. Reuse the eight-frame water atlas,
clock and color cache unchanged. Geometry corrections require no schema change.
Verification: focused layout regression checks, export geometry/material bounds,
hidden captures of each reported issue and an eight-angle fountain sweep, plus
the existing paused water capture. No unrelated renderer or test-matrix changes.

### City atmosphere correction

The subsequent city screenshot shows an abrupt near-total sky-color fade across
the square. Add a pack-owned `daylightFog` triple (start distance, transition span,
maximum opacity), defaulting to the existing outdoor values (10, 4.5, 1).
Sanctuary-city alone overrides this to (4, 12, 0.22): a long, shallow haze that
retains the enclosing walls. The indoor formula (9.4, 3.5, 1) stays unchanged.
Both world and billboard/effect shaders receive the same prepared pack uniform;
this adds no allocations, geometry work or uploads to the frame path.

Publication 9 serializes and validates the triple (finite values, start 0–64,
span greater than 0 and at most 64, opacity 0–1). The baker supplies legacy defaults
for packs without an override; save and visibility formats stay unchanged.
Verify publication roundtrips and rejection boundaries with generated fixtures,
and compare GPU images for capped outdoor haze and identical indoor rendering
across the two profiles. Inspect a matched city-square capture for the final art.

Review outcome: all corrections are implemented and captured. Focused CPU tests,
generated GPU atmosphere/architecture/foliage/water checks, eight fountain angles
and the paused animation comparison pass. See the updated verification and gallery.
SC03 is Ready for review; the accepted tree remains unchanged and SC04 is deferred.

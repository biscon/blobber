# SC01.2 — Architecture foundation and finished street corner

Status: **Implemented; Ready for review** (2026-11-05).
The user approved the detailed conversation plan and requested implementation.
SC01.1 remains accepted. This slice does not replace the campaign hub.

## Deliverable and ownership

The delivered residential L-corner contains a low house, a two-storey house, finished
paving, recessed stone arches and connected walking access around both shells.
It uses the existing camera, discrete movement, painted rendering, default supersampling
and native HUD. Interiors, services, greenery, fountain and market remain deferred.

Immutable WorldDefinition owns up to 128 static building placements, independently
of furniture DecorationKind. A placement contains a stable pack-local model ID,
grid anchor, cardinal rotation and local rectangular footprint. Unit scale is
fixed. Footprints must cover level, solid terrain, without overlapping ownership.
Map preparation derives a cell ownership mask; generated faces bordering those
cells are omitted, while solid terrain continues to block movement and sight.
The opaque exterior shell supplies all visible faces and real doorway recesses.
Door names and approach cells are authored review metadata, not portals.

Pack metadata adds up to 32 named static models, each with up to eight material
mesh parts and 24,000 total vertices. Existing furniture bindings and their tighter
validation remain. Architecture meshes retain the 6,000-vertex part limit, with
an explicit profile allowing local x/z within +/-4 cells and y within [0,4].
Payload, winding, normal, UV and material validation apply at bake/acquisition.
Names resolve once during preparation to checked typed indices; frames borrow
prepared geometry. SC01.1 CPU/GPU leases own selected pack dependencies and roll
back failed staging without changing the active view.

## Geometry and storage

Scene preparation emits each placement once into its anchor's 8x8 chunk in painted
opaque mode with no interaction ID. Bounds include every transformed vertex.
The original plan underestimated diagonal frustum reach: the far corner reaches
about 21.54 cells on a world axis during a turn, before the four-cell overhang.
Building scenes therefore search a bounded 32-cell radius (at most 81 chunks);
other scenes retain 24 cells/49 chunks. The camera far distance stays 16. Owned
regressions exercise cardinal and diagonal overhangs outside the anchor frustum.
Draw storage includes both painted and terrain ranges per material per candidate
chunk plus existing fixture/container ranges; preflight rejects excess explicitly.

Preserve 32 materials per pack, 64 MiB CPU/40 MiB GPU scene, 8 MiB view scratch and
352 MiB CPU/608 MiB GPU tracked-world ceilings. Count new vectors, names and masks.
Preparation may allocate; warmed movement, rendering and culling may not load,
resolve names, create resources or grow storage. No streaming, new visibility
hierarchy, runtime instancing framework or alternate renderer.

## Content and review

The immutable scene factory supplies a 16x16 outdoor scene, a 3x3 low house and
3x4 taller house around a two-cell turning street, with rear access and farther
roofline viewing positions. The low house is anchored at (5,5), facing South;
the taller house at (11,8), facing West. Named door approaches are (5,7) North and
(8,8) East; spawn is (7,10) North. Local footprint offsets are (-1,-1) and (-1,-2).
Complete exterior shells use reusable Blender components exported offline, including
closed corners, roof undersides, thick rounded arches,
inset timber doors/windows, limestone, worn pale plaster, dark roofs and dull iron.
Original ImageGen masters provide 256 texels/runtime cell, a shared padded 4096²
atlas and painted recess/underside shading. The low/tall shells contain 821/1,419
triangles. Prompts, masters, script, packed blend, GLB and runtime exports live in
the dedicated [sanctuary-city pack](../assets/packs/sanctuary-city/README.md).
Actual camera captures guided proportion, reveal-normal and UV corrections.

Material seam correction (implemented, 2026-11-05): the generated six-panel master
contains divider pixels that the original exact-grid crop retained. The offline
exporter samples an eight-pixel inset interior of each 512-square panel, shared by
terrain and atlas generation, then mirrors those clean fields. The original master
stays intact. Atlas gutters are 16 pixels, with fully edge-filled slots aligned to
16 pixels; first-fit shelves reuse earlier gaps within the existing 4096² capacity.
Synthetic colored regions remain isolated through mip level 4. This does not claim
isolation at arbitrarily coarse mips or every anisotropic footprint. Runtime shaders,
filtering, texture density and geometry are unchanged. Synthetic bordered panels
test extraction, repeat joins, padding and packing. Selected hidden near/oblique/far
captures verify the production result; see [before/after evidence](screenshots/sc01/material-seams/README.md).

--sanctuary-city-review launches a deterministic standalone preset party with no
NPCs, normal movement/HUD and no campaign save or character-library initialization.
An opt-in hidden capture tool uses the same scene factory and renderer, scripted
in-process actions, native screenshots, pose metadata and resource accounting.
Its optional exact shot-name argument exports one selected view; default execution
exports all 39. The `materials` selection exports five views for material review.
Output must be a new directory so earlier evidence is preserved.
Never control desktop input, focus or unrelated windows.

## Formats and verification

World publication advances to format 6, appearance revision remains 4. Serialize
placements, bound them on decode, and include them in canonical world fingerprints
(revision 3). Campaign schema advances to 8; simulation/generator identities remain
unchanged. Visibility advances to world-visibility-v14. Unsupported saves reject
without modification or migration; strict recordings retain identity checks.

Owned generated fixtures cover rotations, footprints, reservations, references,
bounds, connectivity, wall suppression, chunk visibility, draw capacities, staged
rollback, dependency checksums, lifetime release and serialization. Run focused
dungeon/furniture/visibility/save-smoke/allocation callers in one configuration.
Hidden GPU checks cover depth, CPU/GPU agreement and resources; production captures
are a separate art check. One intensive job at a time, low priority, <=2 build
workers, one test process; no routine benchmark or multi-configuration matrix.

The [verification/handoff](sc01_verification.md) records commands, outcomes, skipped
checks, captures, actual totals and limitations. This delivery stops at Ready for
review; only the user accepts the finished visual checkpoint and authorizes later
slices. Memory reference lines
remain advisory, with no silent fidelity/supersampling reductions.

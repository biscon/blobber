# Ground-anchored leaf occlusion — revision 2026-11-04

Status: implemented and mechanically verified; interactive acceptance remains user review.
This revision supersedes the leaf
strip masks below; their historical verification did not establish coherent
creature/leaf overlap.

Gameplay footprint, interpolated ground anchor and artwork bounds are independent.
A billboard straddling the extended broad leaf slab selects one relation from the anchor and
camera sides of the broad leaf plane: same side is in front, opposite sides or
an anchor within the thickness is behind. Camera-in-slab uses geometric depth.
Bodies entirely on one side of the broad slab use geometric depth. A 1e-5 world-space tolerance resolves
contact conservatively. Dense free-tip captures exposed a silhouette pop when classification used only the
finite box; extending the classification plane removes that contact-dependent switch.
All rejection is still limited to ray intersections with the
finite box (including height); static frame/terrain masks remain. No fitting,
position shifts, movement changes or size inference from artwork.

Prepared, bounded leaf queries are shared by CPU picking and GPU colour/IDs.
Each overlapping leaf is evaluated independently. Separate leaf-free scenery depth
from full scenery and composite surviving creatures after normal creature depth.
The GPU owns reusable transparent creature targets at both existing resolutions;
CPU owns reusable raster scratch. Base coverage and filtered fringes retain their
existing contracts. Owner badge clipping uses the same anchor relation; ground
picking and independent effects retain physical scenery depth. Guardian emblems
remain attached to their leaf. No per-frame allocations, resource uploads or
readbacks; explicit capacity failures and existing 8 MiB CPU visibility limit.
All added resources use the existing RAII registry and preparation lifecycle.

The rendering helper accepts world-space anchors/bounds, never occupancy masks.
Future multi-size gameplay must address placement, occupancy, navigation, travel,
combat geometry and persistence separately. Terrain mask capacity currently relies
on artwork width <= 1 cell; do not relax that validation here. A creature genuinely
straddling a leaf may need a richer occlusion proxy than a single anchor.

Verification: owned generated fixtures for both sides, headings, camera turns,
swing states, slab ties, finite top/free edge, multiple leaves, wide/cropped/prone
artwork, transparent holes, hidden targets and CPU/GPU agreement. Check allocations,
idle reuse, GL errors, resource release and both rendering qualities. Inspect
hidden application-owned production motion sequences around tip/plane crossings;
large abrupt changes require refinement. Preserve saves/simulation and advance
visibility identity. Use one low-priority intensive job, <=2 build workers; no
OS input automation. Interactive acceptance remains user-owned.

## Delivered revision and verification

`DoorBox::leaf` distinguishes moving slabs from static frames. `LeafQueries`
prepares at most 32 finite boxes with inverse axes and one anchor-side relation;
CPU ray rejection and the GLSL shader use equivalent slab tests. Projection culling
conservatively omits leaves outside a body's rectangle; owner badges use uncropped
queries. No actor/subcell occupancy is consulted by the helper. Static frame and
terrain UV masks remain separate, including existing cropped-canvas registration.

CPU rasterization caches leaf-free opaque scenery, retains that depth in each
published frame, and uses one reusable `PickingBuffer` to compose surviving bodies
over full scenery. Full physical opaque depth remains available to ground picking.
A non-owning function/context pair lets `PickingBuffer::badgeVisible` consult its
immutable owning frame; this callback never escapes the frame/pipeline lifetime.
Body indices avoid searching for an owner inside each badge-pixel query.
The prepared pipeline and effects remain below the 8 MiB view scratch budget,
including allocated raster arrays, both frames and fixed capacities. The subsequent
[movement performance work](movement_frame_work.md) adds bounded cached leaf relations
and visible outline bounds, and skips unavailable picking rasters during movement.
The CPU allocation and integrated resource-budget checks cover those additions.

GPU rendering copies leaf-free scenery/nonbody depth into a reusable transparent
creature target, completes scenery/leaf artwork, then draws and composites bodies.
The two sampled-depth targets add 5,640,000 accounted GPU bytes across 500x282 and
1000x564 resolutions. GL 3.3 framebuffer depth blits stay entirely on the GPU.
Only surviving body colour/IDs replace scenery, with actual body depth published
for subsequent effects. Effects explicitly test physical leaves. Shader capacity
assertions cover 136 UV rectangles plus 32 three-vec4 leaf queries within the
GL 3.3 minimum fragment uniform capacity. Existing resource registry limits and
active/staged budgets remain checked; sampled targets use existing lease cleanup.
Visibility is now `world-visibility-v13`; protocol/save/simulation formats are unchanged.

Release application, headless and affected diagnostics build with two low-priority
workers. Eight focused CPU/headless suites passed across the recorded runs:
`targeting`, `automation_allocations`, `world_cpu`, `sprite_crop`, `world_visibility`,
`world_visual_cli`, `world_integration`, and `world_environment`. After the final
free-tip refinement, only the five affected renderer/visual suites were repeated;
all passed in 0.73 seconds. New regressions cover preservation of silhouette before
finite-tip contact, camera-in-slab geometric fallback, conservative boundary ties,
multiple leaves, four rotated constructions, unchanged anchor-side classification
under supported width/crop changes, and owner-bound badges in front of nearer leaf
depth. Existing traversal tests retain static-jamb/lintel exclusion instead of
incorrectly asserting that every visible pixel must avoid a moving leaf's volume.

Final hidden generated-art GPU visibility and integration diagnostics passed on
RTX 3060/OpenGL 3.3. CPU/GPU ID differences were zero in both executed suites.
Checks include opaque flashed-body colour at both qualities, idle reuse, changed
frame C++ allocation guards, texture/mesh transfer counters, effects, travel rollback,
GL state and resource release. Allocation claims exclude graphics-driver internals.

[Review gallery and motion](screenshots/ground-anchored-doors/README.md) derive from
76 hidden native captures of two approaching creatures plus a rear creature, using
production goblin/bugbear artwork from both sides, denser free-tip samples, and close
views. The first finite-volume implementation visibly popped a shoulder into view
between 375 and 400 ms; classification against the extended broad slab corrected
that discontinuity. Tip/exit contact sheets and close views were inspected. Static
jamb/lintel occlusion and authored animation frame changes remain visible. This is
a 2.5D ordering convention, not a physical body/door collision model; multi-size
creatures and actual straddling bodies remain future work. No user save was loaded
or modified, and no mouse/keyboard/focus automation was used.

The first CPU run correctly rejected the obsolete leaf-volume assertion; it was
replaced with behavior checks, not relaxed for static scenery. The first visual-CLI
run caught its expected identity still being v12; updating that expectation passed
strict replay checks. Production capture initially lacked the newly added copied
map icon; the ordinary `blobber_assets` target supplied it. Capture also caught a
colour-only blend/shader-state error invisible to ID parity: raylib flushes its
batch on blend changes. Binding the compositor after that transition fixed it;
a flashed-body colour regression now exercises this path. These failed/intermediate
runs are not counted as passing verification.

Final logs: `/tmp/blobber-leaf-delivery-build.log`,
`/tmp/blobber-leaf-delivery-gpu.log`, `/tmp/blobber-leaf-delivery-integration-gpu.log`,
and `/tmp/blobber-leaf-stable.log`. Earlier captures remain in task-owned `/tmp`
directories; the repository retains a compact review gallery. No benchmark, stress
matrix, Debug rebuild or interactive gameplay review was performed.

## Historical implementation

# Creature size and scenery occlusion

Status: implemented and mechanically verified (2026-11-03); interactive acceptance remains user review.

Creature billboards retain their authored animation width, height, registered
floor anchor and shadow radius. Remove wall/door/viewport scaling and door-side
anchor searches. Perspective and authored animation dimensions still apply.
Frustum clipping handles screen edges. Existing depth handles scenery in front.

Flat billboards can extend inside solid scenery, where depth alone is insufficient.
A shared CPU helper expresses those intersections as rectangles in stored texture
UVs, preserving the logical canvas/crop mapping. Each frame records terrain
rectangles in a fixed array indexed by actor slot, separate from loot/fixture
billboards: a <=1-cell-wide plane crosses at most four grid cells, each supplying at most two masks (below floor / above ceiling), or one
full-height solid-cell mask. Use base-map solidity, not the closed-door gameplay
cell: doors are clipped against their actual prepared boxes instead. Terrain
masks follow surface elevations; absent ceilings do not clip the sky.

Drawing and CPU rasterization combine the eight terrain masks with up to
doorBoxCapacity (128) intersections with the frame's oriented leaf/jamb/lintel
boxes. A reusable 136-rectangle scratch array bounds work and storage without
per-frame allocation. Actor-only terrain storage adds 67,584 bytes across the two
frames; it does not reserve masks for the much larger loot capacity. Door
horizontal slab intersections and vertical extents must both overlap the cropped sprite before producing a mask. Small zero-area
intersections are ignored; capacity errors must be explicit, never silently
truncate masks or restore shrinking. Each frame owns its terrain masks; the
shared helper borrows the immutable frame/door boxes only during a draw/raster.

CPU rasterization rejects masked UVs before alpha coverage, body IDs and geometry
exposure. The GPU shader uses the same rectangles for body colour, filtering fringe
and IDs, within OpenGL 3.3's minimum fragment uniform capacity. Disable masks for
shadows, decals, guardian emblems, loot and effects. Shadows use existing world
depth without body-mask projection. Attachments and badges keep their full-size
registered transforms and existing depth/visible-body UI gates.

Keep the eight-MiB visibility scratch budget and idle view reuse. No new texture
or mesh uploads, GPU readback, simulation/asset/save format changes. Visibility
identity advances v11 -> v12; strict old visual replay rejects the new identity.
The old sprite-fitting section of door_chest_followup.md is superseded; its door
UV/chest changes and historical evidence remain valid.

## Verification

Generated small fixtures check unchanged dimensions/anchors/shadows/attachments,
wall and viewport partial bodies, four headings/subslots, walk directions, camera
turns, closed/intermediate/open door motion, floor/ceiling variation, cropped
canvases, wide/prone frames, transparent holes and overlapping targets. Check
partial vertical door intersections and fully hidden bodies/candidates explicitly.
CPU/GPU generated-fixture IDs agree except at one-pixel raster boundaries; check
mask state does not leak into other draws. Changed/idle CPU and GPU frames retain
zero engine C++ allocations and no resource uploads; resources release normally.

Run focused visibility, sprite-crop, world integration/environment, targeting and
visual-CLI checks in Release, sequentially. Hidden application-owned production
captures show goblins and wide creatures traversing a doorway from both sides;
inspect fixed-camera motion samples, native presentation, and GL errors. No OS
input/focus automation. Interactive review remains user-owned. At most two build
workers, one intensive job, low CPU/I/O priority; no benchmarks or broad matrix.

## Delivered implementation and checks

`world_billboard_clip` owns terrain and oriented door intersections, registered UV
conversion and mask membership. Visibility prepares terrain masks once per changed
frame; CPU rasterization and the GPU colour/fringe/ID shader consume the same
combined rectangles. No actor dimension, feet or shadow is fitted to scenery.
The obsolete clearance/scale/anchor-search code and its door ray API are removed.
The shader's 136 vec4 rectangles fit within GL 3.3's guaranteed fragment uniform
capacity; a compile-time check ties its capacity to the CPU definition.

Release application, headless, affected tests and GPU diagnostics build with two
low-priority workers. Eight selected CPU/headless suites pass in 1.04 s:
`targeting`, `automation_allocations`, `world_cpu`, `sprite_crop`,
`world_visibility`, `world_visual_cli`, `world_integration`, `world_environment`.
After adding the final viewport regression, only `world_visibility` was rebuilt
and repeated; it passed in 0.15 s. Tests cover stable dimensions and attachments,
masked door-volume picking, fully hidden candidates, heads above doors, partial
onscreen/offscreen body axes, floor/ceiling boundaries and outdoor ceilings.

Hidden generated-art GPU visibility/parity/lifecycle checks and broader door/
effect/travel integration pass on RTX 3060, OpenGL 3.3. CPU/GPU ID differences
are zero in the executed fixtures. Allocation guards and transfer counters pass;
the claim covers engine C++ frame processing and resource uploads, not graphics
driver internals. The final GPU-only rerun adds wide attack frames, terrain
floor/ceiling masks, oblique camera samples and body-to-loot mask reset. Existing
quality/crop/alpha and lifecycle checks remain active.

[Twenty-four inspected native production captures and an overview](screenshots/creature-occlusion/README.md)
show goblin and bugbear pairs approaching/traversing an open doorway from both
sides. Creature dimensions, hidden-window state, GL errors and resource release
are checked by the capture diagnostic. Perspective changes projected size with
distance; no scenery-dependent scale or lateral correction remains. No user save
was loaded or changed. Interactive gameplay is unverified.

The first test build exposed excessive stack use from storing masks on every
billboard; actor-only storage corrected it. New fixtures initially had an open map
perimeter and placed an intended embedded body just outside the leaf; both setup
errors were corrected. The initial production capture runner queried the active
renderer while its GameView was still staged; activating and retaining that view
fixed the runner. These failures are not counted as passing verification.

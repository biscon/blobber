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

# R05 — Heights, outdoor hub and decoration

Status: **Accepted and closed** (2026-10-31).
User approved the reviewed plan with “Implement the plan.” R04/R-B are accepted.
Parent: [renderer roadmap](renderer_plan.md). The user accepted R05 with
“slice accepted, record it in the plan doc”. Verification results and limitations
remain unchanged. At the R05 delivery checkpoint, R06 was eligible for separate
planning and had not started.

Current closure: [R06 and the full renderer roadmap](renderer_plan.md) are Accepted
and closed. Rollout/next-slice notes below record the R05 delivery checkpoint and
are superseded by that closure; R05 evidence and limitations are unchanged.

## World ownership and rules

Map owns an immutable row-major WorldDefinition (quantized surfaces, environment
references and bounded decorations); Map-only GeometryViews retain these values.
The map outlives all session/render borrowers. Legacy text maps explicitly default
to floor 0, ceiling 1, dungeon materials/profile. Integer quarter-cell elevations
bound floors to [-32,32], ceilings to [-28,48] ticks; absent ceilings use a sentinel.
Open cells require four ticks of clearance. No stacked cells, ramps or free motion.
One shared cardinal edge query requires open endpoints, at most one tick difference,
and four ticks common clearance. Player/actor movement, BFS and validation share it.
Melee/touch/adjacent fixture and loot access additionally respect height and clearance.
Sight uses center-to-center segments at floor+2 ticks, checking crossed floor/ceiling
intervals with conservative supercover corners. Solid cells stay full-height blockers.
Ranges, costs, clocks and spell footprints remain 2D. Generated floors remain flat.

## Geometry, presentation and storage

8x8 preparation subtracts adjacent free vertical intervals to emit floors, optional
ceilings, risers and lintels. Scene envelope spans minimum floor through maximum
finite ceiling/highest floor+1. Chunk bounds include actual geometry. Count/reject
before filling; scratch is preparation-only. Camera floor elevation uses existing
movement easing, then adds .5 eye height; FOV/horizon/setback/timings stay unchanged.
Actors use their existing travel endpoints/time (frozen at death); attachments,
shadows, loot, fixtures and effects follow surfaces. Raised doors require level
threshold approaches and fill their aperture; shared 300ms barriers stay authoritative.
Ground picking ray-walks at most 64 cells, intersects floors, checks published opaque
depth and existing spell eligibility; no extra full-screen buffer or frame allocation.

Dungeon lighting is unchanged. Fixed daylight profile and world-direction procedural
sky have no depth/IDs; camera profile interpolation follows movement. Outdoor fog
ends before far clipping and matches the sky horizon. Static scene sky remains visible
through uncovered cells even from indoors. Wall cutouts use prepared alpha, .001 bias,
stable order and no IDs. Opaque bench/urn meshes batch with static geometry. Small
nonblocking props fit perimeter strips <=.25 units high/wide; larger props require
solid cells. Validate attachments, coplanar overlaps and protected areas.

Limits: 256 decals, 128 props and 16 MiB world data per map. Existing 262144 quads,
65536 ranges, 32 materials, 64 MiB CPU/40 MiB GPU scene, 8 MiB view scratch, 352 MiB
aggregate subsystem and registry limits remain. Count capacities and active+staged
owners; any exhaustion rejects preparation without replacing the playable view.
Assets and GPU leases retain R04 ownership and rollback order. No steady-frame
allocation, decoding or resource uploads; source generation/export may allocate.

## Integration and compatibility

Authored hub retains 9x9 topology, entry/sanctuary/stairs and all floor heights. A
roof over (4..7,4), daylight courtyard, two wall motifs and perimeter benches/urns
add presentation only. Hub preparation lives outside campaign orchestration. Legacy
keeps enclosed flat presentation; nonflat legacy scenes reject before preparation.
A versioned r05-heights-v1 fixture is shared by live review and headless scenarios.

Checkpoints retain full WorldDefinition plus canonical fingerprint in every location;
restore validates traversal/progression/landings before publication. Never reroll
saved decorations. Campaign schema/simulation 2/2 reject old files unchanged. World
publication format 1/appearance 4, visibility world-visibility-v3, terrain-rules-v1
strict replay identity; protocol and generator/template versions stay unchanged.
Travel/Continue prepare simulation and first view before committing. Failure retains
session, Party, image/IDs and save. No R06 default cutover, monsters, balancing or PC07.

## Validation

Owned CPU fixtures cover quantization, interval mesh/culling, steps/clearance,
player/NPC/navigation agreement, corner/symmetric sight, reach, raised transforms,
ground picking, decals/props and capacity/allocation failures. Hidden GPU fixtures
compare depth/IDs, alpha and sky/quality paths, restore GL/UI state and exercise
staging rollback/cleanup. Persistence checks cover full definitions, inactive floors,
travel/recall/Continue, corrupt/old versions and strict CLI/client/local MCP replay.
Use the [verification workflow](verification_workflow.md): affected CPU/save smoke
checks and one hidden GPU integration test for final corrections. Earlier broad
runs are evidence, not a requirement to repeat the full matrix or benchmarks.
Publication tests apply when baking changes; stress/timing checks are opt-in.
Capture deterministic motion
and stills without desktop input automation. Record actual results/limitations in
[r05_verification.md](r05_verification.md); only user acceptance closes R05.

## Implemented interfaces and final workflow

`Map` owns `WorldDefinition`, validates bounded records once and caches flatness.
`GeometryView::canStep/reach` share surface clearance; runtime sight uses rational
cell-entry/exit heights. Actor walk snapshots retain starting elevation, including
death freezes. Scene preparation retains a stable map borrow and alpha cutouts;
windowless and GPU preparation both validate material references. Active/staged
world byte accounting includes borrowed world definitions once per distinct map.
The scene builder uses checked preparation scratch instead of a fixed face array.

`sanctuaryMap`, `rendererHeightMap` and shared review fixtures isolate authored data
from campaign orchestration. The renderer fixture is available through the app and
headless scenario codec. Checkpoints serialize quantized surfaces/decorations plus
canonical fingerprints for all locations; restore rechecks connected traversal,
key order and guardian access before publication. GPU resources and transient
interpolation/opening presentation are not persisted.

The user explicitly requested smaller, resource-bounded development verification.
[Verification workflow](verification_workflow.md) supersedes the original repeated
four-configuration/benchmark recipe. Smoke tests, retained extended tests and
resource limits preserve meaningful coverage without making every edit expensive.
See [verification and limitations](r05_verification.md).

# R02 — Production world rendering and assets

Status: **Implemented; Ready for review** (2026-10-31).
User approved the detailed plan with “Implement the plan.”
Parent: [renderer roadmap](renderer_plan.md). R01 and R-A are accepted.
[Verification and handoff](r02_verification.md); R02 acceptance pending.

## Boundaries and appearance

Production static geometry, prepared textures, camera/shaders, bounded GPU owners,
reusable targets and a development preview using Presenter are this slice. Normal
campaign/arena play retains the CPU renderer. Full billboard transforms, picking,
visibility and windowless visual migration are R03; doors/effects and complete
campaign integration are R04. Heights/outdoors/decorations are R05; stacked cells
remain deferred. Campaign stays paused after P-B; additional monsters follow rendering.

Preserve 500×282 at logical (4,18), 640×360 layout and 1920×1080 presentation.
Supersampling defaults on: 1000×564, explicit four-sample box resolve to 500×282,
nearest enlargement. Off means native filtered, never raw nearest. Both use nearest
magnification, nearest-texel/interpolated-mip minification and supported world
anisotropy up to 4×. Keep 84° horizontal FOV, horizon 34%, eye .5, setback .4,
near/far .025/16. Camera smoothstep translation/shortest-quarter-turn yaw derives
from authoritative MovementState without changing gameplay clocks or movement.
Retain R01 face factors, linear camera-depth attenuation and blue-grey fog.

## Modules, data and interfaces

Graphics-independent world camera/geometry lives under render/. The probe delegates
camera calculations to it while keeping its isolated fixtures. An immutable scene
snapshot contains bounded effective solidity and per-cell wall/floor/ceiling material
references, independent of simulation/save schemas. Frozen fixture blockers may be
represented as static volumes in the preview; no production door behavior is added.

Preparation partitions into 8×8 chunks, emitting open-cell floors/ceilings and walls
against solid neighbors (outside is solid). Count first, reject overflow, allocate
contiguous 32-byte position/normal/UV vertices and uint32 indices, then fill ordered
chunk/material ranges. World-aligned mirrored UVs preserve seams. Keep CPU geometry
for future visibility work. One VAO/VBO/EBO per scene avoids per-batch GPU ownership;
a platform-local uint32 glDrawElements adapter is needed because rlgl's indexed
helper assumes uint16. No generic rendering backend is introduced.

Cull current interpolated-camera chunk bounds, not transition endpoints. A 24-cell
horizontal radius bounds candidates to 7×7 chunks, then frustum-test AABBs. Fixed
scratch accommodates 49×32 material ranges; submit material-first. No frame rebuild,
mesh upload, source decoding or allocation. Cache by scene/asset identity, actual
camera pose and quality (plus diagnostic frame identity if used), not elapsed wall time.

An application-owned GpuResources registry is shared by Presenter and the new world
owner. Add typed generation-checked mesh/shader handles and bounded resource counters.
Raw descriptors stay in platform calls. Presenter retains its CPU-frame entry and
adds a typed logical-target/revision entry, sharing all native UI/composition/capture
code. No readback/re-upload bridge. Validate dimensions and handles. Restore GL/rlgl
framebuffer, viewport, matrices, shader, texture, depth, culling and blending before UI.

The minimal --development --world-preview launch reuses movement controls and scripts,
production authored/generated flat scenes and Presenter. Quality is selectable;
capture is hidden and scripted. No gameplay interactions or legacy picking over GPU
pixels. Existing normal launches do not load new world assets.

## Assets

A separate world format/revision and bake target pack reusable material and sprite
textures rather than perspective banks. Keep legacy publication unchanged. Generate
full-resolution registered NPC frames from source sheets, including missing derived
frames; retain dimensions, anchors, clip widths/timings, directions and attachments.
The new path does not consume profile-C half-resolution legacy derivatives.

Apply source cutoff, premultiply, then deterministic area-box mip generation including
odd dimensions and 1-pixel axes. Preserve base alpha separately for R03; no interaction
threshold/raster policy is decided here. Independent mipmapped textures avoid atlas
bleed; pack payloads into a binary publication. World wrapping mirrors; sprites clamp.
World surfaces, current actor frames, loot bag and six fixture markers are included.

Bounded metadata and payloads include format/appearance version, fingerprints,
references, lengths and checksum. Validate before GPU upload; reject missing/corrupt,
incompatible/interrupted/over-budget input with a rebuild instruction. Temporary
outputs and manifest-last publication prevent partial output appearing complete.
Runtime never decodes source PNGs or bakes. Explicit CMake source dependencies retain
no-op/incremental and multi-config behavior. Source images are offline-only ownership.
Current registrations reference 100 frames, calculated 149.34 MiB full RGBA mip data;
this is not a measured GPU-residency figure.

## Ownership, budgets and failure

Shared assets/shaders outlive active/staged views. Each scene owns CPU geometry, mesh
and its logical color/depth target. One shared supersampling target is scratch only.
Preparation is synchronous on the context thread. At most one active and one staged
scene exist. Validate/count/reserve, build, upload and render the staged first image
before nonallocating publication. Staging never overwrites the active logical image.
Any failure rolls back only staged resources, retaining pose/image/assets. Reject a
second stage until commit/cancel. Destroy old view before old borrowed session.

R02 tests this lifecycle without wiring campaign travel. R04 must preserve the existing
prepareTravel / prepare view / commitTravel / swap / old-view-before-old-session order.
No inactive GPU scenes, streaming, background uploads or asset hot reload.

| Resource | Hard limit |
| --- | ---: |
| Map dimensions / chunks per scene | 1024×1024 / 16,384 |
| Quads / material ranges per scene | 262,144 / 65,536 |
| Materials / unique sprite images | 32 / 448 |
| Shared texture payload including mips | 256 MiB |
| Base alpha / metadata | 64 MiB / 4 MiB |
| CPU scene / GPU mesh per scene | 64 MiB / 40 MiB |
| Active + staged scenes | 2 |
| Runtime preparation scratch | 128 MiB |
| Offline working storage | 512 MiB |
| Registry textures / targets / meshes / shaders | 512 / 8 / 8 / 8 |

Geometry cap implies 38 MiB vertex/index payload per scene. Two logical color/depth
and one supersampled color/depth target estimate 6.46 MiB. New subsystem ceilings:
352 MiB estimated GPU and 352 MiB tracked CPU peak including two scenes and scratch.
Valid maps may exceed renderer limits: reject, never truncate. Account capacities,
not only sizes. Existing UI/fonts, legacy banks, captures, raylib/driver overhead and
process RSS are reported separately, including actual combined peaks. These are not
whole-process memory guarantees. Quality switches reuse all targets, preserve pose
and logical coordinates, invalidate once, and never silently degrade on allocation
failure. Slot retirement and owner identity preserve stale/foreign-handle rejection.

## Validation and delivery

Implement CPU camera/mesh/culling, publication/bake, GPU resources/rendering, Presenter
and staged preview, then evidence. Tests own generated small maps/art. Routine CPU
coverage: faces/winding/material seams/bounds, .001-pixel projection, clipping,
transition culling oracle, capacity rejection, malformed publication and mip data,
partial failure cleanup, stale handles, cache invalidation and warmed C++ allocations.
CPU geometry/publication readers must build without raylib/window. Full-capacity stress
is separate. Hidden GPU tests cover orientation/fog/depth, nonuniform resolve, native
UI state, target reuse, staged failure/retry, teardown/recreation and GL errors.
No OS input automation or window selection; human review remains the user's task.

Run relevant Debug/Release rendering/resource/allocation/options/campaign/save suites,
headless checks and CPU ASan/UBSan. Test clean/no-op/changed-source/missing-output and
multi-config builds with isolated generated inputs. Capture compact integration and
failure evidence rather than duplicating R01's appearance review.

Benchmark representative flat scenes and separate stress workloads, 120 warmup/600
samples/three trials in fresh processes. Report CPU submission separately from glFinish
synchronized completion, p50/p95/p99, draws/uploads/cache hits, preparation phases,
managed capacities/GPU estimates/RSS. No timed readback/export. R01 moving p95 native
.0403 ms / supersampled .0722 ms is small one-actor evidence, not production frame
cost or GPU timer-query timing; legacy preparation was not production startup.
2 ms moving-world p95 on Ryzen 9 7900/RTX 3060 is an investigation target, not a noisy
CTest gate. Eventual total-frame target is 16.67 ms. Explain misses before review.

Record exact executed checks and limitations in r02_verification.md. Mark Ready for
review only after validation; user acceptance alone closes R02. Do not advance R03.

## Implemented interfaces and evidence

`world_render::prepare(GeometryView, Surfaces, overrides, Limits)` snapshots effective
geometry into immutable owned vertex/index/chunk/range arrays; it retains no borrowed
map/fixture pointers after preparation. The material grid is a preparation input,
not a new persisted world schema. `camera`, `project` and `cull` remain windowless.
The probe delegates its camera/projection helpers to this implementation.

`world_assets::Writer` publishes `textures.bin`, bounded `metadata.json` and source
provenance. `load` validates the whole publication; each streamed texture/alpha read
rechecks its checksum to reject a changed payload. Full registered frames are rebuilt
from sheets in offline temporary storage. Retained alpha arrays and typed clip/anchor/
attachment metadata need no source images or graphics context. The manifest text cap
is 256 KiB within the 4 MiB retained metadata budget; nesting is bounded and duplicate
keys and noninteger counts/offsets reject. The CMake input inventory deliberately
changes on additions/removals, preserving incremental rebuild correctness.

`WorldRenderer::stage/commit/cancel` owns at most two scene views, shares one asset set
and high-resolution scratch target, and validates the completed initial draw before
publication. `render` returns a generation-checked target plus image revision. A bool
selects the two quality modes. `GpuResources` exposes typed leases, live byte/count
estimates and cumulative resource-upload counters; diagnostic failure injection is
restricted to preparation acquisitions. Shared/scene byte validation precedes upload.
Presenter borrows the application registry and accepts CPU frames or GPU world images
through common native composition. Existing UI resource lifetime still ends with its
application registry; world leases release staged/retired resources earlier.

The preview intentionally omits a diagnostic actor: all current sprite frames are
prepared/uploaded and metadata/alpha are verified, but complete billboard drawing
and interaction remain R03. It provides both production authored and frozen generated
geometry, two quality modes, hidden capture and fresh-process benchmark options.
The existing legacy capture recipe still works. No campaign/save/replay schema change.

See the verification record for exact tests, 81 deterministic world captures,
capacity/failure evidence and timing limitations. The 352 MiB ceilings cover this
subsystem, not UI/fonts, legacy banks, captures, allocator or driver overhead.

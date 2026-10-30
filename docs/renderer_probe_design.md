# R01: Visual and motion proof

Status: **Implemented; Ready for review** (2026-10-31).
[Verification and handoff](r01_verification.md); user acceptance pending.
User approved the detailed conversational plan with “Implement the plan.”
Parent: [renderer roadmap](renderer_plan.md). No production cutover in this slice.

## Responsibilities and interfaces

An explicitly built `blobber_renderer_probe` provides a manual viewer, hidden
`--capture DIR` and hidden `--benchmark DIR` modes. `--art generated|shipped`
defaults to self-owned generated images; `--assets DIR` selects shipped sources.
CPU scene/camera values and mesh extraction live separately from GPU ownership,
asset decoding, legacy comparison, and export/orchestration. Normal game APIs,
saves and renderer remain unchanged. CPU geometry tests build without raylib.

Two bounded scenes provide a flat legacy-comparable room/corridor/pillar and a
sky/covered-passage specimen with a slab door and wall decoration. One idle actor
uses existing metadata dimensions, anchors, directions and timings. Generated
art is used for correctness; existing art is used for appearance review only.

Projection retains 500×282, 84° horizontal FOV, horizon 34%, eye height .5 and
setback .4. Near/far are .025/16. Camera translations use smoothstep; yaw turns
through the shortest quarter-turn with setback derived continuously from yaw.
The authoritative movement controller remains discrete. Geometry emits exposed
faces at preparation, drawing the whole small fixture. Existing mirrored textures,
face factors and camera-depth attenuation/fog are the reference. Wall face shading
interpolates .78–1 by normal/camera alignment. Sky responds to yaw, not translation.

Three profiles: nearest/no-mip control; native with nearest magnification and
nearest texels interpolated between mip levels (up to 4× world anisotropy); and
1000×564 rendering with explicit four-sample box resolve to 500×282. Final scaling
is nearest. Supersampling is the default following user review (2026-10-31);
native filtered remains selectable. Raw nearest remains a diagnostic control.
No MSAA/TAA/jitter/bob.
Sprite preparation applies source cutoff and premultiplies before mip generation;
transparent borders clamp. A single actor avoids claiming production transparency
ordering/picking coverage. All GL state is restored before 2D presentation.

## Ownership and limits

Scene arrays and images are owned CPU values; GPU owners are move-disabled RAII
objects destroyed before context shutdown. Prototype maxima: 32×32 cells, 8192
static quads, 32 textures, four targets. Index ranges/counts are checked before
upload; exhaustion throws during preparation, never drops geometry. No resource
recycling or long-lived borrowed GPU pointers. Meshes, draw scratch, frames and
textures are reused without engine-owned steady-frame allocations. Filesystem,
source decoding, captures and report generation are preparation/diagnostic work.

The GPU owner retains two render targets; the viewer/export owner adds one native
presentation target. Benchmark profiles run separately and do not allocate that
presentation target. GPU programs, uniform locations and mesh buffers are prepared
once; ordinary draws borrow those owned objects.

The legacy adapter prepares the existing CPU renderer for the same flat fixtures
and uses the same movement/animation snapshots. It is lazy in the viewer and absent
from GPU-only benchmark processes. It is not extended for outdoor/door specimens.

## Controls and evidence

W/S forward/back; A/D strafe; Q/E turn. 1–3 quality, Tab scene, Space scripted
play/pause, R reset, F legacy comparison (flat only), O cosmetic door, Esc exit.
T toggles manual movement animation. Switching quality retains pose.
Scripted captures advance fixed 10 ms ticks, independent of wall time. Export PNG
sequences, selected native captures, contact sheets, a synchronized local gallery,
settings/asset/driver provenance and measurements. Never inject OS input or focus
windows. Only the user launches manual review.

Benchmarks use separate processes per backend/profile, 120 warmup frames and 600
samples repeated three times for cached idle, moving, and animated idle. Disable
vsync/frame limiting. Report CPU submission and separate synchronized completion
(including overhead), p50/p95/p99, preparation phases, managed storage, estimated
GPU bytes and process peak RSS. No export/readback in timed frames; no unsupported
claim of hardware GPU execution timing or production-scale performance.

## Validation and acceptance

Windowless CPU tests cover projection to .001 pixels, movement/turn endpoints,
near-plane clearance, mesh topology/winding/count failures, billboard anchors and
directions, shading values and warmed allocation invariants. Generated hidden GPU
checks cover depth/holes, orientation, resolve, enlargement, 2D state restoration,
resource cleanup and deterministic repeat captures with defined tolerances.
Run focused Debug/Release, graphics-disabled tests and relevant existing rendering
checks; report unavailable GPU checks explicitly. Inspect motion as well as stills.

Handoff: `r01_verification.md`, retained review gallery, launch commands, measurements,
known differences and filtering recommendation. Mark Ready for review only after
checks, never self-accept. R02, new monsters, PC07, stacked cells, gameplay picking,
full effects, production asset packaging and height navigation are out of scope.

## Implemented evidence and limits

The probe is split into `scene`, `assets`, `gpu`, `legacy` and `review` modules;
`tools/renderer_probe.cpp` is orchestration only. `billboardCenter` makes foot-anchor
math CPU-testable. Legacy wrapper revisions remain coherent after manual/script
resets. Graphics-disabled builds include only the CPU probe tests/library.

Capture uses twelve 800 ms shots, retaining every 10 ms through each transition
plus selected settled frames. The compact retention helper uses existing ffmpeg,
retains losslessly recompressed PNG reference sheets and creates explicitly labelled
compressed GBR motion previews; the raw gallery is exact PNG. Quality modes preserve
pose; Space pauses/resumes scripts, and manual movement exits the script.

Benchmarks separate source decode, mesh build, combined renderer preparation/upload,
CPU submission and synchronized completion. Active movement samples all six movement
kinds without settled holds. The legacy constructor combines preparation operations;
no false per-phase timing or production startup improvement is claimed. CPU storage
and GPU resource values are estimates, supplemented by process peak RSS where the
platform supports it. No production Presenter/resource-registry changes were needed.

The source-art billboard deliberately uses world-space placement; the legacy
near-actor lateral fitting remains an explicit R03 comparison. The door is cosmetic
and can be crossed manually. Full color/ID parity, variable heights, scene streaming,
production effects and campaign integration remain out of scope. Tests intercept
ordinary/aligned C++ allocation, not C allocation or driver internals.

## User quality review (2026-10-31)

The user prefers button 3 for preserving distant detail before reduction and reports
both buttons 2 and 3 look good without flickering. Keep both: supersampling on by
default, native filtered when off. The probe now starts in button-3 mode; existing
1/2/3 keys and explicit `--profile` overrides remain. Production quality settings
belong to R02, without changing the logical image/picking coordinate system.
This supersedes the earlier native-filtered recommendation, not the test results.
R01 remains Ready for review pending explicit whole-slice acceptance.

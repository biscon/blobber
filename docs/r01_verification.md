# R01 verification and review handoff

Status: **Accepted and closed; milestone R-A Accepted** (2026-10-31). User explicitly accepted R01 and R-A.
Approved with “Implement the plan.” [Design](renderer_probe_design.md) ·
[Roadmap](renderer_plan.md) · [Retained motion gallery](screenshots/r01/index.html).

## Result and review

The standalone `blobber_renderer_probe` provides manual grid navigation and hidden
scripted captures/benchmarks. The normal game, prepared banks, campaign saves and
production rendering are unchanged. Campaign work remains paused after P-B.

```sh
cmake --preset release
cmake --build --preset release --target blobber_renderer_probe -j 4
./cmake-build-release/blobber_renderer_probe --art shipped
```

W/S move; A/D strafe; Q/E turn. 1/2/3 select nearest/native filtered/supersampled.
Tab changes between flat comparison and feature specimen. F toggles legacy on the
flat scene. Space starts the script, then pauses/resumes it; movement exits the
script. R resets, O opens/closes the cosmetic specimen door, T toggles manual
movement animation, Esc exits. Focus loss freezes progression. Script timestamps
and quality changes preserve the displayed pose. There is no mouse-look.

The review gallery has seven synchronized 9.6-second series: legacy plus all three
GPU profiles on the flat fixture, and three GPU profiles on the feature specimen.
Use quarter speed and the 10 ms frame controls to inspect transitions. Linked
native PNGs and five-frame contact sheets preserve exact rendered pixels. Retained
WebM videos are **compressed VP9 GBR CRF 18 motion previews**, not pixel-exact
references. Their 500×282 dimensions and duration were checked with ffprobe.
Twenty-eight retained PNGs were losslessly recompressed and decoded RGBA hashes
were checked before replacement. Raw exact PNG sequences and the original gallery
remain in `/tmp/blobber-r01-shipped`; a lossless video gallery is available at
`/tmp/blobber-r01-lossless-gallery`. These temporary paths are diagnostic evidence,
not dependencies of builds/tests or the retained gallery.

**User review update (2026-10-31):** the user tried the demo, prefers supersampling
(button 3) for distant detail, and reports buttons 2 and 3 both look good without
flickering. Supersampling is now the probe default and the selected production
default; retain native filtered as the lower-cost Supersampling off option. The
original native-filtered recommendation is superseded. Raw nearest stays diagnostic.
The three existing rendering implementations and recorded measurements are unchanged;
only the startup selection changed. The Release probe rebuild and whitespace checks
pass; the rendering algorithms were not changed or re-benchmarked. Existing explicit
`--profile` values still work.
User feedback establishes this preference and reported motion quality; it does not
claim exhaustive manual coverage or whole-slice acceptance.

The specimen demonstrates a depth-tested hinged slab/frame, a wall painting and
sky over a bounded courtyard. The courtyard shot steps out from under the canopy.
The sky and painting are generated specimens; lighting remains the dungeon baseline,
not R05 daylight. Frame textures are prototype quality. No clipping was found in the
sampled near-wall/corner camera paths; the CPU clearance checks cover these paths
at finer intervals than the exported transition frames.

## Executed checks

All automated GPU work used application-owned **hidden windows**, with no OS input,
window selection or desktop control. GPU: NVIDIA GeForce RTX 3060, OpenGL 3.3,
NVIDIA 615.78.08. CPU: AMD Ryzen 9 7900, Linux x86-64. Existing pinned raylib 6.2;
no system packages or toolchains were changed.

| Check | Result |
| --- | --- |
| Debug and Release probe build | Pass; no project warnings in final build logs |
| Focused existing + new graphics suites: `core`, `render`, `resource`, `frame_allocations`, `npc_animation`, `probe_cpu` | 6/6 Debug and 6/6 Release |
| Headless Debug and Release: `core`, `automation_allocations`, `probe_cpu` | 3/3 each; no raylib/window needed |
| Final probe CPU checks in all four presets | Pass |
| Explicit generated GPU integration executable, Debug and Release | Pass |
| Focused ASan/UBSan CPU probe executable | Pass |
| Warmed CPU script/camera and GPU orchestration C++ allocation guards | Zero tracked allocations |
| CLI help, invalid art/profile, missing value, conflicting output modes, unknown option | Six cases pass without opening a window |
| Generated-art and shipped-art scripted captures | Complete; appearance samples inspected |
| Fresh-process Release benchmarks, four profiles | Complete, three trials per workload and timing mode |
| Retained gallery references, seven video dimensions/durations, 28 PNG decoded hashes; raw/retained gallery JS syntax | Pass |
| Human review | User reports trying the demo; prefers 3 and finds 2/3 free of flicker. Detailed navigation coverage and other GPUs remain unverified |

CPU tests check exposed faces/winding, shared-edge removal, outdoor ceiling omission,
map bounds, cardinal projection to .001 logical pixels, smoothstep/turn clearance,
opposite strafe midpoints, paused and animation-off states, noncentral billboard
anchors in all headings, fog and face shading. The allocation guard exercises the
entire fixed-tick script without a window.

GPU fixtures check the shader against an independent uniform-wall fog oracle,
texture/target orientation, a known transparent body hole, full wall occlusion,
front/back source selection, unchanged-view caching, supersampling's uniform-region
resolve, subsequent 2D state, door changes, sky visibility, repeat resource recreation,
a rejected oversized mesh after partial initialization, and absence of GL errors.
Two independent resource lifetimes produce identical reference images on this device.
Warmed forward/back/strafe/turn rendering across all quality profiles performs zero
tracked C++ allocations. These guards do not intercept C `malloc` or establish
allocation behavior inside graphics drivers or third-party internals.

Commands used (configure/build before the relevant executable):

```sh
ctest --preset debug -R '^(probe_cpu|core|render|frame_allocations|resource|npc_animation)$'
ctest --preset release -R '^(probe_cpu|core|render|frame_allocations|resource|npc_animation)$'
ctest --preset headless-debug -R '^(probe_cpu|core|automation_allocations)$'
ctest --preset headless-release -R '^(probe_cpu|core|automation_allocations)$'
cmake --build --preset debug --target blobber_probe_gpu_tests
./cmake-build-debug/blobber_probe_gpu_tests
cmake --build --preset release --target blobber_probe_gpu_tests
./cmake-build-release/blobber_probe_gpu_tests
./cmake-build-release/blobber_renderer_probe --capture /tmp/blobber-r01-shipped --art shipped
python3 tools/retain_probe_review.py /tmp/blobber-r01-shipped docs/screenshots/r01
```

Output directories must be empty. For a rerun choose new directories. The generated
capture used `/tmp/blobber-r01-first`; its initial courtyard endpoint stayed under
the canopy and was corrected before the shipped review capture. The sanitizer build
is `/tmp/blobber-r01-asan`, configured with graphics OFF and
`-fsanitize=address,undefined -fno-omit-frame-pointer` for C++, plus sanitizer linker
flags. Build/run `blobber_probe_cpu_tests`. Logs are `/tmp/r01-*.log`; retained benchmark
JSON is linked below. Later focused reruns cover final anchor/allocation tests;
passing historical suite counts are not a claim that the entire project suite ran.

## Measurements and limits

[Legacy report](screenshots/r01/benchmarks/legacy.json) ·
[Nearest report](screenshots/r01/benchmarks/nearest.json) ·
[Native report](screenshots/r01/benchmarks/native.json) ·
[Supersampled report](screenshots/r01/benchmarks/supersampled.json).

Each profile ran in a fresh process, with vsync/frame limiting disabled. Each of
cached-idle, animated-idle and active movement received 120 warmup frames followed
by 600 measured frames, repeated three times, both with CPU submission timing and
with synchronized completion (`glFinish`, including synchronization overhead).
Active movement cycles forward/back/strafe/turn samples without settled holds.
Neither exports nor readbacks occur in timed frames. Capture encoding and builds
were completed before the retained benchmark run.

The table reports the median of the three trials' synchronized moving percentiles,
not direct GPU timer-query execution or complete game-frame time:

| Profile | p50 ms | p95 ms | p99 ms | Managed CPU estimate MiB | GPU resource estimate MiB | Process peak RSS MiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Legacy | 0.2183 | 0.5038 | 0.7401 | 221.58 | 0.54 | 322.28 |
| Raw nearest | 0.0366 | 0.0471 | 0.3245 | 7.70 | 15.32 | 107.05 |
| Native filtered | 0.0348 | 0.0403 | 0.1959 | 7.70 | 15.32 | 107.49 |
| Supersampled | 0.0670 | 0.0722 | 0.3297 | 7.70 | 15.32 | 119.97 |

All GPU profiles preallocate both logical and supersampled targets, so their managed
GPU resource estimates match. Estimates include mesh arrays/uploads and texture mip
levels, but exclude driver-internal accounting. Source image ownership is retained in
this prototype for comparison. Cached idle reuses the rendered image; animated idle
invalidates only when the two-frame idle clip changes. Moving runs have no cache hits.

This fixture has one actor and no production picking/effects/UI cost. The legacy
probe prepares its perspective banks from source on startup, whereas production
loads offline-prepared banks; **the recorded preparation time is not a production
startup comparison**. Reports separate source loading, mesh construction and combined
renderer preparation/upload, rather than pretending the legacy constructor exposes
independent upload instrumentation. Native filtered's smaller memory footprint and
lower world-render cost support proceeding, but R02/R03 must validate production
budgets, chunking and CPU visibility cost. No production-scale benchmark or balance
experiment was run.

## Final design details and next-agent handoff

`src/probe/scene.*` owns graphics-independent geometry/camera/script logic.
`assets.*` decodes only required idle frames and creates generated fixtures;
`gpu.*` owns resources/shaders and cached color rendering; `legacy.*` adapts the
unchanged CPU renderer with reset-safe revision ownership. `review.*` owns the
viewer, options, export and measurements. The executable is an explicit CMake target;
CPU tests are routine and GPU integration remains opt-in. No production Presenter
API or GPU registry capacity changed.

Resources are prepared once per scene, with fixed bounded arrays for GPU objects;
no recycling requires additional handle machinery in this isolated owner. The viewer
retains one GPU scene (two targets) and one presentation target. Scene changes
replace the GPU owner at an explicit preparation boundary. Ordinary shaders/uniform
locations are prepared once. Cosmetic door placement does not implement blocking,
locks, gameplay timing or interactions; manual movement can cross it. R04 owns that
integration. R01 does not implement close-range lateral actor fitting, body picking,
full animations, environmental schemas or staged campaign GPU travel.

R01 and milestone R-A are Accepted (2026-10-31). R02's
[design](renderer_world_design.md) has subsequently been implemented and is Ready
for review; see its [handoff](r02_verification.md). R01's gallery and measurements
remain unchanged. No new manual-coverage or other-device claims accompany acceptance.

## Acceptance (2026-10-31)

The user explicitly accepted R01 and milestone R-A. This supersedes acceptance-
pending handoff text above without changing evidence or coverage limitations.
R02 planning is complete and its [design](renderer_world_design.md) is approved
for implementation. Supersampling remains default; off means native filtered.

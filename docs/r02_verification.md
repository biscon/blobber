# R02 verification and review handoff

Status: **Implemented; Ready for review** (2026-10-31). User acceptance pending.
Approved with “Implement the plan.” [Design](renderer_world_design.md) ·
[Roadmap](renderer_plan.md) · [Selected evidence](screenshots/r02/README.md).
R01 and milestone R-A are Accepted. Campaign remains paused after P-B.

## Result and review

R02 adds production chunk/material geometry, shared off-centre camera math, world/fog
and resolve shaders, prepared reusable texture publications, bounded typed GPU owners,
active/staged scene replacement and a GPU-world entry into Presenter. Normal menu,
campaign, arena, saves and windowless visual gameplay retain the existing renderer.
No R03 interaction/visibility or R04 gameplay migration was performed.

```sh
cmake --preset release
cmake --build --preset release --target blobber -j 4
./cmake-build-release/blobber --development --world-preview
```

W/S forward/back, A/D strafe, Q/E turn, Tab authored/generated scene, Esc exit.
2 selects native filtered; 3 selects supersampling. Supersampling starts on, at
1000×564 resolved to 500×282 before nearest enlargement. `--supersampling off`
selects native filtered, never raw nearest. The window title reports the selection.
The preview has no actor/loot/fixture interaction; frozen blockers and empty party/UI
areas are intentional. Normal gameplay remains available through the usual launch.

The world publication contains 110 textures: three materials, 100 registered current
actor frames, a loot bag and six fixture markers. Full RGBA mip payload is 168,124,908
bytes; retained base alpha is 31,329,956 bytes. The baked format and appearance
revision are both 1. Legacy prepared banks, saves and replay identities are unchanged.
NPC canvases, pivots, clips/directions/timings, width overrides and attachments survive
publication. This slice prepares those assets, not their complete gameplay rendering.

## Executed checks

| Check | Result |
| --- | --- |
| Full graphics CTest, Debug / Release | 62/62 each |
| Full graphics-disabled CTest, Debug / Release | 43/43 each |
| Final focused graphics `world_cpu`, `options`, `probe_cpu`, `resource`, `frame_allocations` | 5/5 each |
| Final windowless `world_cpu`, `probe_cpu` | 2/2 each |
| CPU ASan/UBSan world + probe checks | Both pass |
| New hidden world GPU integration, Debug / Release | Pass, including final resource/upload counters |
| Existing hidden GPU smoke and R01 probe GPU checks, Debug / Release | Pass |
| Engine-owned ordinary/aligned C++ allocation guards | Zero during prepared camera/culling, movement, idle and quality switching |
| Shared preparation acquisition failures | All seven injected points roll back |
| Staged GPU/initial-render preparation failures | All five injected points retain the active image and resource counts |
| Staged CPU allocation failures | Five owned allocation points rejected; successful subsequent staging/cancel also checked |
| Independent resource lifetimes, stale/foreign handles, quality reuse, 2D state restoration | Pass |
| Near-capacity GPU/CPU stress | 262,086 quads; uint32 indices; active/staged owners fit budgets; next larger valid map rejects |
| Maximum map dimensions | Sparse 1024×1024 fixture prepares within the geometry budget |
| Generated publication/build fixtures | Clean/no-op/change/touch/missing/add/remove/failure pass for Ninja and Ninja Multi-Config Debug/Release, including paths with spaces |
| Hidden production-asset world captures | 81 PNGs repeat-identical, also matching preceding Debug captures |
| Presenter nearest enlargement | Pixel samples agree across all eight native UI captures |
| Existing legacy menu/campaign capture recipe | Completes, 69 PNGs; retained floor view inspected |
| Final whitespace/no-op build | Pass |

Full suites preceded the final focused resource/publication hardening; focused tests,
sanitizers and hidden GPU checks were rerun for the affected final changes. Existing
`commands_tests.cpp` aggregate-initializer warnings appeared in full builds; final
changed-target builds are warning-free. No packages or toolchains were changed.

CPU fixtures cover topology/winding, seam elimination, material overrides, capacity
rejection, projection, interpolated turn/strafe culling and malformed publications.
Payload checksum checks run both at publication validation and individual reads,
rejecting changed files before upload. Odd-size mip tests include every source texel;
alpha cutoff/premultiplication, masks and noncentral anchors are checked independently.

GPU fixtures compare shader output to an independent adjacent-wall depth/attenuation
oracle, verify asymmetric source orientation, and compare every resolved channel to
the four high-resolution source samples with a one-channel-unit rounding tolerance.
The resolve fixture explicitly requires nonuniform samples. Preparation rollback
covers buffers, targets and the completed initial draw before publication. C++ failure
injection covers the view owner and contiguous scene allocations. Frame transfer
counters remain unchanged through motion and quality switching.

Allocation guards intercept C++ `new`/`new[]`, including aligned forms. They do not
intercept C allocation or establish driver/third-party internal allocation behavior.
GPU work used application-owned hidden windows and in-process scripts exclusively;
there was no OS input, focus selection or desktop control. Human interactive review
and other GPU vendors remain unverified.

## Measurements and limits

Reference machine: AMD Ryzen 9 7900, NVIDIA GeForce RTX 3060, OpenGL 3.3,
NVIDIA 615.78.08, Linux x86-64, existing raylib 6.2. Release measurements use a seeded
36×36 production-generated floor and the complete current texture publication.
Six legal forward/back/strafe/turn movement sequences have no settled holds.

Each quality profile runs in a fresh process, vsync/frame limiting disabled, with
120 warmup frames and 600 measured samples for each of cached idle and motion,
three trials each. CPU submission and synchronized completion (`glFinish`, including
its overhead) are separate. No readback, capture encoding or asset upload occurs in
timed frames. Reports record transfer-counter deltas, draw calls and cache hits.

<!-- R02_METRICS_START -->
| Moving world, median of three trial percentiles | Native filtered | Supersampled |
| --- | ---: | ---: |
| CPU submission p50 (ms) | 0.007685 | 0.014598 |
| CPU submission p95 (ms) | 0.015949 | 0.025237 |
| CPU submission p99 (ms) | 0.025898 | 0.042410 |
| Synchronized completion p50 (ms) | 0.025258 | 0.053109 |
| Synchronized completion p95 (ms) | 0.029896 | 0.070772 |
| Synchronized completion p99 (ms) | 0.176440 | 0.659732 |
| Peak process RSS (MiB) | 149.80 | 163.91 |
| Measured frame mesh/texture uploads | 0 | 0 |

Synchronized moving p95 ranges across trials were 0.029726–0.031659 ms native
and 0.068147–0.228066 ms supersampled; p99 shows additional scheduling/driver
variability. Cached idle performs no world draws (600/600 cache hits per trial).
Moving trials issue 5,508 native / 6,108 supersampled draw calls per 600 samples.

Fresh-process publication reads took 248–334 ms and shared uploads 377–435 ms.
Scene mesh/upload/first-draw preparation took 1.95–2.01 ms. These single preparation
observations are CPU wall times including cold-start effects, not GPU timer queries.
<!-- R02_METRICS_END -->

[Native report](screenshots/r02/benchmarks/native.json) ·
[Supersampled report](screenshots/r02/benchmarks/supersampled.json).

Both profiles preallocate the supersampling target, so managed GPU estimates match.
One active scene retains about 30.02 MiB tracked CPU and 165.83 MiB estimated GPU
storage. Active plus staged scenes retain about 30.14 MiB CPU / 167.02 MiB GPU on
this floor. Texture and mesh estimates exclude driver overhead and compiled-program
storage. The separate stress fixture peaks at approximately 76.13 MiB tracked CPU /
82.44 MiB estimated GPU for two almost-capacity scenes using tiny generated textures.
This is below the proposed per-subsystem ceilings; it is not a whole-process bound.

Benchmark processes exclude Presenter/UI/font resources and legacy banks. The world
capture process exercises Presenter separately; the normal legacy path is tested in
its own process. R02 does not load legacy color banks alongside the world preview.
RSS reports include driver/runtime overhead but are not a GPU residency measurement.

The 2 ms moving-world p95 investigation target is met on this machine. These are
world-only costs without actors, CPU picking, effects, combat or native UI submission;
they do not establish the eventual 16.67 ms complete-frame budget. R01's smaller
one-actor fixture, different texture inventory and legacy bake-on-start adapter are
not like-for-like production startup or frame-performance baselines. No speedup claim
is made from those comparisons. No campaign balance experiments were run.

## Commands and next-agent handoff

```sh
ctest --preset debug
ctest --preset release
ctest --preset headless-debug
ctest --preset headless-release
cmake --build --preset release --target blobber_world_gpu_tests blobber_world_stress blobber_gpu_smoke blobber_probe_gpu_tests -j 4
./cmake-build-release/blobber_world_gpu_tests
./cmake-build-release/blobber_world_stress
python3 tests/world_build_tests.py cmake-build-release/blobber_bake_world cmake/WorldAssets.cmake
./cmake-build-release/blobber --development --capture-world /tmp/NEW-EMPTY-DIRECTORY
./cmake-build-release/blobber --development --benchmark-world /tmp/NEW-NATIVE-DIRECTORY --supersampling off
./cmake-build-release/blobber --development --benchmark-world /tmp/NEW-SS-DIRECTORY
```

Sanitizer build: `/tmp/blobber-r02-asan`, graphics disabled, Debug,
`-fsanitize=address,undefined -fno-omit-frame-pointer`, sanitizer linker flags.
Run `blobber_world_cpu_tests` and `blobber_probe_cpu_tests`. Diagnostic logs are
`/tmp/r02-*.log`. Full world captures are under `/tmp/blobber-r02-captures-release`
and `/tmp/blobber-r02-captures-repeat`; retained evidence has no temporary-path dependency.

User review/acceptance of R02 is next. Correct issues inside this slice, then wait for
explicit acceptance before R03 planning. Do not start picking/visibility migration,
production door/effect integration, new monsters, PC07 or stacked-cell work. No commit.

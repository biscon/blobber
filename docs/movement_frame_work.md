# Movement-frame work and uncapped frame budget

Status: implemented and mechanically verified (2026-11-04); interactive campaign
verification remains user-owned.

The final controlled profile exceeds 400 FPS in Debug and Release. Debug moving
mean is 1,370 FPS, p95 1.95 ms; Release is 1,994 FPS, p95 1.54 ms. See the workload,
limitations, complete results and reproduction commands below.

The user's acceptance target is >=400 FPS (<=2.5 ms/frame), uncapped and without
VSync, including representative moving gameplay and UI. Measure Debug and Release
separately; do not substitute cached idle speed or GPU-only time. Short bounded
profiles use low priority, one job, <=2 workers. No desktop input automation.

A four-creature generated doorway profile (8 warmup, 32 samples per mode, hidden
window, synchronized offscreen rendering) measured Debug moving CPU visibility
p50/p95 46.51/49.38 ms versus Release 1.59/1.82 ms. Debug submission plus GPU wait
was about .27 ms. This is subsystem evidence, not whole-game FPS. Full software
rasterization, pixel scans and composition are performed even when public picking
is deliberately unavailable during camera/door motion.

Keep presentation preparation every changed frame. Separate picking metadata reset
from raster-buffer clearing, skip software raster/target/loot scans during movement
in graphical and visual-automation callers, and rebuild once interaction settles.
Invalidate the opaque cache after skipped raster frames, even when the final pose
matches the last motion sample. Mark raw frame raster validity explicitly.
Standalone CPU diagnostics retain full-frame default sampling; GPU diagnostics can
request an explicit moving raster without publishing interaction or changing image
identity. No input/simulation/save/visibility semantics change.

Regression checks cover moving/settled handoff, final-pose cache validity, door
motion, no stale targeting, diagnostic on-demand parity, cached idle frames and
allocation-free updates. Measure the same bounded workload before/after, then the
full game update/HUD/presentation path with production artwork before claiming the
400 FPS goal. Record remaining bottlenecks honestly and continue until resolved.

The initial skip removes Debug moving visibility from 46.51 ms to .010 ms median,
but a full production-art/HUD/audio/presentation profile still measures 283 FPS
mean moving and 227 FPS stationary-animation. Settling software raster spikes
reach 53 ms. Stationary HUD also repeats leaf-query preparation for each badge pixel.
Cache per-frame leaf inverse axes plus per-actor byte relations (shared geometry,
not 256 full query arrays). This keeps memory bounded under the existing 8 MiB
budget and removes redundant badge work without changing visibility.

Split the numerical rasterization/composition/target-scan implementation into
`world_raster.cpp`. Optimize that source and leaf intersection arithmetic in Debug
using the existing compiler (-O2 for GCC/Clang); retain symbols, assertions and
unoptimized gameplay/orchestration. Provide an opt-out for stepping these numerical
kernels. This is explicitly a partially optimized Debug build, not a claim that
unoptimized software per-pixel rasterization meets the budget. Avoid loot pixel
scans when the snapshot contains no containers or visible loose-loot sprites.

The next whole-frame Debug profile reaches 943 FPS mean moving (p95 2.11 ms),
but stationary-animation remains 387 FPS: HUD outline rendering scans all 141,000
picking pixels even with an empty mark set. Retain the visible pixel bounds already
computed by picking, and expose a bounded union query for marked actors. Empty
marks yield an empty scan; otherwise inspect only their clipped visible union.
No highlight colour, silhouette coverage, picking identity or draw order changes.

Expose `--uncapped` in the normal application as an explicit diagnostic launch
option: omit VSync and set raylib's target to zero. The default remains VSync/60.
Use the same Window option in the profile, retaining hidden-window validation.
F1 already toggles the FPS display. Final verification uses 32 warmup + 256 measured
frames per case, including hover outlines and more moving-to-settled handoffs;
this remains an opt-in subsecond-to-few-second measurement, not a stress test.

## Ownership and validity

`VisibilityPipeline::update(input, rasterMoving)` keeps presentation preparation
separate from interaction rasterization. The default is full CPU diagnostic
coverage; `WorldRenderer` and visual automation explicitly pass false. A frame's
`rasterized` flag governs access to its raw pixel/depth arrays. `picking()` remains
null during camera/door motion. Metadata is cleared even when raster work is
skipped, preventing stale target/loot publication. The first settled frame rebuilds
opaque scenery after a skipped frame, including when easing already reached the
exact final camera pose. Repeated unchanged settled frames reuse the result.

`rasterForDiagnostics()` explicitly rebuilds a missing raw raster without changing
presentation revision or publishing moving interaction. Diagnostic GPU ID readback
uses this path. `world_raster.cpp` owns numerical coverage, composition and target
scans; `world_visibility.cpp` owns validation, invalidation and frame preparation.

Each immutable published frame owns one bounded leaf geometry array (32 entries),
256×32 byte owner relations, and its non-owning badge callback context. Each picking
buffer retains 256 visible pixel bounds calculated by the existing target scan.
`markedBounds()` unions only marked visible actors; HUD scans that half-open range.
No per-frame allocation or growing cache is added. Existing preparation enforces
the 8 MiB visibility/effects scratch limit; capacity failures leave the current
view intact. The active/staged GPU lifecycle remains unchanged by this follow-up.

## Final measurements and verification

Ryzen 9 7900, RTX 3060, OpenGL 3.3 NVIDIA 615.78.08. Profile uses a hidden
1920×1080 window, normal supersampling, VSync off, no FPS cap, 32 warmup and 256
measured frames per case. Production artwork/HUD/fonts, four animated goblins,
an open door and owned 9×9 map; no saves are loaded. Timing includes game update,
two normal view/command refreshes, muted audio update, full presentation and GPU
completion (`glFinish`). Roaming AI and music playback are disabled to keep the
workload stable. Simulation advances 1/120 second per frame independently of
render speed; 243 moving and 13 settled frames exercise repeated handoffs.

| Build / case | Mean FPS | p50 ms | p95 ms | Maximum ms |
| --- | ---: | ---: | ---: | ---: |
| Debug animated idle | 1,055 | .970 | 1.918 | 3.909 |
| Debug hover outline | 1,376 | .678 | 1.238 | 3.186 |
| Debug movement | 1,370 | .647 | 1.951 | 2.600 |
| Release animated idle | 2,922 | .299 | .800 | 1.656 |
| Release hover outline | 2,494 | .356 | .864 | 2.072 |
| Release movement | 1,994 | .420 | 1.539 | 2.103 |

Raw stage timings: [Debug](performance/movement-debug.txt),
[Release](performance/movement-release.txt). These are bounded diagnostic results,
not a minimum-FPS guarantee across campaign sizes, effects, AI or desktop loads.
Some Debug frames still exceed 2.5 ms; the reported means do not hide those tails.
Earlier 64-frame runs varied with GPU/presentation time (562–943 FPS moving after
the fixes), which is why the final check includes more frames and explicit maxima.
No desktop input/focus automation was used. Interactive review remains unverified.

Debug retains symbols/assertions and unoptimized gameplay/orchestration. Only
`world_raster.cpp` and `world_billboard_clip.cpp` use `-O2` in GCC/Clang Debug builds.
Configure `-DBLOBBER_OPTIMIZE_DEBUG_RASTER=OFF` for fully unoptimized stepping;
the reported Debug performance does not apply to that opt-out.

Reproduce sequentially (one intensive job at a time):

```sh
nice -n 15 ionice -c 3 cmake --build cmake-build-debug --target blobber_world_movement_profile -j 2
nice -n 15 ionice -c 3 cmake-build-debug/blobber_world_movement_profile cmake-build-debug/assets
```

The profile is `EXCLUDE_FROM_ALL`, outside ctest, and needs a graphics/audio session.
For ordinary play, launch `cmake-build-debug/blobber --uncapped` and press F1.
Normal launches retain VSync and the 60 FPS target.

Focused verification passed:

- Debug `options` and `world_visibility`, including moving-to-settled cache handoff,
  on-demand diagnostic parity, moving doors, outline bounds and frame allocations.
- Release `options`, `targeting`, `automation_allocations`, `sprite_crop`,
  `world_visibility`, `world_visual_cli`, `world_integration`, `world_environment`.
- Hidden Debug GPU visibility/color/lifecycle and integration/effects/travel checks;
  CPU/GPU visibility differences were zero and resource/allocation budgets passed.
- Debug and Release application builds; command-line help exposes `--uncapped`.

These checks target the changed raster/visibility/outline callers and native GL
integration. The full unrelated test matrix and capacity stress were not run.

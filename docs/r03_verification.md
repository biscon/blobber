# R03 verification and review handoff

Status: **Accepted and closed** (2026-10-31). The user explicitly accepted R03:
“slice accepted, record it in the plan doc”. Results and limitations are unchanged;
no additional interactive-review coverage is inferred.
Approved by “Implement the plan.” [Design](renderer_visibility_design.md) ·
[Roadmap](renderer_plan.md) · [Review gallery](screenshots/r03/index.html).
R01/R-A and R02 remain Accepted. R04, new monsters and PC07 are unstarted.

## Delivered behavior and interfaces

The new path draws current actor clips/directions, travel/reaction/prone/death states,
shadows, fitted formations, registered pivots and attachment points, loot and fixture
markers/proxies. A graphics-independent CPU triangle/alpha pass supplies logical
pixel depth/IDs, live targets, loot membership, ground projection and stable HUD
anchors. Status icons and new-path health bars clip against full pixel depth. Legacy
health-bar behavior stays unchanged. Movement still suspends interaction.

`world_assets::CpuAssets` retains immutable-publication metadata and base masks.
`world_render::VisibilityPipeline` double-buffers presentation/interaction results and
caches opaque geometry separately. `WorldRenderer` consumes the same billboards and
publishes matching image/picking data; Presenter accepts that picking publication.
Both scene owners share masks. Initial/staged preparation rolls back without replacing
active images/IDs, and view/session lifetimes remain explicit on success and exceptions.

The development `--world-preview` has F1 formation, F2 loot and F3 fixture scenes;
F5 is native filtered and F6 supersampled. Default quality remains supersampling.
The standalone preview intentionally has incomplete spell/projectile, campaign and
combat-result presentation; it does not advertise production campaign parity.
Normal launches keep the legacy renderer.

`blobber_headless --mode visual --visibility world` and Python `Client(...,
visibility="world")` select the new windowless path in graphics **and** graphics-disabled
builds. It requires prepared world masks/metadata but no source images, legacy color
banks, display or context. Default visual mode stays `legacy`. Local MCP sessions keep
their default; replay jobs forward a recorded backend. World replay image capture
returns `capture_unsupported` before creating a directory/window, pending R04.

Protocol/save schemas are unchanged. World publication format/appearance remain 1/1.
Strict traces record backend, `world-visibility-v1`, publication fingerprint, format
and appearance identity; mismatches reject before execution. Existing core command,
range, fixture-discovery and at-feet loot authority is preserved.

## Executed checks

| Check | Result |
| --- | --- |
| Full graphics CTest, Debug / Release | 64/64 each |
| Full graphics-disabled CTest, Debug / Release | 45/45 each |
| Final affected graphics rendering/HUD/visibility/protocol/allocation suites | 11/11 each |
| Final affected headless world/automation/fixture suites | 5/5 each |
| Final independent facing-sector regression | Pass, Debug / Release |
| Focused CPU ASan/UBSan world + visibility | Pass |
| Hidden R03 GPU coverage/lifecycle, Debug / Release | Pass |
| Existing hidden R02 GPU integration, Debug / Release | Pass |
| Initial interactive preparation acquisition failures | All seven points roll back, including new shared resources |
| Staged CPU preparation injection | 20 positions exercised; at least ten owned failures reached; successful stages cancelled |
| Prepared CPU/GPU submission C++ allocation guards | Zero ordinary/aligned C++ allocations |
| Motion/animation/quality mesh or texture resource uploads | Zero |
| Maximum candidate capacity | All 2,656 bodies/loot/fixtures retained, zero frame C++ allocations |
| Existing near-capacity world GPU stress | Pass |
| Windowless generated CLI/client, record/replay and changed-header/corrupt-publication failures | Pass without DISPLAY/WAYLAND_DISPLAY |
| Production-art hidden captures | 403 PNGs each in Debug/Release; all 403 byte-identical |
| Final changed-target builds / no-op graphics builds / whitespace | Pass; changed targets warning-free |

Full suites preceded final health-clipping, attachment/lifetime and diagnostic
hardening; affected suites, sanitizers and GPU checks were rerun. The final additional
facing-sector test and capture-label correction were checked separately. Final
preparation-time checks for missing future sprites/required clip directions were
also covered by the affected suites and sanitizers. No new passing
counts are inferred from historical R02 evidence. No packages or toolchains changed.

Generated fixtures cover alpha holes and partial edges, wall/body overlap, corpses,
clip/frame/directional changes, a noncentral pivot, subpixel travel, near-plane crossing,
loot generations, ground intersection, badges/health clipping, stale snapshots and map
replacement. Live and windowless results/command eligibility are compared using the
same snapshots. Reset/failure/replay checks use owned temporary publications with
intentionally absent source PNGs. Existing campaign/travel/save suites remain passing.

The hidden diagnostic GPU pass uses canonical logical centers and unfiltered base
masks, independently of filtered color resolution. CPU/GPU visible-object membership
matches exactly. Across the 23 compared snapshots there are **21 boundary pixel
differences**, all within one logical pixel of geometric edges; no interior differences.
Actual native/supersampled color captures were inspected separately, including close
formations and a sleeping rear actor's clipped health/status indicators. This is not
a cross-vendor exact-pixel guarantee.

Allocation guards intercept C++ new/new[] including aligned forms, not C malloc or
raylib/driver internals. Preparation, exports and protocol JSON remain outside the
normal frame guarantee. Captures use hidden application-owned windows and in-process
scripts/snapshots only. Human interactive review and other GPU vendors remain unverified.

## Measurements

Reference: AMD Ryzen 9 7900, NVIDIA RTX 3060, OpenGL 3.3, NVIDIA 615.78.08, Linux,
Release, current prepared artwork. Each quality runs in a fresh process, frame limiting
and vsync disabled, with 120 warmups and 600 measured samples for three trials.
Submission is CPU wall time; synchronized elapsed additionally calls glFinish and
includes its overhead. No timed readback/export/upload. Medians below are medians of
three trial percentiles; the full reports include p50/p95/p99 and counters.

The GPU interaction fixture is an isolated 11×11 room with five current-art actors.
Animation alternates registered idle frames every sample; motion covers six camera
actions with held simulation state. It is not an end-to-end combat/UI benchmark.

| p95, milliseconds | Native filtered | Supersampled |
| --- | ---: | ---: |
| Moving CPU visibility | 1.0128 | 1.0220 |
| Moving CPU GPU-submission portion | 0.0228 | 0.0324 |
| Moving unsynchronized total | 1.0302 | 1.0456 |
| Moving synchronized total | 1.3557 | 1.3618 |
| Stationary animated visibility | 0.2865 | 0.2849 |

Cached idle records 600/600 cache hits per trial and no draws. All measured trials
record zero mesh/texture resource uploads. Idle CPU checks are about 0.0001 ms total;
such tiny timings are diagnostic, not a reliable cross-system performance claim.

Separate windowless generated-floor fixtures use 32 and 256 actors, camera motion,
and tiny test-owned art: median p95 visibility **1.6392 ms / 1.7180 ms**, respectively.
These establish CPU workload evidence, not production-art GPU or full-capacity speed.
The distinct maximum-candidate stress checks correctness/allocation bounds, not a
frame-time guarantee. The 2 ms CPU visibility investigation target is met here; no
GPU readback or legacy color fallback was adopted.

Per-view presentation scratch is **4,827,808 bytes** (4.60 MiB), below the 8 MiB cap.
The current-art interaction fixture retains 34.54 MiB tracked CPU / 165.75 MiB estimated
GPU; active plus staged views retain 39.18 / 166.85 MiB. Fresh-process peak RSS is
155.47 MiB native / 164.77 MiB supersampled. These are separate accounting categories:
GPU estimates exclude driver/compiler overhead, RSS is not GPU residency, and the
benchmark excludes Presenter/UI/fonts and legacy banks. R02 aggregate limits remain
352 MiB per CPU/GPU subsystem; these are not whole-process ceilings.

[Native report](screenshots/r03/benchmarks/native.json) ·
[Supersampled report](screenshots/r03/benchmarks/supersampled.json) ·
[32-actor CPU](screenshots/r03/benchmarks/cpu-32.json) ·
[256-actor CPU](screenshots/r03/benchmarks/cpu-256.json).
The complete 16.67 ms gameplay-frame target remains R06 work.

## Commands and next action

```sh
cmake --build --preset release --target blobber blobber_world_visibility_gpu_tests blobber_world_visibility_stress blobber_world_visibility_benchmark -j 4
ctest --preset debug
ctest --preset release
ctest --preset headless-debug
ctest --preset headless-release
./cmake-build-release/blobber_world_visibility_gpu_tests
./cmake-build-release/blobber_world_visibility_stress
./cmake-build-release/blobber_world_visibility_benchmark
./cmake-build-release/blobber_world_visibility_benchmark --stress
./cmake-build-release/blobber --development --world-preview
./cmake-build-headless-release/blobber_headless --assets cmake-build-release/assets --mode visual --visibility world
```

Hidden capture/benchmark recipes are in the [gallery guide](screenshots/r03/README.md).
Final capture runs: `/tmp/blobber-r03-delivery-debug` and
`/tmp/blobber-r03-delivery-release`; retained evidence has no temporary-path dependency.
Sanitizers use `/tmp/blobber-r03-asan` with graphics disabled and
`-fsanitize=address,undefined -fno-omit-frame-pointer`. Diagnostic logs are `/tmp/r03-*.log`.

R03 is Accepted and closed. R04 is next eligible for separate planning when
requested; do not automatically implement it, retire legacy defaults, add monsters,
resume PC07, commit or automate desktop input.

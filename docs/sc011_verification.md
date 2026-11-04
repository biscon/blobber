# SC01.1 verification and handoff

Date: 2026-11-05. Status: **Accepted by the user on 2026-11-05.**
Scope: approved [location-owned asset design](sc011_design.md), including the
user's clarification that supersampling and fidelity stay unchanged and memory
reference lines are advisory. SC01.2 architecture has not started.

## Delivered behavior

- Catalog opening loads metadata only. Texture pixels, CPU alpha masks and mesh
  vertices are acquired for active/staged location requirements. Creature IDs stay
  global; only types in the prepared location's actor records acquire all their
  animation frames. Common interaction/effect artwork is shared.
- Move-only CPU leases and shared GPU texture leases preserve active/staged
  rollback. Commit releases source-only assets; cancel releases destination-only
  assets. Clearing the world retains zero location payloads. There is no unused
  biome cache or frame-time lazy loading.
- Pack source/settings deduplication reduces the current publication from 401 to
  387 textures. Format 5 stores bounded mesh descriptors with checked binary vertex
  ranges; runtime reads validate selected payloads, and the baker verifies all
  payloads. Installed catalog limits are separate from resident resource limits.
- Memory reporting includes registry-owned UI/world resources and separately owned
  font atlases; peak accounting also includes diagnostic capture targets. The
  384/448 MiB reference lines do not reject content or change quality. Existing
  480 resident-world-texture / 512 registry-slot, 512 MiB world-texture, 608 MiB
  tracked-world-GPU and CPU/scene safety capacities still reject explicitly.
- Supersampling remains ON by default. The 1000×564 pass, 500×282 logical output,
  1920×1080 native presentation, texture detail, mip/sampling policy and camera are
  unchanged. Explicit native-mode construction omits high-resolution scratch;
  after first enabling supersampling, its bounded scratch stays allocated to
  preserve allocation-free warmed quality switches.

## Production evidence

Hidden OpenGL review on NVIDIA GeForce RTX 3060, seed 5, supersampling ON. No OS
input, focus changes, external windows or user's save files were used. The review
constructs its own campaign and uses application APIs for the hub → floor one →
hub round trip. All screenshots are native 1920×1080.

| State | Resident world textures | World texture payload | Application-owned GPU total |
| --- | ---: | ---: | ---: |
| Hub | 25 | 46.63 MiB | 177.51 MiB |
| Floor one | 243 | 240.06 MiB | 371.49 MiB |
| Hub/floor transition | 243 | 240.06 MiB | 372.60 MiB |
| Returned hub | 25 | 46.63 MiB | 177.51 MiB |
| World cleared, UI and renderer scratch retained | 0 | 0 MiB | 129.78 MiB |

Installed texture mip data is 328.61 MiB, not permanently resident GPU allocation.
Floor-one CPU masks are 40.55 MiB; returning to the hub restores 6.49 MiB; clearing
releases all masks and models. The measured peak including the extra diagnostic
capture target is 387.31 MiB. These are application byte estimates, not total
physical driver VRAM measurements.

The review also computes dependency-union/geometry estimates for this seed's four
locations without uploading all locations. Largest ordinary adjacent-floor pairing
is floors one/two at 428.44 MiB. The deliberately conservative one/three pairing
is 461.57 MiB (there is no direct one-to-three travel route); it exceeds the advisory
448 MiB line and is recorded without reducing fidelity. These projections exclude
small subsequently prepared billboard mesh storage (~1 KiB); actual recorded
hub/floor totals include it. They do not establish every seed's maximum workload.

[Raw memory report](screenshots/sc011/memory.txt), [hub](screenshots/sc011/hub.png),
[floor one](screenshots/sc011/floor-one.png), and
[returned hub](screenshots/sc011/hub-returned.png). Floor and returned-hub captures
were visually inspected: expected wall/paving detail, sky, stairs, native text and
portraits remain present. Hub and returned-hub PNG files are byte-identical.
The user accepted the slice on 2026-11-05; no separate interactive desktop
verification was recorded.

## Executed checks

One Debug configuration, low CPU/I/O priority, no more than two build workers and
one test process; resource-intensive commands ran sequentially. Memory/scratch
space checked before builds. CPU suites run without opening a window.

- `world_residency`: owned disjoint packs/types, corpse requirements, metadata-only
  opening, mesh decoding, sharing, commit/cancel, invalid IDs, corrupt destination
  alpha/mesh rollback, zero payload retention after release, unused catalog growth,
  explicit resident-count exhaustion before I/O, allocation-free resident access.
- `world_cpu`, `underkeep_render`, `world_visibility`, `world_environment`:
  publication integrity, geometry/material behavior, furniture footprint checks,
  picking, bounds and warmed allocation invariants.
- `world_integration`, `automation_allocations`, `sprite_crop`, `targeting`,
  `visual_review`, `dungeon_refresh`, `campaign_save_smoke`: affected callers,
  windowless visual ownership, cropped sprites and campaign save/travel behavior.
- `world_visual_cli`: display-free CLI/client, strict trace replay and preparation
  failure/retry after corrupt required artwork. Startup intentionally does not
  reject a corrupt unused payload; acquisition rejects it without a fallback.
- `blobber_world_residency_gpu_tests`: hidden selective upload, shared reuse,
  source retention under injected acquisition failures, corrupt destination color,
  repeated commit/cancel/revisit, zero location residency on clear, shutdown,
  unchanged default supersampling and frame allocation/I/O/upload invariants.
- `blobber_world_gpu_tests`, `blobber_world_visibility_gpu_tests`,
  `blobber_world_integration_gpu_tests`: hidden sampling/color oracle, resource
  lifetimes, CPU/GPU visibility parity, doors/effects and campaign travel rollback.
  Visibility and integration runs reported zero differing pixels; the integration
  fixture exercised 32 injected CPU allocation failures with source retention.
- Offline `world_build_tests.py --single-config`: clean/no-op/change/touch/missing/
  add/remove/failure publication behavior with generated fixtures. Format 5 and
  reused wall/floor bindings verified. Production publication rebuilt and checked.
- Debug application, headless CLI, baker and review executable built. `git diff
  --check` clean at handoff.

The first GPU attempt could not access X11 inside the sandbox. Retrying with
approved display access succeeded; all GPU windows stayed hidden. Initial checks
exposed two obsolete eager-integrity assertions; they now test rejection when the
required payload is acquired and successful recovery after repair. The first
production review missed discovery after its diagnostic pose reset; adding the
normal discovery update fixed the review script. Final round trip passed.

No Release/sanitizer/stress/benchmark matrix or separate headless build was run.
No 512 MiB/1 GiB GPU was available for hardware verification. Driver allocations,
window-system buffers and application-independent GPU consumers are outside the
tracked totals. Performance/FPS claims are not made by this slice.

## Reproduction and next work

Build (the normal application target also rebuilds the prepared publication):

```sh
nice -n 15 ionice -c 3 cmake --build cmake-build-debug --target blobber blobber_review_world_residency blobber_world_residency_tests blobber_world_residency_gpu_tests -j 2
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-debug -R '^world_residency$' -j 1 --output-on-failure
nice -n 15 ionice -c 3 cmake-build-debug/blobber_world_residency_gpu_tests
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_world_residency cmake-build-debug/assets /path/to/new-review-directory
```

The last two commands need graphics access and create only hidden application-owned
windows. The review requires a new output directory. Normal play remains
`cmake-build-debug/blobber`; supersampling is still on without flags.

Implementation entry points: `assets/world_assets.*` (format/catalog),
`assets/world_residency.cpp` (requirements/CPU leases), `platform/world_renderer.*`
(view GPU leases/preflight/reports), `automation/world_visual.cpp` (CPU-only view
lease), and GPU registry/Presenter/UiFonts accounting. Small owned residency
fixtures live under tests/support; the production review is an opt-in tool.

Campaign schema/simulation remain 7/11; no save migration, deletion or schema bump.
World format is 5, appearance revision 4, visibility v13; strict traces retain
publication and binary identity checks. Rebuild other configurations normally
before running them against the new format; only Debug was built here.

Next eligible action: separately plan SC01.2 architecture and the finished street
corner when requested, using the new residency
interfaces and measured headroom. Do not automatically implement it.

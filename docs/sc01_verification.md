# SC01.2 verification and handoff

Date: 2026-11-05. Status: **Ready for review; user acceptance pending.**
Scope: the approved [architecture design](sc01_design.md). SC01.1 remains accepted;
normal campaign Sanctuary remains the courtyard. No SC02/PC08 work or hub cutover.

## Delivered foundation

WorldDefinition owns bounded static-building placements with stable pack-local IDs,
cardinal transforms and explicit solid footprints. Map preparation validates level,
disjoint reservations and suppresses only their generated backing walls. Movement
and sight still use authoritative solid terrain. A separate architecture mesh
profile preserves the tighter furniture checks and existing four furniture slots.

Prepared pack definitions support multipart models and shared atlases. CPU/GPU
residency uses SC01.1 leases, with rollback and release at preparation boundaries.
Bounds include all roofs and overhangs. The original planned 24-cell candidate
radius was insufficient for diagonal far-frustum corners plus four-cell overhangs;
building scenes now search 32 cells/81 chunks. Other scenes retain 24/49. Far
clipping remains 16 cells. Fixed draw storage covers both painted and terrain ranges.

Publication is format 6, appearance 4; campaign schema/simulation 8/11; generator/
templates remain 9/7; visibility is world-visibility-v14; world fingerprint revision
is 3. Unsupported old saves reject without modification or migration. Protocol v1
and terrain-rules-v2 remain unchanged. New static placement fields round-trip.

## Artwork and review

The dedicated sanctuary-city pack supplies a low house and a two-storey house,
finished paving, thick recessed arches, closed timber doors, inset shutters,
roof undersides and chimneys. The 16x16 authored L-corner has a two-cell turning
street and connected access around both shells. Doorway approaches and spawn are
centralized in the scene definition. --sanctuary-city-review runs ordinary gameplay
movement/HUD with a preset party, no NPCs, and no save/library initialization.

Original material art used the built-in ImageGen tool. The retained
[master](../assets/packs/sanctuary-city/source/material-master.png) and
[exact prompt](../assets/packs/sanctuary-city/prompts.json) are in the
[city pack](../assets/packs/sanctuary-city/README.md). Both houses share one 4096²
padded atlas at 256 texels/runtime cell, without reducing source detail. Low/tall
shells contain 821/1,419 triangles. The reproducible Blender script checks UV density
and exports packed Blender, unlit GLB and runtime mesh JSON sources.

The first captures exposed reversed doorway-reveal normals and fragmented plaster
UVs. These were corrected at source, alongside arch outer-edge closure and a
threshold that does not overlap generated paving. Shared continuous material fields
preserve vertical timber grain and avoid restarting plaster on every mesh fragment.

The [capture index](screenshots/sc01/README.md) links 38 native 1920x1080 images:
door approaches, rear/side faces, farther rooflines and seven movement actions
sampled at four transition positions. Selected captures were inspected for closed
door recesses, continuous facades, inset shutters, roof undersides and corner
closure. No duplicate backing faces or roof popping were observed in these views.
The two-storey silhouette fits the farther view; close views naturally crop the
roof above the fixed camera. The distant enclosing wall still exhibits the existing
16-cell far clipping/fog, visible in diagonal views. This is not a completed town
layout or a claim about every future street/camera combination.

Measured on an NVIDIA GeForce RTX 3060 with supersampling ON. The
[raw report](screenshots/sc01/report.txt) records byte counts and poses. These are
application-owned accounting figures, not driver memory or frame-rate benchmarks.

| Resource | Measured amount |
| --- | ---: |
| Resident world textures | 26; 131.96 MiB |
| Resident alpha masks | 6.49 MiB |
| Resident model payloads | 0.205 MiB |
| Scene CPU / GPU geometry | 0.396 / 0.396 MiB |
| Application GPU, including UI/fonts | 258.71 MiB |
| Capture peak, including temporary capture target | 274.53 MiB |
| World textures / models after view release | 0 / 0 |

The shared atlas preserves detail while avoiding duplicate atlas residency for the
two houses. No supersampling or fidelity reduction was used to meet a target.

## Executed checks

One Debug configuration, low CPU/I/O priority, no more than two build workers and
one test process. Heavy jobs ran sequentially; memory and scratch space checked.
Retained [test output](screenshots/sc01/checks.txt) records the results below.

- `architecture`: owned multipart meshes/publication, rotations, footprint overlap,
  missing backing, unlevel foundations, decoration conflicts, bad references/bounds,
  capacities, wall suppression, roof bounds, cardinal/diagonal far-frustum culling,
  allocation-free culling, shared leases, corrupt-part rollback and release.
- `options`, `campaign_save_smoke`, `dungeon_refresh`, `world_residency`, `world_cpu`,
  `underkeep_render`, `world_visibility`, `world_integration`, `world_environment`:
  all passed. Save smoke includes static placement fields and decode capacity.
- After the diagonal-culling/layout correction, `architecture`, `world_visibility`
  and `dungeon_refresh` passed again. Save/furniture suites were not redundantly rerun.
- Final review moved building-neighbor checks after decoration coordinate validation,
  avoiding arithmetic on malformed coordinates. The architecture regression passed
  again; valid-scene rendering was unaffected. C++ formatting and `git diff --check`
  also passed.
- New hidden `blobber_architecture_gpu_tests`: final depth/visibility, stage failure,
  cancel/revisit, allocation/read/upload and release checks passed with zero differing
  CPU/GPU ID pixels. Warmed frames performed no engine-owned allocations, asset reads
  or GPU uploads. The checks use small owned fixtures, independently of city artwork.
- `world_build_tests.py --single-config`: generated clean/no-op/change/touch/missing/
  add/remove/failure publication checks passed, including multipart architecture,
  atlas sharing, changed mesh dependencies and invalid-bound atomic rejection.
- Blender export validates triangle geometry and physical UV density. Debug
  application and review executable build; prepared production publication rebuilt.

Initial sandbox GPU capture could not open X11; approved display access allowed
hidden application-owned windows. No OS input, focus change or user save was used.
Blender's compressed library/optional GLTF export crashed; ordinary uncompressed
save and deterministic GLB export succeeded. An optional Blender thumbnail cache
write was denied; delivered assets still saved successfully. The first bake rejected
sources changing during preparation. Two final rebuilds were interrupted for the
culling/layout corrections; replacement builds completed. These interrupted attempts
are not counted as passing checks.

No Release/sanitizer/full persistence/stress/benchmark matrix or separate headless
build was run. No 512 MiB/1 GiB GPU was available. GPU byte figures exclude driver
and unrelated desktop allocations. Interactive visual acceptance remains user-owned.

## Reproduction

```sh
nice -n 15 ionice -c 3 cmake --build cmake-build-debug --target blobber blobber_review_sanctuary_city blobber_architecture_tests blobber_architecture_gpu_tests -j 2
cmake-build-debug/blobber --sanctuary-city-review
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-debug -R '^architecture$' -j 1 --output-on-failure
nice -n 15 ionice -c 3 cmake-build-debug/blobber_architecture_gpu_tests
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_sanctuary_city cmake-build-debug/assets /path/to/new-review-directory
```

The last two commands need graphics access and keep their windows hidden. The
interactive review uses normal game controls; leaving it exits the session.
Supersampling remains ON by default. Asset regeneration is documented in the pack.
Normal launch remains `cmake-build-debug/blobber` and uses the existing courtyard.

Implementation entry points: world sanctuary_city factory and building records;
assets world_assets/world_residency and baker; world_scene geometry/culling;
app city_review and review_sanctuary_city capture tool. Owned fixtures live in
`tests/support/architecture_fixtures.hpp` and the architecture CPU/GPU tests.

Next action: user review of this slice. SC02 becomes eligible for
separate planning only after SC01 acceptance. Do not automatically continue.

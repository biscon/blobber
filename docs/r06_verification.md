# R06 verification and handoff

Status: **Accepted and closed** (2026-10-31). The user approved the
[cutover design](renderer_cutover_design.md) with “Implement the plan.”
The user explicitly accepted R06 with “slice accepted, close up the new renderer
plan”. Milestone R-C and the complete renderer roadmap are accepted and closed.
Acceptance changes no executed checks or limitations; no new coverage is inferred.
[Gallery and door motion](screenshots/r06/index.html).
[Completed deletion inventory](r06_removal_inventory.md).

## Delivered behavior and removal audit

Normal launches, campaign New Game/Continue, arena/rematch/restart and replay images
use WorldRenderer exclusively. GameView requires a renderer reference; Presenter
has no CPU frame upload resource/API. Full-pixel world depth/IDs serve live picking
and windowless visual automation. The command/HUD controller requires a real session;
fabricated demo presentation and its alternate validation path are deleted.

124 tracked files were deleted: old renderer/cache/transition/raster code, bank
serialization/loading/baking, probe and comparison implementation, exclusive tests
and fixtures, standalone application demos/previews/captures, obsolete authored map/
recovery content and loader, preparation profile and public height showcase.
Useful timing, posture, source-sheet/metadata, command, depth/picking, allocation and
lifetime regressions use current world pipelines and real owned sessions. Original
source artwork, shared preparation and native UI/creator/portrait atlases remain.

Source/CMake searches find no SceneRenderer, FramePipeline, PreparedNpcs, old bank
reader/baker, renderer selector or executable legacy copy. Fresh and existing managed
build outputs contain no old banks/root manifest/profile/derivatives. The cutover
removed 160 explicit retired target/object outputs from existing build trees; their
Debug/headless graphs were refreshed without rebuilding/retesting those presets.
A final symbol audit inspected 327 binaries/libraries and removed 108 additional
stale outputs containing retired renderer/demo code; stale modified-source object
caches were discarded too. Current Release/clean-build executables remain valid.
Final cleanup also removes copied authored-demo map/recovery files. No install/CPack
rules exist; build-directory asset publication is the distribution mechanism.

## Executed checks

All intensive jobs ran serially at `nice -n 15 ionice -c 3`, with two build workers
at most and one test process. Memory/disk availability was checked first. Scratch
and clean build outputs are on the workspace disk under `scratch/r06/`, not a large
RAM-backed `/tmp` workload. No toolchain/package installation was required.

| Check | Actual result |
| --- | --- |
| Graphics Release build and ordinary CTest | **54/54 passed**, 8.38 s; labelled slow tests excluded by preset |
| Fresh graphics-disabled Release configure/build and ordinary CTest | **49/49 passed**, 7.34 s; no raylib dependency or image bake/copy |
| Fresh graphics Release configure/build of `blobber` | Passed, using the already pinned local raylib source; current world/UI assets generated, no obsolete banks/targets |
| Final affected follow-up after deleting unused demo loader/content and strengthening capability assertion | `party_content`, `world_visual_cli`, `retired_assets`: **3/3 passed** in graphics and clean headless builds; no repeated full suites |
| New-renderer CPU allocation/rollback guard | Passed in both ordinary suites; real session/command/HUD, first prepared world visibility/effect updates, movement/combat, travel and restore failure injection; zero tracked steady-frame C++ allocations |
| Generated publication fixtures | Ninja single-config and Ninja Multi-Config Debug/Release outputs passed clean/no-op/change/touch/missing/add/remove/failure cases, including mandatory prone-art rejection |
| Generated retirement fixtures | Passed seeded single/multi/custom-config layouts, idempotence, preserved world/native assets and external-directory/symlink rejection |
| Hidden integrated world GPU check | Passed door/effect depth/ID membership, impact color/fade, active/staged travel rollback, 17 failing CPU preparation positions among 32 checked, allocation and resource cleanup; **zero CPU/GPU edge differences** |
| Hidden environment GPU check | Passed heights, indoor/outdoor, qualities, transition, budgets, rollback, no frame uploads and zero tracked C++ allocations; **zero CPU/GPU edge differences** |
| Hidden native UI/resource lifecycle | Passed native art sampling/clipping, handles, capacity, release and context cleanup |
| Fresh regular-engine expedition | **1,070 requests** completed doors, combat, loot, descent, recall, save and Continue using default world visual mode; no backend selector |
| Strict replay images | Two serial passes matched the fresh trace and exported 18 campaign and 12 door PNG outputs; curated 17 PNGs plus a short door video retained |
| Actual optional MCP SDK stdio | Passed visual-mode safe save preview, durable save artifact, changed handles on resume, stale-handle rejection, fresh saved session and strict replay |
| Diff/source/build/output audit | Passed; no disabled legacy targets or runnable old-renderer dependencies remain |

The final loader deletion removes only the unused authored-demo JSON adapter and
its parsing assertions; core recovery, camp, inventory and campaign behavior retains
its existing coverage. Full-suite results above preceded that small follow-up;
the affected checks and build links were then rerun, rather than repeating matrices.

Representative commands (full outputs in `scratch/r06/*.log`):

```sh
nice -n 15 ionice -c 3 cmake --build --preset release
nice -n 15 ionice -c 3 ctest --preset release
nice -n 15 ionice -c 3 cmake -S . -B scratch/r06/clean-headless -G Ninja -DCMAKE_BUILD_TYPE=Release -DBLOBBER_BUILD_GRAPHICS=OFF
nice -n 15 ionice -c 3 cmake --build scratch/r06/clean-headless -j 2
nice -n 15 ionice -c 3 ctest --test-dir scratch/r06/clean-headless -j 1 -LE 'slow|stress|benchmark' --output-on-failure
nice -n 15 ionice -c 3 cmake --build scratch/r06/clean-graphics-config --target blobber -j 2
nice -n 15 ionice -c 3 python3 tests/world_build_tests.py cmake-build-release/blobber_bake_world cmake/WorldAssets.cmake
nice -n 15 ionice -c 3 cmake-build-release/blobber_world_integration_gpu_tests cmake-build-release/assets docs/screenshots/r06
nice -n 15 ionice -c 3 cmake-build-release/blobber_world_environment_gpu_tests docs/screenshots/r06
nice -n 15 ionice -c 3 cmake-build-release/blobber_gpu_smoke
nice -n 15 ionice -c 3 python3 tools/renderer_integration_review.py --runner cmake-build-release/blobber_headless --assets cmake-build-release/assets --scenario docs/screenshots/r06/campaign-scenario.json --output NEW_DIRECTORY
```

The clean graphics configure used `BUILD_TESTING=OFF`, Release/Ninja and
`FETCHCONTENT_SOURCE_DIR_RAYLIB` pointing to the existing hash-pinned checkout;
the ordinary graphics test suite used the normal Release preset. Final focused
headless rebuilds relinked the runner/party-content test after the demo loader was
removed. The SDK command was bounded to 90 seconds and used the existing `.venv`.

## Review artifacts and identities

[Gallery](screenshots/r06/index.html) includes actual production main menu, arena
setup and character-library callbacks; regular expedition outdoor/indoor scenes,
projectiles/creatures, loot, travel and Continue; and clearly labelled generated
height fixtures. The menu/library review used an isolated temporary character
library, never the user's data. The optional GPU test export calls the production
application library; there is no replacement public demo/capture executable.
Door video is four recorded boundaries, slowed to three frames per second for
inspection. Linked source PNGs retain native output; height fixtures are 500×282.

[Campaign capture metadata](screenshots/r06/campaign-capture.json),
[door metadata](screenshots/r06/door-capture.json) and [selected operations](screenshots/r06/review.json)
retain executable/trace/publication fingerprints. The 31 MiB strict trace and
isolated campaign save are in `scratch/r06/campaign-review/`. Capture fingerprints
refer only to `prepared/world/metadata.json` and `prepared/world/textures.bin`.
Same-process/same-environment replay is evidence, not a cross-device pixel guarantee.

Versions remain: protocol 1; campaign schema/simulation 2/2; world format 1,
appearance 4; `world-visibility-v3`, `terrain-rules-v1`,
`shared-opening-300ms-v1`; generator/template 3/3. Remaining CommandReason numeric
values are preserved when retiring the preview-only zero value. Renderer/visibility
selectors and the showcase scenario are removed APIs. Strict recordings from a
changed executable/legacy visibility reject; they are not migrated as save games.

## Limits and exact next action

GPU evidence comes from OpenGL 3.3 on **NVIDIA GeForce RTX 3060, driver 615.78.08**,
reported by the actual GL context. Initial sandboxed GPU access could not open the
display; authorized hidden checks outside the sandbox passed. The first sandboxed
SDK check timed out during MCP initialization; the bounded authorized repeat outside
the sandbox passed. These failed attempts are not counted as passing checks.

No OS input injection, window focus/control, interactive desktop review or other-GPU
verification was performed. No repeated Debug/sanitizer/full persistence matrix,
performance benchmark or capacity stress investigation was run. Prior accepted
R01–R05 evidence remains historical, not new R06 coverage. The frame allocation
claim covers engine-owned C++ storage; driver/raylib internals and export are excluded.

R06 and milestone R-C are accepted and closed; the renderer roadmap is complete.
Next work is separate planning for additional monsters when requested. Any future
cheat/debug interface also requires separate planning. PC07 remains paused.
Acceptance recording ran no additional builds/tests/captures and made no commits.

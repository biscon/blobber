# SC04 verification and handoff

Historical checkpoint; raw run artifacts were pruned on 2026-11-05.
[Archive index](README.md) · [Current workflow](../../verification_workflow.md).

Date: 2026-11-05. Status: **Accepted and closed — 2026-11-05**.

## Delivered behavior

The accepted dressed Sanctuary city is now the ordinary campaign hub. New Game
starts at (11,8) north; recall reaches the sanctuary at (9,6) north; ascent from
floor one reaches (13,7) south, facing into town. Continue restores the saved pose.
The downstairs fixture at (13,6) requires its adjacent landing and north facing.
The gate stays closed and has no action. Services/interiors remain deferred.

City discovery grows through walking and survives travel and Continue. Free rest
and the contextual R prompt use the same sanctuary definition. Recall costs one
camp kit only at commit and does not heal; unlocked-floor travel is still free
from the entrance. Existing Party/inventory identity, inactive floor effects,
loot, clocks and progression remain intact. Destination preparation failures and
cancellation preserve the source session, Party, view and picking.

[Design](../../sc04_design.md) documents ownership and interfaces. The small hub builder
derives fixtures/recovery/arrivals from the production city factory, shared with
optional review tooling. Courtyard content remains only in test support. No new
art, spatial subdivision, capacity or persistence fields were required. The entrance
review correction below adjusts prepared stair geometry.

## Compatibility and publication

Campaign schema/simulation are **9/13**; generator/templates remain **9/7**.
Diagnostic hub identity is `sanctuary-city-v1`; protocol remains v1. Publication/
appearance are **10/4**, visibility **v17**, world fingerprint revision **4**.
Unsupported old saves reject clearly without rewriting/deleting their bytes.
Canonical immutable city validation covers terrain, world fingerprint, anchors,
fixtures, protected cells and recovery. Mutable pose/discovery use existing codecs.

The Debug game and production publication built successfully with all 397 catalog
textures; catalog storage is not permanent residency. Optional review flags are
not needed for normal play. No user saves or character library were touched.

## Verification performed

One Debug configuration, sequential low-priority jobs, at most two compilation
workers and one test process. No desktop input, focus switching or OS automation.

- `sanctuary_city`: passed. Existing authored layout/reservation failures and
  protected routes remain covered.
- `campaign`: passed, 3.16 s. New tests cover all arrivals, closed gate, partial
  discovery, canonical definition rejection and a hazard specifically at the
  dungeon-return landing. Existing tests now navigate to stairs/rest explicitly;
  rollback tests reach the entrance before injecting failure.
- `campaign_save_smoke`: passed, 20.86 s. `campaign_save`: passed once at the
  integration checkpoint, 89.43 s, including the expanded city disk roundtrip,
  inactive state/effects, progression and I/O failure cases. The extended run also
  covers the smoke functions after the final saved-pose test adjustment.
- `campaign_save_cli`: passed, 12.29 s, including ordinary wire navigation,
  disk Continue, client/local MCP and strict replay.
- `map`, `recovery`, `world_environment`, `world_residency`: passed.
- `automation_allocations`: passed, 4.86 s. CPU-only frame guards and preparation
  allocation-failure rollback cover the new city and all travel kinds.
- `gameplay_startup`: passed, 1.17 s, after updating its entrance navigation.
- `campaign_city_route` and `campaign_route_tools`: passed, 0.32/0.05 s. The new
  short route checks the actual planner/client path from square to dungeon.
- Optional slow `campaign_route_fixture` reached its existing 30-second deadline
  during dungeon combat navigation after successfully leaving the city. It did
  not complete and is **not counted as passing**. No deadline increase or repeated
  whole-campaign run was used to obtain a passing result.
- Hidden `blobber_world_integration_gpu_tests`: passed on NVIDIA RTX 3060/OpenGL
  3.3. City movement/HUD/discovery refresh perform zero guarded C++ allocations
  and uploads; 32 injected CPU preparation failures plus GPU failure retain the
  active view. Integrated ID parity has zero differing pixels, and resources
  release correctly. Raw output (historical artifact removed).
- Hidden `blobber_architecture_gpu_tests`: passed architecture, gate, cutout and
  water checks with zero differing ID pixels. This includes water phase/pause
  behavior and warmed allocation/upload guards.
  Raw output (historical artifact removed).
- Hidden production review exercised New Game, staircase travel both ways,
  staging cancellation, paid recall, explicit rest, disk load and explored map.
  Nine native captures were exported; representative arrival/entrance/return,
  rest and map views were inspected. [Gallery](../../screenshots/sc04/README.md).
- `git diff --check`: passed.

The sandbox initially could not open display :0. Authorized hidden GPU runs then
succeeded with display access. Interactive menu/desktop review remains user-owned.
No benchmark, Release/sanitizer matrix or low-memory hardware test was run.

## Resource measurements

Seed 5, supersampling on, native 1920×1080 presentation. MiB = 1,048,576 bytes.
Raw report (historical artifact removed) includes dependency-union
projections for all depth bands and actual active/staged/released measurements.

| Resource | City active | City + floor one staged | Hard limit |
| --- | ---: | ---: | ---: |
| Resident textures | 32 | 253 | 480 |
| Texture mip bytes | 261.29 MiB | 455.72 MiB | 512 MiB |
| Alpha masks | 7.74 MiB | 41.80 MiB | 64 MiB |
| Tracked world CPU | 18.97 MiB | 61.13 MiB | 352 MiB |
| Tracked world GPU | 274.64 MiB | 470.73 MiB | 608 MiB |
| Application GPU | 394.74 MiB | 590.82 MiB | Advisory 384/448 MiB references |

The city/floor transition exceeds the advisory application target. All hard
ceilings pass; no quality or budget changes were made. Cancelling and returning
restore city-only residency. Clearing the view releases all location textures,
alpha masks and models; retained UI/fonts/renderer scratch total 129.78 MiB GPU.
Driver allocations are excluded. These measurements do not establish performance
or compatibility on low-memory GPUs. Production capture save files were isolated
review fixtures and moved to ignored build scratch, not included in the gallery.

## Retained review recipe and closure

Launch `cmake-build-debug/blobber` normally and start a new campaign. From the
north-facing square spawn, strafe right twice and move forward once to reach the
dungeon landing; F enters floor one. Return via its upstairs and confirm that the
party faces south into town. Turn around to re-enter, or use paid recall from the
dungeon to reach the sanctuary and explicitly press R to rest.

Explore the town, open M to inspect growing discovery, save, then Continue and
confirm pose/exploration/resources. The gate remains impassable; the service signs
remain scenery. Existing pre-SC04 saves cannot Continue; replacing a save still
uses the ordinary explicit New Game flow.

The user accepted SC04, including the entrance trim correction, on 2026-11-05
and requested closure. Milestone S-C and the Sanctuary city roadmap are complete
and closed. Recorded verification limitations remain unchanged. Future work
requires a separate request.


## Entrance review correction (2026-11-05)

Raised doorway trim protruded through the black staircase terminal panel. Building-backed
stairs now close 0.06 cells ahead of the footprint boundary, hiding that trim while
preserving the surrounding entrance. Shared CPU/GPU geometry uses visibility v17;
assets, save schema, travel and ordinary terrain-backed stair geometry are unchanged.

The normal game and affected review/test targets rebuilt successfully. Focused
`dungeon_refresh` passed (0.20 s). Hidden GPU integration passed, including city
entrance ID parity with zero differing pixels and the existing allocation/resource
checks (output (historical artifact removed)). Four production entrance views
were inspected: close, street, left and right; the bright strips are absent.
[Updated gallery](../../screenshots/sc04/README.md). No desktop input was used. The user accepted this correction with SC04
on 2026-11-05; the slice and roadmap are closed.

# SC02 verification and handoff

Date: 2026-11-05. Status: **Accepted by the user 2026-11-05.**
Scope: the approved [full town design](sc02_design.md). SC01 remains accepted.
Normal campaign Sanctuary is still the courtyard; SC03 and SC04 have not begun.

## Delivered town

The 24×24 authored development town has ten principal buildings: six homes, inn,
smithy, provisioner and sanctuary. The inn and two homes have two-storey shells.
A separate dungeon entrance and substantial closed gate complete twelve named
approaches. The modeled boundary occupies the inner ring; no engine placement,
collision, sight or wall-replacement rules were relaxed.

Arrival is (11,8), facing north; rest approach is (9,6), facing north; dungeon
landing is (14,7), facing north; its future stair cell is the clear cell (14,6)
in front of the opaque shell. Arrival-to-rest/dungeon distances are four/four
steps, and the rest-to-dungeon route is six. Gate approach is (12,20), facing south, with a clear
3×3 approach at (11,18). The gate remains opaque and noninteractive.

The residential loop connects (7,6), (7,14), (3,14), (3,6); the service loop connects
(15,7), (21,7), (21,14), (15,14). A one-cell alley runs through (4..6,10). Fountain
base (10,11), 2×2, retains a two-cell circulation margin. Market base (19,11), 2×2,
and three planting reservations remain undressed. Preparation validates all
remaining streets and named loops with these future bases blocked.

[Top-down layout and captures](screenshots/sc02/README.md) show the authoritative
layout, low/gabled and hipped roofs, distinct service facades, arched/static door
variants, gate joins and selected movement views. The final gallery has 55 main
captures plus eight gate-strafe frames. The tool's current default emits all 63.
The gallery records poses, transition fractions and draw counts.

## Assets and interfaces

The world factory owns fixed arrays for up to 16 named doorway records and 16
rectangular dressing reservations, plus named anchors and two loop descriptions.
Doorways associate stable names with checked placement indices and approach poses.
Metadata is preparation/review data, not persisted entities or dormant portals.
`Map` continues to own terrain and building footprints; frame callers borrow
prepared geometry through the existing scene/asset leases.

Ten named models use the accepted original material master, two 4096² atlases,
256 texels/cell, inset panel extraction and aligned 16-pixel gutters. Each model
currently has two material parts. The modular geometry and export/material helpers
produce packed Blender, deterministic unlit GLB, runtime JSON and export reports.
The former one-atlas/one-part exporter now supports the runtime's existing multipart
format; no runtime renderer or publication-codec changes were needed.

Publication/appearance remain 6/4, campaign schema/simulation 8/11,
generator/templates 9/7, visibility v14 and world fingerprint revision 3. Source
checksums and the authored review world change normally. No save migration,
campaign hub replacement, service interaction or gate travel was introduced.

## Verification results

One Debug configuration; sequential intensive jobs at low CPU/I/O priority, <=2
build workers and one test process. Available memory was about 19 GiB and disk
space about 454 GiB before work. No desktop input or focus automation was used.

- [Eight focused CPU/integration suites](screenshots/sc02/checks.txt) passed:
  sanctuary_city, architecture, options, campaign_save_smoke, dungeon_refresh,
  world_residency, world_visibility and world_integration.
- The new city suite checks complete connectivity, all closed approaches, sight
  blocking, alley width, gate clearance, fountain margin, future reservations,
  loop preservation, route lengths and invalid identity/association/capacity data.
  The [final anchor check](screenshots/sc02/anchor-checks.txt) constructs the existing
  downstairs fixture at the reserved cell/landing without changing the map; future
  routing treats it as blocked. Stair movement blocks while its landing stays clear.
  It uses authored CPU layout data without loading production art.
- Six fast owned Python material/export tests passed: panel-divider exclusion,
  fragmented UV continuity, padded mip isolation through level 4, packing overlap/
  exhaustion, deterministic two-page packing and triangle-boundary part splitting.
  These tests run without Blender or GPU and do not load the production master.
- Generated architecture fixtures now include a five-cell gate between boundary
  piers. [Focused CPU follow-up](screenshots/sc02/gate-cpu-checks.txt) verifies
  complete backing-wall suppression, pier endpoints, movement and sight blocking.
- [Hidden GPU checks](screenshots/sc02/gate-gpu-checks.txt) pass for both original
  architecture and the wide gate on NVIDIA GeForce RTX 3060, OpenGL 3.3. Both report
  zero differing CPU/GPU ID pixels. Covered staged failure/cancel/revisit, lifetime
  release, and warmed moving/settled frames with no engine-owned C++ allocations,
  asset reads or GPU uploads. Existing moving-picking/cache behavior is unchanged.
- Production exports validate geometry bounds and metric UV area density. The
  normal Debug game publication and review executable were rebuilt successfully.
- Both full loops and the alley run through production GameSession movement. Eight
  additional frames exercise left/right strafing on the clear gate approach.
  [Production stage/cancel accounting](screenshots/sc02/gate-motion/report.txt)
  confirms a second full city view prepares within aggregate limits, cancels without
  changing active revision/residency, and all world textures/models release when
  the active view ends.
- Inspected authoritative SVG rendered through rsvg-convert, arrival, gate street/
  roofline/both joins, inn entrance, alley, distant rooflines, two mid-turn views,
  and gate strafe. No exposed backing walls, roof gaps or material-divider seams
  were observed in those views. Close roof cropping and distant fog follow the
  unchanged camera policy. Visual acceptance remains the user's decision.
- The final `layout`-only CLI succeeds with a nonexistent asset root and no display;
  its SVG matches the inspected gallery layout exactly. New document links resolve.
- C++ formatting and `git diff --check` pass. No compiler warnings were reported
  in the final integration/review builds.

The save smoke took 19.78 seconds of the 21.19-second integration run. Inspection
shows it still runs six scenarios with repeated four-location construction and
serialization; it was run once, not repeated after capture/test-only changes.
No new campaign workload was added. Further persistence fixture optimization is
outside SC02. No full persistence, Release, sanitizer, separate headless build,
benchmark/stress or publication-build-dependency matrix was run. Publication build
rules and codecs were unchanged; owned roundtrip fixtures plus the actual bake
cover the affected multipart output.

## Actual resource totals

From the [main report](screenshots/sc02/review/report.txt),
[stage report](screenshots/sc02/gate-motion/report.txt) and pack export report.
All memory is application-owned accounting, excluding driver and desktop storage.

| Resource | Actual | Declared limit |
| --- | ---: | ---: |
| Named models / placements | 10 / 91 | 16 / 96 for SC02 |
| Doorways / future reservations | 12 / 5 | 16 / 16 |
| Materials / model parts | 7 / 2 per model | 32 / 8 per model |
| Unique source triangles | 7,885 | Existing per-part/model vertex limits |
| Prepared scene index triples | 26,202 | Existing scene budgets |
| Atlas pages | 2 × 4096² | 2 × 4096² |
| Resident world textures | 27; 217.29 MiB | 512 MiB mip textures |
| Resident alpha / model payload | 6.49 / 0.72 MiB | 64 MiB alpha; existing mesh limits |
| Scene CPU / GPU geometry | 2.10 / 2.10 MiB | 64 / 40 MiB |
| Visibility scratch | 7.28 MiB | 8 MiB combined view scratch |
| Tracked world CPU / GPU, active | 16.82 / 230.15 MiB | 352 / 608 MiB |
| Tracked CPU / GPU, active + staged city | 26.29 / 233.33 MiB | 352 / 608 MiB |
| Application GPU including UI/fonts | 345.75 MiB | Reported separately from world budget |
| Application capture peak | 361.57 MiB | Includes temporary native capture target |
| Maximum observed world draws | 24 | Fixed existing draw storage |
| Resident world textures/models after release | 0 / 0 | Complete view-owned release |

Scene index triples include the existing triangle-to-quad storage convention;
this is a submitted-index count, not a distinct authored polygon count. Visibility
scratch excludes the separately accounted effects structure; preparation enforces
the combined 8 MiB ceiling. No FPS or low-memory-device performance claim is made.
The 512 MiB–1 GiB devices referenced by the roadmap were unavailable.

## Corrections and limitations

Early layout checks caught an isolated southeast pocket and a planting reservation
crossing the residential loop; both were corrected before delivery. The exporter
rejected the first hipped-roof tangent projection on UV density; the corrected
projection passes. Blender's optional thumbnail-cache write was denied, while the
source/export files saved successfully. The first sandbox GPU attempt could not
open X11; approved display access enabled the hidden checks.

The first capture attempt stopped after three images because two diagnostic camera
poses were inside houses. Its partial `town/` evidence remains historical; corrected
poses are preflight-validated before opening the window, and the complete `review/`
run succeeds. No topology or rendering change was needed for that correction.

SC02 is intentionally undressed. The fountain/market/plants are reservations, and
all buildings are exterior scenery. A final integration review moved the future
dungeon fixture from the solid shell to the clear frontage cell; the landing
sits one cell farther out. The CPU check admits the existing downstairs fixture
without changing the map. This metadata-only correction leaves captures/geometry
unchanged. SC04 installs the actual fixture and binds travel to its named landing. Gate operation remains
future scope. The full review replaces SC01's former scene/shot names; historical
SC01 evidence remains intact.

## User review feedback — 2026-11-05

The user described the town as "very nice" and reported about 3,500 FPS uncapped.
This is a user observation, not an additional controlled benchmark; hardware and
measurement conditions were not specified with the feedback. No advanced occlusion
or additional spatial subdivision is requested. Retain the existing prepared
geometry, material batching and chunk/frustum culling.

The user also requested signs to distinguish the inn, smithy and provisioner.
Those signs are now explicit SC03 scope/acceptance in the roadmap, using static
modeled boards and generated painted emblems. No SC03 implementation accompanies
that documentation update. The user subsequently explicitly accepted SC02
("record this slice as accepted"); the slice is now closed and accepted.

## Reproduction and next action

```sh
nice -n 15 ionice -c 3 cmake --build cmake-build-debug --target blobber blobber_review_sanctuary_city blobber_sanctuary_city_tests blobber_architecture_tests blobber_architecture_gpu_tests -j 2
cmake-build-debug/blobber --sanctuary-city-review
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_sanctuary_city cmake-build-debug/assets /path/to/new-directory
# Add layout, stills, materials, motion, gate-motion, or an exact shot name.
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-debug -R '^(sanctuary_city|architecture)$' -j 1 --output-on-failure
nice -n 15 ionice -c 3 cmake-build-debug/blobber_architecture_gpu_tests
```

The first command is for user-controlled interactive review. Captures/GPU checks
use hidden windows; `layout` needs no display. See the [pack README](../assets/packs/sanctuary-city/README.md)
for regeneration and helper checks. Supersampling remains ON.

Next action: await a request for detailed SC03 planning, including service-building
signs. SC02 is accepted; this does not automatically start SC03, advance PC08, or
switch the campaign hub.

## Subsequent handoff — 2026-11-05

The user requested and approved SC03 planning/implementation. The first
representative-tree checkpoint is tracked in [SC03 verification](sc03_verification.md);
its user approval precedes the remaining foliage production. SC02 remains accepted.

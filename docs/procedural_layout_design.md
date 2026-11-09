# Variable stone layouts and shared generation geometry

Status: **Implemented and verified; awaiting user review**, 2026-11-01.
Implementation approval: “Implement the plan.”

Current follow-up: [Underkeep dressing](underkeep_dressing_design.md) implements
biome details and containers at generator/catalog 9/7, preserving this delivery's
8/6 terrain/population random streams. The geometry evidence below remains valid.

This follow-up replaces generator 7's spatial lattice. Historical PC01–PC07
acceptances remain historical. Campaign length, recipes, resources, movement and
prepared-image presentation were unchanged by this geometry delivery. Decorations
were deferred here and are now implemented by the linked Underkeep biome pass.

## Responsibilities and interfaces

`FloorSpec` resolves the three campaign depths into dimensions, biome, algorithm,
population band and key/guardian requirements. `BiomeDefinition` binds stone to the
existing dungeon publication and its two profiles, CompactWings and ChamberNetwork.
Only StoneRooms is implemented; future algorithms provide the same explicit output,
without inheriting stone's construction steps. Dispatch uses functions and typed IDs.

FloorLayout owns contiguous cells, chamber membership, bounded region records with
bounds/anchors/clearances, explicit boundary sockets/facings and ordered connection
paths in one index pool. Bounds are descriptive; membership is authoritative.
Fixtures, encounters, diagnostics and persistence consume this geometry, never a
lattice coordinate or a reconstructed centre. Local IDs last for the floor's life;
no slot recycling or frame lookup layer is introduced. Published Map/metadata remain
immutable; campaign/session ownership and preparation/commit lifetimes are unchanged.

## Stone construction

The enclosed 36×36 canvas holds 10–12 regions. Incremental bounded placement builds
a connected backbone at arbitrary integer coordinates. Shapes include rectangles,
halls, L rooms and pillared chambers (3–9 cells per dimension, 12–54 ordinary
walkable cells). Each normal floor has four room areas, a 2:1 area range and a
landmark group. CompactWings favors short/shared-wall attachments (75%);
ChamberNetwork uses 25%, with other attachments targeting 3–8 passage cells.
Sockets vary along boundaries. Straight/dogleg routing precedes bounded deterministic
turn-penalized search. Routes cannot silently cross/touch other passages or penetrate
unrelated rooms. Useful loops are checked on actual tiles with an eight-step detour.

Protected bent stair motifs retain safe landing sightlines. Guardian entry/gate,
reward and exit thresholds have explicit clearance. Depths 2–3 retain both keyed
sector entrances and a reachable upstream key. Current direct/optional/guardian
population, XP and reward quantities remain. Small geometry motifs are not a new
art/decoration system. Profiles are chosen once per floor; retries retain them.

## Bounds, failure and determinism

Capacities remain 1,600 cells, 16 chambers, 24 connections, 32 fixtures, existing
population limits, and a shared 1,600-index passage pool. At most 32 placement
proposals per chamber and 16 floor attempts precede checked authored fallbacks.
Exhaustion rejects a candidate; invalid fallback rejects publication. Retained
generation metadata stays below 16 KiB and scratch below 128 KiB. Preparation may
allocate; engine-owned ordinary frames reuse immutable data and prepared storage.
Seed domains isolate topology/interiors/encounters/rewards/runtime. Fingerprints
encode canonical fields, including membership, sockets, paths and biome/profile.
Generator/catalog 8/6 and campaign schema 6 reject old saves without rewriting;
simulation remains 10 and character libraries are independent.

## Validation and acceptance

Shared checks cover capacities, complete membership, explicit connections, final-tile
connectivity, fixture/progression states, return travel, landing safety and population
clearance. Stone-specific checks cover shape and spatial variety. Fixed seeds
0,1,5,8,13,21,34,55 across all depths test repeatability and ordinary generation;
forced fallback/malformed geometry test failure paths. Owned fixtures cover
campaign travel, automation/replay, save roundtrip/tampering and frame allocations.

Review seeds 0–3 across three depths plus both fallbacks as map sheets and selected
first-person captures. Aim for 230–390 walkable cells and progression-walk medians
within 25% of baseline 54/90/84 steps. These pacing checks do not establish balance.
Run one low-priority intensive job at a time, at most two build workers/one test
process. Full persistence runs once at integration. No broad sweeps, benchmarks,
OS input or desktop control. Record executed evidence and limitations here at handoff.

## Delivered implementation and evidence

The old lattice topology and template stamping files are retired. `geometry.cpp`
owns shared access/specification helpers; `stone_layout.cpp` owns stone shapes,
placement and routing; `fallback.cpp` contains two frozen, reviewed geometry records;
`metrics.cpp` owns bounded diagnostic path measurements. Generation orchestration,
fixtures, encounters/population and final validation retain separate responsibilities.
`FloorSpec` supplies population band, keyed-sector and final-floor policy; the stone
biome supplies the prepared pack, algorithm and supported profile IDs.

Chamber membership uses one byte per cell (`255` means passage/no chamber). Paths
use a shared uint16 cell-index pool with per-connection offset/count. Shape and
rotation are diagnostic/motif descriptors, not geometry reconstruction instructions.
Small landing motifs retain the stair-behind-arrival facing contract. Ordinary
encounter facing comes from its actual incoming connection, including bends.

The builder places 6–8 junctions and adds two useful cycle edges. Its selected
sector has two upstream neighbours; removing it leaves upstream connected with a
cycle. It then attaches guardian/exit safety regions, entrance and optional leaves.
Root hall dimensions vary, and its first two attached chambers form the landmark
group. Profile weights apply to proposals, not guaranteed final connection ratios.
Placement has 32 total proposals per region, spread over bounded parent choices.
Loop search has 96 pair proposals per extra connection. Routes first try monotone
straight/dogleg paths, then indexed-heap A* with step/turn costs 10/3, Manhattan
heuristic and stable state-ID ties. Its 6,400 states and 20-cell route admission cap
bound routing; no crossings, incidental contacts or hidden graph edges are accepted.
Depth-1 entrance parent selection favors a 28-step interior route to the guardian,
leaving room for stair nooks and the final connection within the pacing target.

### Executed verification

Release application, generator, headless runner and affected CPU/GPU test targets
built with `nice -n 15 ionice -c 3`, at most two build workers. No toolchain/package
installation. One intensive command ran at a time.

```sh
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-release \
  -R '^(generation|fixtures|population|guardians|campaign|generation_automation|fixtures_automation|generation_cli|campaign_save_smoke|campaign_save_cli|campaign_save|automation_allocations)$' \
  -j1 --output-on-failure
```

**12/12 passed in 11.78 s.** Full persistence ran once at integration (4.92 s).
Coverage includes both profiles across eight fixed seeds/three depths, independent
RNG domains, rotation/reflection variety, the maximum seed, both checked fallbacks,
invalid membership/socket/path/clearance/capacity records, key/sector/gate failures,
real AI encounter approaches, travel and strict fresh-process transport replay.
Save tests reject old schema 5 and corrupt new geometry/enum records. Allocation
checks cover prepared generated/campaign processing and preparation rollback; they
intercept C++ new/new[], not third-party/driver allocations.

The new stair-facing regression was corrected. Test walkers now use actual socket
sides, path geometry and settled presentation barriers; encounter approach fixtures
explicitly clear the incoming room and acquire required keys before approaching.
These changes preserve real checked commands, discovery and encounter isolation.

```sh
nice -n 15 ionice -c 3 python3 tools/layout_gallery.py \
  cmake-build-release/blobber_generate_floor NEW_REVIEW_DIRECTORY
nice -n 15 ionice -c 3 cmake-build-release/blobber_world_integration_gpu_tests \
  cmake-build-release/assets NEW_CAPTURE_DIRECTORY --layout-review
```

Hidden production captures passed on raylib 6.2/OpenGL 3.3, NVIDIA RTX 3060. Ten
1920×1080 images cover both profiles' landings, halls, bends, shared walls and guardian
thresholds. GL errors were checked after every draw, and world leases returned to
the persistent presenter's resource baseline before registry/window destruction.
The full map sheets, fallback sheets and capture contact sheet were visually
inspected. [Review gallery and manifests](screenshots/variable-layouts/README.md).
No OS input or focus/desktop automation ran. Interactive play remains user review;
no new GPU pixel-parity matrix, sanitizer matrix, balance campaign or broad seed
sweep was run.

### Scale and memory

| Depth | Previous median progression steps | New median | New walkable-cell range |
| --- | ---: | ---: | ---: |
| 1 | 54 | 58 | 299–341 |
| 2 | 90 | 83.5 | 248–351 |
| 3 | 84 | 70 | 256–312 |

These are seeds 0–3, not production-scale statistics. Progression distance follows
entry → key when present → guardian approach → exit landing with appropriate
fixture states; it does not count combat, interactions, turns or exploration detours.

The CLI reports 11,096 bytes for the generation record plus 36×36 cell bytes
(11,400 bytes at the hard 1,600-cell capacity), below the 16 KiB metadata budget.
It separately reports the Map's existing WorldDefinition storage: a conservative
10,383 dynamic bytes, including string capacity, giving 21,479 retained bytes for
this floor definition. NPC/session/render assets are separate owners, as before.
Reported scratch allowance is 113,248 bytes, below 128 KiB; compile-time bounds
cover the candidate/progression workspaces and routing heap. Preparation/diagnostic
JSON, images and Map construction may allocate; ordinary frame containers do not grow.

Campaign length and population/resource quantities are unchanged. Caves, mines,
additional campaign bands and biome decals/billboards/models are not implemented
in this deliverable. Geometry changes can change combat circumstances despite equal
recipes and XP; the fixed structural/pacing sample does not establish balance.

## Architectural ceilings — 2026-11-09

Status: **Implemented and mechanically verified; user visual acceptance pending.**

A generation-owned architecture pass runs after successful dressing and before Map
publication. The immutable WorldDefinition owns its existing row-major surface array;
no height fields are duplicated in Room, and no frame processing is added. Count
actual room-member floor cells before furniture, excluding pillars and missing corners.
Entrance/objective stair regions, passages and door/gate cells keep ceiling 4 ticks.
Other rooms use 5 ticks below 20 cells, 6 ticks otherwise, or 7 ticks at >=30 cells
with both dimensions >=5. L rooms and narrow halls cap at 6. Large pillared rooms
use 8 ticks. Each tick is a quarter cell; all floors stay at zero. Threshold overrides
create lintels with existing interval geometry. Both profiles/depths/fallbacks share
the rules. No new randomness, lighting, materials, artwork or movement changes.

The pure classifier and bounded preparation helper live in generation/architecture.
The pass counts membership once into maxRooms scratch, initializes at most maxCells
surfaces, and adjusts existing decoration records. Pilasters reach the local ceiling;
webs retain their ceiling offset and size. Door surrounds, furniture and other dressing
retain their dimensions/positions. Surface allocation occurs only during preparation;
existing Map budgets, invalid-surface checks, scene bounds and rollback apply. The
pass accepts validated generation geometry and fresh, unadjusted dressing. Diagnostic
room reports expose derived ceiling ticks; existing fingerprints cover final surfaces.

Generator 13/templates 9 reject older campaigns unchanged under the existing strict
policy. Dressing RNG explicitly retains 12/9, other streams retain 8/6; the ceiling
change therefore preserves layouts, encounters and decoration/chest placements. No
save schema, simulation or asset publication format changes are needed.

Verification: small owned fixtures cover tier boundaries, irregular membership,
rotations, narrow/L/pillared rooms, stair/door exceptions, dressing attachment, traversal
and fingerprint changes. Existing fixed-seed normal/fallback cases exercise both
profiles and all depths. Focused Debug generation/dressing/fixtures/environment/render,
CLI/replay, save and allocation checks precede one hidden GL review, with <=2 build
workers and one intensive job at low priority. Review corridor transitions, room tiers,
pillars, open/closed doors and stairs; no OS input. Keep <=6 selected images and
record commands/results here; user visual acceptance remains separate.

### Architectural ceiling verification

Debug game, headless runner, generation CLI, affected tests and hidden GPU review
runner build with the existing `/home/bison/clion/bin/cmake/linux/x64/bin/cmake` and
configured `/home/bison/clion/bin/ninja/linux/x64/ninja`, at most two workers. Existing
baker relinks triggered world/creator/portrait preparation; source artwork and asset
formats were not changed. One intensive job ran at a time, with low CPU/I/O priority.

```sh
nice -n 15 ionice -c 3 /home/bison/clion/bin/cmake/linux/x64/bin/ctest \
  --test-dir cmake-build-debug \
  -R '^(generation|dressing|fixtures|world_environment|underkeep_render|campaign_save|automation_allocations|generation_automation|generation_cli|campaign_save_cli)$' \
  -j 1 --output-on-failure
```

**10/10 passed in 33.79 s**, including full persistence once (7.47 s), active/inactive
world fingerprints, generator-12 rejection without file mutation, CLI/replay and
engine-owned C++ allocation guards. Normal/fallback fixtures cover both profiles and
all depths. Six pre/post CLI comparisons (seeds 0,5; depths 1–3) preserved map, room
geometry, connections, fixtures, population, encounters, dressing locations, containers,
landings, spawn, runtime seed, attempts and fallback state exactly. The only new room
report field is nominal `ceiling_ticks`. No additional retained world surface storage
is required: Map already allocated the same 8-byte-per-cell array for default heights.
Classifier scratch is two maxRooms arrays (96 bytes); publication uses existing budgets.

```sh
nice -n 15 ionice -c 3 cmake-build-debug/blobber_world_integration_gpu_tests \
  ASSET_ROOT NEW_OUTPUT --ceiling-review
```

Hidden raylib/OpenGL 3.3 review passed on NVIDIA RTX 3060. Twelve captures covered
four room tiers, both profiles' low stair regions, doors closed/open and thresholds.
Checks include GL errors, CPU/GPU interior picking (<100 nonedge discrepancies per
500×282 view) and GPU resource return to the persistent presenter baseline. Selected
images were visually inspected for ceiling joins, pilasters, pillar height and door
proportions. [Six native captures](screenshots/architectural-ceilings/README.md) remain.

The first sandbox run could not open X11; the authorized hidden display run was used.
Its first attempt stopped on a review-harness omission: relocated poses had not called
fixture discovery before interaction. Adding discovery fixed the harness; the rebuilt
review passed. No game-code correction or CPU-suite rerun was needed. Interactive
acceptance is pending; no desktop input/focus automation, broad seed sweep, benchmark,
sanitizer or second build configuration ran. Allocation checks do not cover driver or
third-party allocations. All runners closed; disposable artifacts and temporary asset
links were removed after selecting evidence. Existing saves and build caches remain.

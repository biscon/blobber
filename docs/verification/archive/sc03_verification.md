# SC03 verification and handoff

Historical checkpoint; raw run artifacts were pruned on 2026-11-05.
[Archive index](README.md) · [Current workflow](../../verification_workflow.md).

Date: 2026-11-05. Status: **Accepted by the user — complete SC03 implementation**.
The user approved the shorter representative tree and authorized continuation.
The remaining planting, market, fountain, three service signs and optional water
animation and all subsequent review corrections are accepted. SC04 has not started.

## Delivered behavior

The accepted courtyard tree remains unchanged, now in a 3×2 planted bed at (4,4).
A smaller eastern tree and bushes fill the 2×2 eastern bed; a 2×1 bush bed dresses
the western lane. Sparse ivy appears on two selected residential facades. Two
north-facing timber stalls with muted russet/green cloth occupy two reserved
cells at the square’s southwest edge. A low, broad stone fountain fills its 2×2 base in the square.
Tankard, anvil and provisions emblems hang on modeled, double-sided timber signs
at the inn, smithy and provisioner. Services and campaign cutover remain deferred.

All six physical bases stay inside the five SC02 reservations. Player/NPC movement
is blocked at bases; cell LOS and generated floor remain intact. Actual geometry
controls depth and picking. Both street loops, alley, gate approach, rest anchor
and future dungeon landing remain clear. There are 91 architecture placements,
19 named models, six scenery placements and 12 materials.

Water uses eight prepared opaque texture frames at 8 FPS on fixed basin/stream
geometry. The shader selects the frame from the existing gameplay clock. Color
changes only when a water-bearing culled chunk is present and its phase changes;
CPU visibility/picking and uploaded geometry stay unchanged. This is conservative
chunk visibility, not pixel-level occlusion. Pausing freezes water. Offline glints
and small ripple arcs retain a stepped painted look; there is no transparency
sorting, reflection, particle or fluid simulation.

## Design, artwork and compatibility

[SC03 design](../../sc03_design.md) records placement ownership, cutout depth semantics,
water interfaces, preparation/frame boundaries, capacities and tests. World
publication is **10**, appearance **4**, campaign schema/simulation **9/12**,
generator/templates **9/7**, visibility **v16**, world fingerprint **4**. Old save
versions reject without migration or modification. The water addition did not
change save fields or depth identity after the tree checkpoint.

The original generated art and exact built-in ImageGen prompts are retained in
[source/tree-art.prompt.json](../../../assets/packs/sanctuary-city/source/tree-art.prompt.json)
and [source/dressing-art.prompt.json](../../../assets/packs/sanctuary-city/source/dressing-art.prompt.json).
The latter records six new masters plus the ivy transparency correction. Static
emblems are generated artwork. Planting reuses approved foliage; material packing,
resampling and water animation phase preparation happen offline.

Packed Blender sources, unlit GLBs, runtime mesh JSON and export reports are in
[models](../../../assets/packs/sanctuary-city/models). The accepted tree is 610 triangles
and 2.691 cells tall; the dressed gardens are 718/96/706 triangles, each stall 216,
and the fountain 604. Dressed service/ivy variants use up to four parts per model.
Shared source paths are interned by the pack baker so variants reuse published
mesh IDs and resident payloads. New shared textures are 2048² opaque, 1024² cutout,
and 2048×1024 water, within the agreed art ceilings. Sixteen-pixel water frame
gutters and atlas padding retain approximately 256 texels/cell.

## Verification performed

One Debug graphics configuration was used. Jobs ran sequentially at low CPU/I/O
priority; compilation used at most two workers, production baking one. No desktop
input, focus switching or OS automation was used.

- Final `sanctuary_city` and `scenery`: passed. Owned fixtures cover rotated bases,
  bounds/overlap/capacity/level failures, floor/LOS separation, alpha holes/depth,
  water publication roundtrip and invalid flags/UVs. City checks cover all filled
  reservations, circulation, protected approaches and rejection paths.
- Final `architecture`, `world_residency`, `world_cpu`, `world_visibility` and
  `world_environment`: passed (1.39 seconds total). These are the affected
  publication, prepared geometry, residency and visibility callers.
- Expanded hidden `blobber_architecture_gpu_tests`: passed on NVIDIA RTX 3060,
  OpenGL 3.3. Architecture, wide gate, cutout scenery and animated-water modes all
  had **zero differing CPU/GPU ID pixels** in tested front/back/oblique views.
  Staging failure/cancel/revisit/release checks pass. Warmed moving/settled frames
  and all water phases perform zero engine-owned allocations, asset reads or GPU
  uploads. Water checks cover 0/124/125/999/1000ms, exact phase wrap, unchanged
  picking revision, paused-clock caching and fully culled water caching.
- The earlier tree checkpoint passed `campaign_save_smoke`, including scenery
  roundtrip, old-schema rejection and bounded-array rejection. It also passed
  `underkeep_render`, `world_visual_cli` and `world_integration`. Those checks were
  not repeated after art-only corrections; save/persistence code did not change.
- The production baker accepts all final meshes, normals, UVs, model bindings and
  budgets. Final Debug game and review executable are built with publication 10 after the view-distance correction.
- Hidden production captures traversed both complete loops, alley and gate/tree
  movement samples without blockage. Twenty-four final dressed views include two
  mid-turn views. Nine water captures cover eight distinct phases and a paused
  sample. Every active water frame changes color, picking remains revision 1,
  and the paused capture is pixel-identical to the preceding active capture.
- Production active/staged cancellation preserves the active view; release returns
  location textures and meshes to zero. `git diff --check` passes.

Evidence: layout/scenery checks (historical artifact removed),
CPU integration (historical artifact removed),
GPU checks (historical artifact removed),
water clock/cache report (historical artifact removed),
pixel comparison (historical artifact removed).

## Final resource measurements

From the final native capture report (historical artifact removed).
MiB means 1,048,576 bytes. These are residency measurements, not an FPS benchmark.

| Resource | Measured | Declared limit |
| --- | ---: | ---: |
| Named models / scenery / materials | 19 / 6 / 12 | 24 / 32 / 32 |
| Architecture placements | 91 | 96 content / 128 engine |
| Resident textures | 32 | 480 |
| Resident mip textures | 261.29 MiB | 512 MiB |
| Resident alpha | 7.74 MiB | 64 MiB |
| Resident model payload | 0.99 MiB | bounded model/part/vertex catalog |
| Scene CPU / GPU | 2.58 / 2.58 MiB | 64 / 40 MiB |
| Visibility scratch | 7.28 MiB | 8 MiB combined view scratch |
| Active world CPU / GPU | 18.95 / 274.63 MiB | 352 / 608 MiB |
| Active + staged world CPU / GPU | 28.90 / 278.28 MiB | 352 / 608 MiB |
| Application GPU / capture peak | 390.22 / 406.04 MiB | 384 / 448 MiB reference targets |

The application steady reference is exceeded by 6.22 MiB; hard world budgets and
the capture peak reference remain satisfied. No budget was raised. The new atlas
was reduced from 4096² to 2048² without reducing metric detail. Scene index triples
are 32,154 (the prepared triangle-to-quad convention includes degenerate halves).
Maximum observed draws in the six distance/tone views: 41. Installed publication contains
397 textures, 570,711,452 mip bytes and 61,267,045 alpha bytes; location residency
loads only its dependencies. Driver allocations are excluded from engine counts.

## Visual review, limitations and next action

[Capture index and animated water preview](../../screenshots/sc03/README.md).
The first dressing captures exposed overly high signs and dark stream strips;
the final art lowers signs modestly, lowers the basin rim and gives streams
brighter stepped glints. Street approaches show distinct signs; standing directly
under a sign or canopy can crop it with the preserved low, fixed camera. Captures
include that behavior rather than changing camera/framing rules. Foliage remains
fixed crossed/tilted cutout cards and can reveal its construction at some angles.

The shorter-tree before/after comparison remains retained, and the user has
accepted the complete dressed city and water loop on 2026-11-05, after all review
corrections. Agent-controlled interactive desktop review, Release/sanitizer matrix,
benchmark, stress and low-memory-device checks were not run. Tested GPU agreement
is not a claim of cross-device pixel identity or driver allocation freedom.

Launch `cmake-build-debug/blobber --sanctuary-city-review`. Regeneration and focused
capture commands are in the [pack README](../../../assets/packs/sanctuary-city/README.md).
**SC03 is accepted.** SC04 is eligible for separate planning; await the user’s
request before starting it or replacing the campaign Sanctuary.

## Screenshot correction verification — 2026-11-05

The requested corrections are implemented; SC03 is **Ready for review**.
The [design addendum](../../sc03_design.md#user-review-corrections--2026-11-05) records
placement, geometry and atmosphere decisions. Dedicated generated soil/marble
masters and exact prompts are retained. Blender exports and the Debug application
build completed at bounded worker counts; runtime geometry/material validation
passed. The accepted tree mesh is unchanged.

- Four focused CPU suites pass: sanctuary layout, architecture, scenery/publication
  and resource residency. Regression coverage includes two cells beside the inn,
  clear former market alley, square circulation, custom/default fog roundtrips
  and invalid fog values. Results (historical artifact removed).
- Hidden GPU environment and architecture checks pass. Generated atmosphere
  fixtures retain distant wall and actor color with capped fog and produce
  pixel-identical dungeon frames with either daylight profile. Shell, gate,
  foliage and animated water CPU/GPU ID differences are zero; warmed engine-owned
  allocations/uploads remain unchanged. Results (historical artifact removed).
- Thirteen production correction stills and eight cardinal/diagonal fountain
  views were inspected. Stalls fit the square, ivy no longer breaks the roofline,
  bushes meet soil, and fountain stone contrasts with paving. Closed water streams
  have visible thickness at every sampled angle, with ordinary physical occlusion.
- Eight production water frames change only the water region. Picking revision is
  constant; a paused update preserves time, cached image and every pixel.
  Pixel comparison (historical artifact removed).
- Current resource table above uses the correction capture report. Texture count,
  mip/alpha bytes and atlas dimensions are unchanged. The small geometry increase
  stays inside existing hard budgets; the previously exceeded application steady
  reference remains documented. Staging cancellation preserves the active view,
  and clearing releases all city texture/model leases.

Publication is now 9 for pack daylight fog. Only sanctuary-city overrides the
legacy outdoor profile; indoor fog constants remain unchanged. No save, simulation
or visibility format changes. No desktop input/focus automation, broad test matrix
or performance benchmark was run. Final interactive art acceptance remains the
user's review; SC04 has not started.

## Far enclosure and fountain tone — 2026-11-05

The city pack now sets `viewDistance` to 40 cells, enough for its complete 24×24
enclosure from every walkable location. GPU projection, CPU rasterization, actor
selection, effects, leaf occlusion and ground picking use the shared camera limit.
Other installed packs (`dungeon`, `underkeep`) publish 16 cells. Capped city haze
is unchanged. Fountain stone is restored to the original carved material with
face shading raised modestly; the rejected white-marble images remain historical.

Five focused CPU suites pass (architecture, scenery, world CPU, visibility and
integration), plus the visual CLI check for visibility diagnostic version 16.
The hidden environment GPU suite also passes the new distant wall/actor cases,
existing fog isolation and allocation/resource-lifetime checks. GPU actor IDs and
CPU visibility agree beyond the old cutoff. The CPU fixture checks diagonal
candidate reach, clipping beyond 40, invalid publication distances and cache
identity. CPU results (historical artifact removed),
CLI result (historical artifact removed),
GPU results (historical artifact removed).

The Debug application and assets are rebuilt (publication 10). Six native hidden
captures were inspected: southern square, one step forward, corner, halfway turn,
close fountain and the previous square view. Far walls and rear roof structures
remain complete in the paired southern views. [Gallery](../../screenshots/sc03/README.md).
Draw-list storage increases by 104 KiB per renderer; no frame allocations or uploads
were introduced. The resource table above reflects this capture, with unchanged
GPU residency and visibility scratch. No unrelated broad test matrix, benchmark,
OS input automation or campaign cutover was performed. Final visual acceptance
remains user review.

## Inn sign mount — 2026-11-05

Shifted the inn sign 0.55 cell along its facade onto the solid strip between the
upper-window trim and corner timber. Exported mesh comparison confirms unchanged
height, wall penetration, normals and vertex count (UV rounding below 0.000002).
Other runtime meshes and the atlas are unchanged. Blender export, Debug application
asset rebuild and three hidden captures passed; street and close views confirm
the mounting correction, while the frontal view crops the sign at the fixed camera.
No runtime code changed, so no broad test suite was rerun. Resource counts are
unchanged. Geometry check (historical artifact removed),
street (historical artifact removed),
[close](../../screenshots/sc03/inn-sign-v1/sign-inn-near.png),
capture report (historical artifact removed). SC03 remains Ready for review.

## Acceptance and next action — 2026-11-05

User instruction: “nice record the slice as accepted”. SC03 is **Accepted**, including
all screenshot corrections through the inn sign mount. Milestone S-B is accepted.
The preceding dated verification entries retain their original handoff history;
this acceptance supersedes their pending-review status. SC04 remains planned,
with no implementation or campaign cutover started.

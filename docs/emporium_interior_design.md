# Arcane Emporium interior

Status: implemented and mechanically verified; runtime visual/interactive review
pending (2026-11-07).
The user approved both [interior and exterior sign](mockups/magic-shop-v1/README.md)
and the name Arcane Emporium. This slice follows the smithy interior: build the
room, city connection and sign first. Merchant identity/art, dialogue, stock and
scroll mechanics remain a subsequent slice; inn supplies remain available.

## Authored layout and presentation

Use the existing authored location/prepared-mesh pipeline. A 12x12 map has an
8x8 walkable floor from (2,2) through (9,9), solid padded enclosure and the usual
1.5-cell ceiling / .5-cell eye. Entry (5,9) north; exit same cell south. World pack
`sanctuary-emporium` reuses city painted stone/timber, inn ceiling and fixed
1920x1080 rendering. No new lighting, runtime geometry, animation or allocator work.

| Fixture | Reserved cells |
| --- | --- |
| Potion cabinet | (2,2)–(2,4), east-facing |
| Scroll/book cabinet | (2,6)–(2,8), east-facing |
| Rear books/drawers | (5,2)–(7,2), south-facing |
| Counter | (5,4)–(7,4), south-facing |
| Alchemy bench/herbs | (9,4)–(9,6), west-facing |
| Future merchant | (6,3), subcell (1,1), south-facing |
| Customer | (6,5), north-facing |

Columns 5–6 remain clear from entrance to counter, column 8 is the staff bypass
and row 3 connects behind the counter. Reserve scenery in whole cells without
making it full-height sight walls. Mesh bounds must fit each reservation. The
future merchant must fit existing two-cell dialogue range without occupying any
walking route; prove all remaining cells reachable using a small owned resident.
Counter/bench .30 cells high, cabinet .9 cells maximum, hand-sized bottles/scrolls.
Indigo inset and the approved flask/scroll/star artwork distinguish the counter.
Alchemy heat is baked coloration; no new fire emitter or gameplay mechanic.
Runtime review correction: move only the hanging herb rail and its three bundles
.65 cells along the bench toward screen left when facing east. Local X increases
by .65, placing the rail at world Z=4.05..4.85, clear of the window opening at
Z=5.24..5.76. Keep the bench, bottles, still, window, collision reservations and
save identity unchanged. Regenerate packed source/runtime assets together and
inspect only the affected alchemy/entry views; reuse prior gameplay checks.

## Assets and sign

Offline `tools/export_emporium.py` stages a dedicated pack in an owned agent run:
packed Blender, unlit GLB, runtime meshes and metric painted atlas generated
together. Reuse existing material masters with bounded palette variations and
256 texels/cell, 16-pixel gutters, at most one 4096-square prop atlas. Finite prop
counts and simple low-poly forms avoid arbitrary object/per-frame costs. Native
geometry is appropriate for these props; use the approved generated sign artwork
for its emblem, never substitute a procedural UI icon.

Keep the current city building geometry/model key `provisioner` to avoid cosmetic
geometry churn, but give its canonical doorway the semantic ID `emporium` at the
same street approach (16,9), east-facing. Replace the old bread/wheat/sack sign
with the approved indigo potion/scroll/star sign. Extend the existing city dressing
exporter to accept a staging asset root; stage a complete city pack, replace only
the sign source and regenerate its packed Blender/GLB and atlas/meshes. Compare
every previous runtime vertex/UV record to demonstrate no unrelated geometry
changes before promotion. Inspect the actual sign from legal street cells.

## Campaign, ownership and compatibility

Append Emporium=6 (seven fixed locations), leaving all current IDs stable. Raise
canonical doorway storage from two to three links: city slots inn=0, smithy=1,
emporium=2; each interior has only slot 0 back to city. Static action labels say
Enter/Leave Arcane Emporium. Existing checked world/slot/revision/facing handles,
transactional prepare/commit/cancel and source-view lifetime ordering apply.
No new travel kind. Exclude the third city arrival from NPC placement.

An `emporiumInterior()` constructor/validator owns map, entry/exit, fixtures,
empty placements and restore rejection for tampered authored state. Use shared
interior recall/waypoint restrictions and ordinary rest rules; no inn free rest.
HUD/map/automation identify Arcane Emporium at depth zero and authored diagnostic
identity `sanctuary-emporium-v1`. Add one bounded FloorState, with the same existing
resource ceilings and explicit preparation failure/rollback behavior. Dynamic
mesh/catalog loading stays in preparation; steady-state frames reuse all storage.

Seven-location wire data requires campaign schema/simulation 16/19, replacing
15/18 with strict old-save rejection and no migration. User saves remain untouched.
NPC definitions, item rules, dungeon generator and existing merchant stock are
unchanged. Future merchant stock can move temporary inn potions when implemented.

## Verification

Use small self-owned CPU fixtures for grid layout/route/merchant range, all three
city slots, stale/foreign/wrong-facing requests, labels, preparation failure,
cancel/source preservation, warmed simulation/command allocation guards and
recall/rest restrictions. Check disk Continue, inactive discovery/drops, old save
rejection and authored tampering, plus inn/smithy/city regressions. Run focused
headless Release tests and one extended persistence check, max two low-priority
build workers / one test process. No benchmarks or broad configuration matrix.

Inspect bounds, triangle winding, metric UVs and all texture bindings; bake once
at integration. Hidden Debug application-owned captures cover exterior sign,
entry, counter, scrolls, alchemy bench and exit/Continue. Verify staged city/room
resource budgets, GPU failure/cancel rollback and final lease release. Never use
desktop input/focus control. Retain six selected review images with a README,
record commands/results/limitations here and clean the owned run after preview.

## Verification results — 2026-11-07

Implemented seven fixed locations and all three city links, preserving prior
location and doorway IDs. City semantic doorway `emporium` retains the previous
building/footprint and east-facing (16,9) approach; its model key/source sign
filename remains `provisioner`. Header/map/automation use Arcane Emporium, depth
zero, `sanctuary-emporium-v1`. Schema/simulation are now **16/19**, no migration;
existing user saves, NPC catalog, item rules and merchant inventories are untouched.

The staged exporter delivers 8 models / 15,206 outward triangles, split into 14
parts, and one 4096-square atlas. Counter/bench .30-cell work surfaces and all
scenery world-space bounds fit their reserved cells. Independent mesh checks pass
all texture bindings, non-degenerate triangle/normal winding and metric UV edges;
maximum error .005655 texels, within .015 tolerance. Cabinet anchors are centred
on their three-cell reservations; rear shelf/counter fronts face south toward
customers. No props extend into the two-cell approach or staff bypass.

The city sign source is byte-identical to the approved generated PNG. Regeneration
changes its source PNG, dressing atlas and packed Blender/GLB only: all **39 city
runtime mesh files**, pack bindings and other PNG sources are byte-identical.
The altered atlas region is x=1232..1408, y=1088..1232, including padding. Original
generated prompt/provenance is retained beside the source and in mockups.

The city export completed all writes and validation but reached the 180-second
bound during host audio teardown. The new interior export, using the null audio
driver and factory startup, exits successfully in about two seconds. Read-only
host thumbnail/extension-cache warnings do not affect packed source/runtime files.
Use this bounded reproduction pattern with a fresh owned staging run:

```sh
# Copy the city pack to RUN_DIRECTORY/assets/packs/sanctuary-city first.
ALSOFT_DRIVERS=null OMP_NUM_THREADS=2 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 \
  timeout 180 blender --background --factory-startup -noaudio --threads 2 \
  --python-exit-code 1 --python tools/export_city_dressing.py -- \
  --output-root RUN_DIRECTORY/assets
ALSOFT_DRIVERS=null OMP_NUM_THREADS=2 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 \
  timeout 180 blender --background --factory-startup -noaudio --threads 2 \
  --python-exit-code 1 --python tools/export_emporium.py -- \
  --output-root RUN_DIRECTORY/assets --report RUN_DIRECTORY/emporium-export.json
```

Focused headless Release build/checks:

```sh
nice -n 15 ionice -c 3 cmake --build --preset headless-release --target \
  blobber_emporium_tests blobber_smithy_tests blobber_inn_tests \
  blobber_sanctuary_city_tests blobber_campaign_tests blobber_campaign_save_tests -j 2
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-headless-release \
  -R '^(emporium|smithy|inn|sanctuary_city|campaign|campaign_save_smoke)$' \
  -j 1 --output-on-failure
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-headless-release \
  -R '^campaign_save$' -j 1 --output-on-failure
```

All six distinct focused suites pass. Initial emporium tamper coverage assigned a
cabinet's already-canonical coordinate; change it to an increment so it actually
tests corruption, then rerun only `emporium` (0.27 s). The other five suites passed
in the original 3.44-second selection and are reused. The extended persistence
check passes in 6.98 s. New coverage proves actual cardinal routes with the future
resident cell occupied, customer facing/range, all three slots, failure/cancel/
stale/facing handling, ordinary rest/interior restrictions, warmed allocation-free
engine frames, dropped items/discovery/Continue, inactive travel via inn/smithy,
canonical tampering and old schema rejection. Fixtures own their content/art;
no production assets are dependencies of CPU correctness tests.

The Debug game and review runner build; publication has **543 textures**,
856,180,096 mip bytes and 64,012,448 alpha bytes (whole catalog, not live residency).
Hidden production review on NVIDIA RTX 3060 / OpenGL passes:

```sh
OMP_NUM_THREADS=2 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 \
  cmake-build-debug/blobber_review_emporium cmake-build-debug/assets RUN_DIRECTORY/review
# Focus only the exterior evidence if required; no repeat of gameplay checks:
OMP_NUM_THREADS=2 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 \
  cmake-build-debug/blobber_review_emporium cmake-build-debug/assets RUN_DIRECTORY/sign-review sign
```

GPU injection failure and cancel preserve the source city and exact resource
residency; successful prepare/commit/return follow the existing view/session
lifetime ordering. City+emporium overlap peaks at **456.79 MiB world GPU /
46.22 MiB world CPU**, application GPU peak **579.01 MiB**; emporium alone is
185.84 MiB GPU / 23.80 MiB CPU. Existing 1 GiB GPU / 352 MiB CPU ceilings are
unchanged. All texture/alpha leases return to zero after release; bounded shared
render buffers remain owned by the renderer until teardown. No claim covers
driver-internal allocations.

Seven FloorState slots total 1,814,400 bytes; the production active-emporium save
is 2,304,391 bytes, below 16 MiB. Production disk encode/decode/Continue restores
the location and exit pose, and leaving returns to the same west-facing street
cell. All [six selected captures](screenshots/emporium/README.md) were visually
inspected. The first sign shot clipped the top at (16,7); a sign-only follow-up
from (16,6) shows the whole emblem and replaces that evidence. No geometry, camera
or gameplay change was needed, and passing CPU/integration checks were reused.

Runtime movement/appearance acceptance remains user-owned. Merchant identity,
animations, dialogue, stock and scroll mechanics remain the next design slice;
this room intentionally has no fake service prompt or free inn rest. No desktop
input, benchmark, stress workload, extra build matrix or user-save edits occurred.
All runners are closed. Exactly six images were promoted; artifact-helper cleanup
was previewed and applied, removing 119 disposable files (202,678,736 bytes) of
raw exports, captures and logs. No diagnostic exception is retained. Packed
Blender/GLB and required texture/mesh sources remain durable assets.

## Herb-rack window clearance correction — 2026-11-07

The user requested moving the alchemy bench's hanging rail/bundles to screen left
so they do not cover the window. Shift this group by +.65 local X in the offline
exporter; keep the bench model anchor and its reserved cells. In the east-facing
view this places the rail at world Z=4.05..4.85, on the adjacent plaster bay;
the window opening remains Z=5.24..5.76. No map, camera, collision, actor, item or
save-identity change. Existing 16/19 campaigns remain compatible.

Regeneration exits successfully using the pack README's bounded null-audio
Blender command. Independent comparison identifies exactly 900 serialized
vertices (the rail and three bundled-herb groups) translated by +.65 X; every
other position and all normals/bindings are unchanged. Only the alchemy mesh,
packed Blender and GLB need promotion; atlas, floor/wall PNGs and other runtime
mesh files remain byte-identical. All 15,206 triangles retain valid winding and
metric UVs (maximum error .005655 texels, tolerance .015); the group stays inside
the original bench footprint.

Debug game/publication and focused review runner rebuild pass with `nice -n 15
ionice -c 3 cmake --build --preset debug --target blobber blobber_review_emporium
-j 2`. Publication remains 543 textures / 856,180,096 mip bytes. The hidden
`blobber_review_emporium ASSETS NEW_OUTPUT alchemy` mode passes on NVIDIA/OpenGL
with bounded threads. Entrance, wide (7,5) and close (8,5) east-facing bench views
were inspected: the full rack is left of the window and all window panes remain
clear. Texture/alpha leases release. No desktop input/focus automation.

Prior CPU and full travel/Continue verification is reused for this cosmetic
mesh-only adjustment; no gameplay suite or benchmark was rerun. Only entry/alchemy
gallery evidence is replaced, retaining six PNGs. Runtime correction acceptance
remains pending user review. All runners are closed; preview/apply cleanup removed
29 owned disposable files (16,946,823 bytes), including exports, logs and the unused
wide capture. No diagnostic exception is retained.

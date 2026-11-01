# Underkeep dressing

Status: **Implemented and verified; awaiting user visual review**.

The user requested [scale, moss anchoring and combat-facing corrections](underkeep_review_corrections.md)
on 2026-11-01. That follow-up supersedes the original chest instance size and
visibility identity below; the original delivery evidence remains historical.

The [prop surface revision](underkeep_props_revision.md) supersedes the original
mesh construction and UV mapping; see its before/after gallery for current art.

## Intent and art

The opening three stone floors become the Underkeep biome: an abandoned fortress,
with faded heraldry, damp masonry, neglected furniture and sparse imprisonment
remnants. Preserve discrete movement, prepared assets, fixed presentation,
encounter counts and reward quantities. No physical lighting, shadows, new
biome generators or additional torture furniture.

Use imagegen concepts before Blender modelling; retain concepts, prompts, Blender
sources, GLB interchange, export script and texture masters. Chest, empty cage
and rack each use one 256-square painted colour atlas, <=2000 triangles per
variant. Paint soft AO, joints, undersides and wear, without directional lamps.
Props/cutouts bypass existing face shading but keep distance dimming/fog. Terrain
keeps its accepted shading. Two corner webs, two bone scatters, two damp/moss
patches, cracks, soot, faded blood and two banners complete the first library.

## Ownership and preparation

WorldDefinition owns immutable bounded dressing records. Map owns and reconstructs
cached movement occupancy. Keep terrain solidity separate: furniture leaves a rendered floor,
blocks player/NPC movement and reach, and does not become a full-cell sight wall.
World asset packs bind prepared meshes and cutouts by stable IDs; Underkeep has
its own pack, with existing dungeon terrain and door sources reused. The offline
baker consumes narrow Blender-exported mesh data. GLB is interchange, not a new
runtime dependency. Shared immutable CPU assets and registry GPU leases retain
current preparation/rollback lifetimes. All scene/view storage is bounded and
allocated before steady frame work; variants are preloaded, not rebuilt on open.
CPU picking/occlusion and GPU drawing use identical triangles/alpha. Decorations
have no interaction IDs. Chests use world-scoped typed container handles.

## Placement and bounds

Dressing uses an independent seeded domain after fixtures/population/rewards.
Group details by context and leave bare stretches. Target 16–32 surface details,
2–5 webs, 2–5 bone scatters; <=64 architectural additions and <=2 large furniture
pieces per floor. <=32 candidates per furniture piece. At most one imprisonment
arrangement per floor; racks only at depth three. Pilasters, bases and doorway
surrounds follow actual boundaries/runs and stay within .12-cell perimeter relief.
Protect fixtures, door swings/approaches, stair rooms, spawn, room anchors,
passages, NPC cells, keys and reward access. Validate occupied-cell connectivity,
useful loops and key/guardian order. Reject optional placement rather than change
encounters/rewards. Mandatory chests reject a bad candidate; frozen fallbacks have
validated assigned sites. Respect existing 256-cutout/128-prop/384-record and
world/scene/material/resource budgets; fail preparation without replacing live
owners when a budget is exceeded.

## Containers and commands

LootWorld owns up to eight stable nonrecycled containers per floor, their opened
state and content membership. Two supply chests per floor replace generated
caches; depths one/two also have a sealed guardian reward chest. Contents and
quantities stay unchanged. Containers reserve one cell and a legal adjacent
interaction position in their reward region. Existing loose ground/death/party
loot stays bags. Checked GameSession commands open reachable chests only during
settled safe exploration, for free; opening immediately shows the existing loot
panel, filtered by container, and remains saved. Interaction key uses a directly
ahead chest after existing usable fixtures. Dedicated access permits the blocked
chest endpoint without enabling movement through furniture. Existing recipient,
capacity, Take/Take All, empty auto-close semantics remain; empty chests can be
inspected again. Guardian completion makes its chest available exactly once.
No reroll, regeneration, or discarded items on failed pickup.

Serialize placements, content membership and mutable container state through
travel/recall/Continue; reconstruct and validate derived occupancy. Bump affected schema/simulation/generation/publication
versions; reject older campaign saves unchanged. Character libraries remain
independent. Include dressing/containers in diagnostic reports, automation,
fingerprints and state digests without leaking undiscovered player content.

## Verification

Small owned fixtures cover deterministic dressing, protected cells, rejected
placement, movement/navigation, progression and both fallback profiles at all
depths. Check container open/sealed/empty/partial/full/stale/foreign behavior,
container/bag isolation and save restore including inactive floors/corruption.
Generated asset fixtures check geometry, UVs/bounds, alpha, modes and capacities.
Allocation guards cover decorated views and interactions. Use focused Release
checks, then one affected integration checkpoint and hidden GL/CPU-picking/resource
lifecycle verification, plus deterministic cardinal/mid-turn production captures
for both layout profiles. No OS input/focus automation. At most two build workers,
one intensive job, low priority. Interactive visual acceptance remains user-owned.

## Implementation and evidence

The following records the delivered implementation and review evidence.

### Final responsibilities and interfaces

- `BiomeId::Underkeep` retains numeric ID zero; `underkeepBiome()` selects its own
  `underkeep` pack. Algorithm `StoneRooms` and its two profiles remain. Generator /
  catalog are **9/7**; established topology/interior/encounter/reward/runtime seed
  derivations explicitly retain random revisions **8/6**. Only `Domain::Dressing`
  uses the new content identity, so the accepted room layouts/populations are not
  reshuffled by this art delivery.
- `dressUnderkeep` is a preparation-only pass over a validated layout. It reserves
  fixture approaches, passage cells, protected stair areas, room anchors, NPC cells
  and existing reward access, then places caches and optional furniture. Each
  proposed blocking placement runs bounded flood connectivity. Preserved passage
  paths retain the validated useful loops and key/guardian separation. Optional
  furnishing can be omitted; mandatory cache failure rejects the candidate. Both
  frozen spatial fallbacks use this same bounded admission and publish explicit
  checked cache sites. They do not run an unbounded fallback search.
- `FloorLayout` owns up to three `CacheSite` values and the complete prepared
  world fingerprint. The reward resolver maps the existing six supply stacks and
  two nonfinal guardian camp kits to these sites, without adding items. Guardian
  chests sit in the guardian chamber and unlock when its objective completes;
  their old metadata reward point remains a protected progression access point.
- `WorldDefinition::decorations` owns stable kind/cell/facing/size records plus
  optional container index/seal definition. Original four decoration IDs remain
  stable. `Map` reconstructs one byte of movement occupancy per cell on preparation
  and validates footprints/attachments/duplicates. `movementBlocked` is separate
  from terrain/sight `solid`; floors remain rendered under furniture. `canStep`,
  runtime NPC admission and reach respect this occupancy. Maps retain at most
  256 cutouts / 128 props / 384 total records. On generated 36-square maps the
  occupancy adds **1,296 bytes**, included in diagnostic memory accounting.
- `LootWorld` owns eight fixed `LootContainer` records and stack membership.
  `ContainerId` uses world + nonrecycled slot, with invalid/foreign IDs rejected.
  `GameSession::containerAccess/openContainer/nearbyContainer` implement settled,
  conscious-party, front-band, height and corner access checks. Opening is immediate
  and free; no animation timeline or physical lockpicking system was added.
  Loot panels filter by owner as well as cell. Empty containers persist and remain
  inspectable; floor bags keep their existing lifecycle and collection behavior.
- Campaign schema/simulation are **7/11**. Definitions, cache sites, remaining
  stacks and opened/sealed state round-trip through active/inactive floors and
  are validated before use. Derived occupancy is reconstructed, not redundantly
  serialized. Digests include container state; world and generation fingerprints
  include dressing. Old campaigns reject without rewriting; character libraries
  remain independent.
- Automation accepts `{"kind":"open_container","container":0}` with the normal
  host session/revision tokens. Preview does not mutate. Observations include
  visible container slots/positions/opened/sealed flags and stack owner identity;
  unopened contents stay hidden in visual mode. Diagnostic mode retains its
  explicit full-information policy. Visibility identity is **world-visibility-v8**.

### Asset and scene data

Prepared publication format is **4**, appearance revision remains **4**. Pack
bindings select the two chest variants, cage and rack, and named cutout artwork.
The narrow triangle-list export stores position, unit normal and authored UV in
contiguous eight-float vertices. Loading checks finite data, UVs, winding, texture
ownership and each variant's movement footprint; at most 16 meshes, 6,000 vertices
(2,000 triangles) per mesh and a 2 MiB metadata envelope. Atlas images remain in
existing texture/mipmap/alpha publication storage and its budgets.

Scene preparation transforms models into the existing chunked mesh, retaining
both chest variants with conditional ranges and container picking IDs. Opening
selects ranges from a fixed snapshot, without reuploading/rebuilding geometry.
Chunk horizontal bounds include two-cell furniture extending across a chunk edge.
The bounded draw list includes the additional prop/variant ranges. Painted ranges
use authored UVs and shader mode 4, bypassing face-brightness modulation while
retaining dimming/fog; terrain keeps the original shading. CPU/GPU picking use the
same geometry and decal alpha. Transparent web planes span real corners; floor
art receives small shallow chips, not upright character-style cards.

The initial final export has **192 triangles per chest variant, 672 for the cage,
396 for the rack**. All three props have a 256-square atlas, derived from the
retained imagegen colour master with painted seams, dark interiors/undersides and
wear. No runtime AO computation or real light was introduced. Procedural plain
piers, wall bases and doorway jamb/lintel assemblies use existing stone materials.
Banners favor halls, optional rooms and guardian chambers; bone scatter groups
near furnishings as well as sparse room scatter. The one small old blood stain is
restricted to depth three. One optional cage/rack arrangement per floor is the
current implementation, below the approved two-piece cap.

Source assets and prompts: `assets/packs/underkeep/`. Regenerate authoring exports
with `nice -n 15 ionice -c 3 blender --background --factory-startup -t 2 --python
tools/export_underkeep.py`. The editable scene and combined named-node GLB are
`models/underkeep.blend` and `models/underkeep.glb`; `.mesh.json` files are the
baker inputs. The image tool rejected the empty rack concept; its failed prompt is
retained and that model follows the written empty-furniture design. Architecture,
chest and cage concepts and all colour/cutout sources used the built-in imagegen
workflow, with no CLI/API fallback. Blender's connected process crashed during
repeated temporary-scene GLB exports. A single combined export in a headless
Blender process recovered the complete sources without desktop automation.

### Verification record

The focused integration checkpoint passed **16/16 in 12.07 seconds** (Release):
dressing, recovery, automation allocations, fixture automation, generation,
generation automation/CLI, population, guardians, full campaign save, campaign
save CLI, world CPU, Underkeep rendering, world visibility/CLI and environment.
New checks cover deterministic placement, both fallback profiles at all depths,
footprints/protection, closed/open/sealed/empty/partial/full/stale/foreign container
behavior, source ownership, active/inactive save continuity, corrupt state,
prepared mesh/UV/capacity contracts and allocation-free engine-owned C++ work.

The extended save test was interrupted once after its existing unbounded test
turn helper met a live encounter on a new persistence fixture's return route.
The helper is now bounded, the persistence-only fixture clears that route, and
the full check passed in **5.52 seconds**. Earlier outdated floor-bag assumptions
and fixture publications/path helpers were updated to explicit containers,
owned miniature model assets and furniture-aware stepping. These are test fixture
changes, not combat or reward rebalancing.

The first hidden production GL review passed CPU/GPU interior ID comparisons,
container picking, draw-error checks and resource return to the Presenter baseline.
Final asset-bounds checks/captures and their gallery are recorded below when complete.
C++ allocation guards do not claim to intercept C allocation or driver internals.
No benchmarks, broad seed/balance studies, sanitizer/Debug matrix or OS input were
run. Interactive play and visual acceptance remain with the user.

### Final handoff

After the last mesh-footprint/draw-capacity validation correction, the five affected
checks passed **5/5 in 0.83 seconds**: world CPU, visibility, prepared Underkeep
rendering, generation automation (including wire container preview/bounds), and
C++ allocation guards. The single-configuration offline publication fixture also
passed; it uses small generated images and the Release baker, not a second engine
build configuration. Final Release application build succeeded with two workers.

Final hidden capture review passed for **19 views**, seed 0 at depths 1 and 3,
covering both profiles, wall relief, a faded banner, moss/damp, webs/bones, cage,
rack, closed/open/emptied chests and turns. It checks GL errors, visible chest IDs,
CPU/GPU interior picking (under 100 nonedge mismatches per 500×282 view), and GPU
lease cleanup. The renderer reported 60,488,291 CPU / 352,749,736 GPU bytes after
view teardown; these are retained shared storage estimates, not process/driver
peak memory. Publication has 401 textures, 348,236,672 mip bytes and 60,185,701
alpha bytes, within existing limits.

[Review gallery and reproduction](screenshots/underkeep/README.md) includes native
1920×1080 images, diagnostic footprints, matching generation fingerprints and
prepared metadata identity. Concepts/source art/models are linked there. Earlier
camera-placement QA captures were moved outside the repository. Room shots use
real populations; isolated chest-state views omit NPCs/fixtures so checked free
interaction can be demonstrated without pretending to have played a combat route.

Focused integration command (16 checks, 12.07 seconds):

```sh
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-release \
  -R '^(dressing|underkeep_render|generation|population|guardians|fixtures_automation|generation_automation|generation_cli|campaign_save|campaign_save_cli|recovery|world_cpu|world_visibility|world_environment|automation_allocations|world_visual_cli)$' \
  -j1 --output-on-failure
```

The design is delivered, not marked visually accepted. No interactive desktop
review, gameplay/balance route study, physical lighting or further biome work was
performed. Old campaigns need a new run; character libraries are unchanged.

# BR02: finished room kit and assembled review

Status: **Furniture proportions and room dressing implemented and mechanically verified;
awaiting user review**, 2026-11-09.
The user accepted the revised fireplace, rejected the remaining flattened furniture,
and approved the implementation plan below. Earlier verification is historical.
The user requested implementation of the detailed plan and chose pair-first review.
Stop after delivering the finished kitchen/mead hall in-engine review; complete
barracks/armory only after that checkpoint. BR03 remains separately proposed.

## Design and ownership

### Furniture proportions and room dressing

Approved implementation: preserve the fireplace, floor areas and camera; rebuild
supports to put dining tabletop/bench seats/kitchen worktop at .28/.17/.32 cells.
Translate tabletop objects without stretching them. Remove the provisions pallet;
independent crates/sack grow 14%, barrels grow 14% in diameter and become 1.5 diameters
tall. Five nonblocking storage placements: large crate (1,0) and barrel (3,0) north;
small crate (0,3) and barrel (0,4) west; sack (1,4) south. Two .65×.9 heraldic
hangings begin .65 above the north hall floor at (1,0)/(5,0). Two blocking 1×1
braziers occupy (0,0)/(6,0), .30-wide bowls with .36-high rims, animated with existing
flame artwork. Woven burgundy/ochre heraldry uses retained imagegen masters.

Kit and SceneryPlacement gain default-true `blocking`. Visual validation bounds
remain independent of movement/NPC/reservation occupancy; all placed geometry is
still checked for bounds/terrain/other placements. Kit gains optional local fire
anchor, transformed with room/model and floor elevation. Runtime WorldDefinition
retains legacy hearth plus a preparation-owned vector of at most two additional
AmbientFire records. Map rejects excess/nonfinite/out-of-map anchors; frames read
the stable collection with per-fire visibility and at most 24 particles per fire.
No per-frame allocation, spawning, new lighting or interactions. Residency requests
include flame artwork when the map has fires. Persistence/defaults and hash extension
tags preserve old content identity when these optional additions are absent.

Keep existing bounded kit/placement/scenery/mesh/atlas/effect capacities; no generic
entity framework or runtime scaling. Recompute metric UVs on final geometry. Keep
all sources/editable models reproducible and fireplace geometry/material appearance
unchanged. Update blockouts for new forms. Room, lane, socket and reward contracts
remain; barracks/armory and campaign integration stay deferred.

Verification uses owned fixtures for decorative collision in four rotations, visual
bounds/overlap/attachment, two-fire visibility/caching/pause/invalid capacity, asset
residency, persistence compatibility and allocation-free frames. Build affected
Debug suites with configured CMake/Ninja, <=2 workers and one intensive job. Inspect
front/side/underside model views and hidden in-game close/overview/turn captures;
explicitly assess proportions, barrel silhouettes, separate storage and hall art.
Passing technical checks do not establish visual acceptance. Confirm traversal of
storage cells, doors/rewards, GL/picking/lease cleanup; retain six refreshed images,
clean owned temporary output, and deliver `./review-rooms` directly to the user.

### Scale and hearth correction

The user's review requests approximately 50% smaller furnishings, a coherent
hearth interior, matching wall materials and more geometric detail. This supersedes
the four pair-stage kit dimensions in the initial BR01/BR02 handoff. Author the
four models at half their initial linear dimensions, baked into source vertices;
runtime instance scale remains 1. Collision becomes 1×1 for hearth/work/provisions
and 1×2 for table-and-benches; height caps become .5/.175/.225/.2 respectively.
Whole-cell movement still requires rounding fractional furniture depth upward.
Wall-backed pieces sit against the back edge of their cell. Move kitchen hearth
to (0,0), kitchen/armory work-surface to (4,0), and right hall table to (6,1),
preserving corner/wall composition while releasing the old occupied cells.
Sockets, room footprints, ceiling profiles and reserved encounter/reward lanes
stay unchanged. Later unbuilt barracks/armory kits remain at their old contracts
until the pair's revised scale is reviewed.

Rebuild the hearth with fitted chamfered masonry, a shallow stone arch, lined
back/side surfaces, ash bed, supported grate, and rounded thick-rimmed pot with
handles/feet. Sample the existing dungeon wall artwork for the stone palette and
detail. Increase useful geometric detail within the existing prepared-mesh limits;
the earlier crude silhouettes were not an intentional pixel-art requirement.
Keep the existing atlas size, native texel density and unlit material pipeline.
UVs are computed after the half-size transform so texture detail does not double
in density. Validate the smaller blockout forms as well as finished models.

Focused checks: owned compact-kit transformed bounds/collision fixtures, full
authored socket/rotation validation, one Debug blueprint/render/scenery/fixture
selection, final production bake and matched hidden pair/90° captures. Inspect
the hearth at adjacent and overview distances and in an emission model view.
Replace superseded pair evidence, record limitations, and clean the owned run.

### Original pair design (superseded dimensions above)

Preserve all [accepted BR01 contracts](br01_blueprint_rooms_design.md): immutable
blueprints, whole-cell scenery occupancy, native pivots/scale, rooms, sockets,
rotations, encounter/reward reservations and ceiling profiles. Runtime receives
ordinary maps and scenery. No campaign generation, population, save version,
renderer schema or frame-time interpretation changes.

Author eight independent kits using imagegen concepts/material masters and a
dedicated reproducible headless Blender exporter. First checkpoint: hearth
(2×1×1), work-surface (2×1×.35), provisions (2×1×.45), table-and-benches
(2×3×.4). Later: sleeping-bay (1×2×.6), footlocker (1×1×.2), weapon-storage
(2×1×.8), armor-display (1×1×.8). Bounds are width×depth×height in cells;
origin is anchor-cell floor center, +X right/+Y up/+Z backward. No overhangs.
Hearth is cold; furnishings are static/decorative. Existing doors, pillars,
terrain and chest variants retain their owners and bindings.

Use Underkeep's named model bindings and existing scenery transforms, painted
triangles, picking, mips, scene preparation and CPU/GPU leases. Keep the four
legacy furniture slots unchanged. Review assembly gains an explicit pack
argument, defaulting to blueprint-blockouts; launcher gains `--art blockout|finished`
with compatible blockout default. Finished selects underkeep and checks every
referenced kit; missing/unbuilt rooms fail contextually, never fall back silently.
Preparation validation reads model payloads temporarily and verifies the
complete grouped bounds against the kit contract before starting the view.
Offline export validates the same bounds before registering files.

## Artwork and bounded preparation

Follow [the accepted prop guide](prop_modelling_guide.md): true thickness, fitted
joints, restrained grain, dull iron, dark recesses, painted shading and emission
preview. Occupied cells must look occupied; grouped tables/benches and storage
bases must not suggest traversable holes. No artificial blockout-color plates.

Maintain 256 texels/runtime cell with four-pixel gutters, checking both UV axes
and area. Each kit is one mesh of at most 2,000 triangles. First four kits share
one 2048² opaque atlas; reserve a second for the remaining kits. Full added mip
budget is about 42.7 MiB. Packing fails instead of rescaling, and existing
scene/resource/count limits remain unchanged. All authoring and validation
allocations occur outside frame processing. Retain prompts, concepts, texture
masters, scripts, packed blend, GLB and mesh JSON under durable source paths.

## Review and verification plan

Use the existing isolated launcher through GameSession/GameView/WorldRenderer/
Presenter. Add deterministic close/turn views and report art mode, publication
identity and resident counts/bytes. Review both rooms, their shared doorway,
floor attachment, ceiling clearance, door sweeps, reward access and table aisles.
Use hidden application-owned captures and in-process commands only; user operates
interactive walk. At pair checkpoint deliver selected captures and a walk command.

Extend owned small fixtures for pack selection, missing bindings, transformed
footprints/heights, invalid prepared data, lifetime/rollback and zero-allocation
CPU frames. Run focused Debug blueprints, blueprint_render, scenery, fixtures,
underkeep_render and world_residency tests plus a generated publication fixture.
At final BR02 checkpoint run generation regression once. Production captures
check GL errors, CPU/GPU IDs and cleanup; fixtures never depend on production art.
No benchmark, broad seed sweep, sanitizer/configuration matrix or desktop input.

Use configured CMake/Ninja, <=2 workers and one intensive job, low CPU/I/O
priority and bounded Blender threads. Own disposable output in an agent run,
promote <=12 selected images while awaiting review (<=6 at closeout), preserve
BR01 acceptance, then preview/apply cleanup. Record executed commands, results,
limitations, review status and any implementation changes below.

## Implementation and reproduction

User launch correction: the repository-root `./review-rooms` command opens the
finished kitchen/mead hall with normal controls. It resolves paths automatically,
updates only its required Debug targets using the configured CMake/Ninja, and
owns/cleans its temporary walk output. `--check` performs the same build and
headless content validation without opening a window; `--combat` adds the preview
encounter. The longer commands below remain developer reproduction details.
Launcher `--check` passed against the configured Debug build and finished catalog;
no interactive window was opened by the agent. Builds use one worker so asset
preparation jobs stay sequential. The launcher-change artifact run was cleaned.

Nine pair-stage kits are registered as Underkeep named models: hearth 1,794 triangles,
work-surface 1,496, table-and-benches 1,176, barrel 436, each crate 528, sack 174,
tapestry 260 and brazier 596. The obsolete grouped provisions model is removed.
The shared 2048² atlas uses 649 packed rows; its full mip chain costs
22,369,620 bytes (21.33 MiB). One atlas remains reserved for the later kits.
No production map, generation, save identity, resource ceiling or renderer format
changed. Adding unused pack bindings does increase the prepared Underkeep palette;
resident usage is measured in the hidden review rather than inferred from total
installed publication size.

`blueprintCatalog` is an optional source pack field restricted to
`blueprints/NAME.json`, included in build dependencies and both provenance scans.
The baker checks each registered kit, allowing the pair-stage subset. Finished
walk/capture validates every used binding before opening a window; finished
`validate` checks the selected room's published art in addition to the full catalog's
geometry variants. Default blockout assembly and launcher behavior remain compatible.
There is no runtime fallback for missing finished art.

New validation lives in `src/assets/blueprint_art.*`. The publisher retains ordinary
finite/normal/winding/UV validation; kit validation adds exact bounds, opaque/static
mode, single-part grouping and the 2,000-triangle cap. The exporter independently
checks positions, per-axis UV density and triangle count. Allocation and ownership
are unchanged: temporary reads end before scene startup, views own the existing leases.

Configured tools for this checkout are `/home/bison/clion/bin/cmake/linux/x64/bin/cmake`
and `/home/bison/clion/bin/ninja/linux/x64/ninja`; reread the cache if using another
build tree. All commands below run from the repository root.

```sh
BR_RUN=$(python3 tools/agent_artifacts.py create)
nice -n 15 ionice -c 3 /home/bison/clion/bin/cmake/linux/x64/bin/cmake \
  --build --preset debug --target blobber_review_blueprints blobber_bake_world_assets

cmake-build-debug/blobber_review_blueprints validate --art finished \
  --room kitchen-hall --prepared cmake-build-debug/assets/prepared

# User-operated interactive review; no campaign save owner.
cmake-build-debug/blobber_review_blueprints walk --art finished \
  --room kitchen-hall --catalog assets/blueprints/underkeep.json --assets assets \
  --prepared cmake-build-debug/assets/prepared \
  --ui-prepared cmake-build-debug/assets/prepared --output "$BR_RUN/walk"

# Hidden, application-owned images; no OS input or focus control.
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_blueprints capture \
  --art finished --room kitchen-hall --assets assets \
  --prepared cmake-build-debug/assets/prepared \
  --ui-prepared cmake-build-debug/assets/prepared --output "$BR_RUN/pair"
```

`--rotation 90` checks the rotated pair; `--combat` adds the existing fixed
goblin/archer encounter. `--room kitchen` and `--room mead-hall` work independently.
Finished barracks/armory intentionally reject missing kits until the review gate
is passed; blockout mode remains available through the BR01 separate publication.

Rebuild authoring outputs into a new run subdirectory, inspect them, then copy
selected source outputs to the pack before baking. Never regenerate into a capture
gallery. The authoring script does not update the pack manifest automatically.

```sh
nice -n 15 ionice -c 3 timeout -k 5s 30s blender --background --factory-startup \
  -noaudio -t 2 --python tools/export_blueprint_rooms.py \
  -- --output-root "$BR_RUN/authored"
nice -n 15 ionice -c 3 timeout -k 5s 30s blender --background --factory-startup \
  -noaudio -t 2 --python tools/review_blueprint_rooms_blender.py \
  -- "$BR_RUN/models"
```

Blender in this environment can stall in PulseAudio shutdown after successfully
writing every output, even with `-noaudio`. A timeout/interrupt is not a successful
process exit: confirm the completed export report, output files and independent
published geometry checks. Only task-owned Blender processes were stopped. Sandbox
thumbnail/config cache writes also fail harmlessly; all retained assets are in the
workspace and all textures are packed in the editable/interchange models.

## Furniture and room dressing verification

Implementation uses one matched heraldic tapestry design for both hangings. Its
[built-in imagegen master](../assets/packs/underkeep/source/rooms/heraldic-tapestry-master.png)
and [exact prompt](../assets/packs/underkeep/source/rooms/tapestry-prompt.json) are retained.
All nine models, the packed Blender/GLB and the atlas are reproduced by the exporter.
The accepted hearth's vertex positions and normals compare exactly with the preceding
revision; only atlas UV placement changes. No atlas or mesh capacity was raised.

Model review exported 27 front/rear/underside views. Inspected work surface, table,
barrel, tapestry and brazier fronts, table rear, and work/table/barrel/brazier undersides.
The brazier underside revealed an open outer bottom; added its cap, regenerated sources
and inspected the corrected underside. As documented below, Blender completed every
export/render but stalled during audio shutdown; bounded timeouts closed owned processes.

Configured Debug targets built with at most two workers. Seven affected suites pass
serially in 3.64 seconds: blueprints, blueprint_render, scenery, fixtures,
world_integration, world_residency and campaign_save_smoke. Tests cover decorative
footprints and local fire transforms in every orientation, visual-overlap rejection,
malformed/capacity rejection, two-fire animation/pause/independent culling and
allocation-free frames, explicit flame residency, and persistence of decorative scenery
and multiple fires. Existing legacy hearth behavior and ordinary save round trips pass.
The full slow campaign suite, second configuration and benchmarks were not run.

```sh
nice -n 15 ionice -c 3 /home/bison/clion/bin/cmake/linux/x64/bin/ctest \
  --test-dir cmake-build-debug \
  -R '^(blueprints|blueprint_render|scenery|fixtures|world_integration|world_residency|campaign_save_smoke)$' \
  -j 1 --output-on-failure
```

The default `blocking=true` and absent additional fire collection do not append hash
extensions. Sparse persistence inherits these defaults for missing fields, and fire
collections reject sizes above two before resizing. No save version or generated-world
identity changed, and no user saves were read or written. Current catalog geometry
passes 192 room variants and four pair orientations.

Final serialized UV density is 255.9911–256.0113 texels/cell. Sampled triangle-centroid
colors on the hearth match the accepted atlas exactly after repacking. Final production
bake succeeded: 556 textures, 883,633,616 mip bytes and 64,967,422 alpha bytes;
publication fingerprint `12382331285521188286`. Finished catalog bindings pass.

Two hidden runs (0°/90°, 21 views each) pass all 42 nonedge CPU/GPU ID comparisons,
GL checks, door traversal, reward access, explicit movement into decorative storage,
injected GPU acquisition rollback and complete view/renderer lease cleanup. Both
flame frame pairs change visibly (32,814/18,837 viewport pixels after .15 seconds).
Inspected kitchen overview/work/storage/barrel, table front/end/turn, tapestry,
brazier close-up, and hall composition in both orientations. Taller supports now
read clearly at the unchanged game camera height; barrel bodies are upright.

Each pair holds 51 textures: 114,800,228 mip bytes, 12,613,249 alpha bytes and
797,184 source-mesh bytes. Renderer CPU accounting is 23,617,772 bytes; world GPU
141,388,604, peak application GPU 294,785,928. Scene CPU/upload are
1,755,704/1,754,928 bytes in both orientations. No limits were raised; counts exclude
process-wide/driver memory. Fire uses existing artwork, with no new dynamic lighting.

`./review-rooms --check` passes with the final Debug executable, production assets,
portrait and creator caches ready. Interactive desktop input/play remains unverified;
no user window was controlled. The [six refreshed review images](screenshots/blueprint-rooms-br02/README.md)
retain current evidence while explicitly preserving fireplace and historical BR01
acceptance. Barracks/armory remain deferred. All owned runners are closed; the owned
run was preview-cleaned and removed after promotion, with no diagnostic exception.
Source artwork/editable models and normal build caches remain.

## Previous scale/hearth revision verification (fireplace accepted; furniture superseded)

All four source meshes now bake in the 50% linear-size correction. The hearth
has nine arch stones, fitted masonry courses, a lined firebox, ash floor, a
supported seven-bar grate and a 24-sided curved pot with rolled lip and handles.
Timber members have small geometric chamfers. Twelve emission previews were
exported; hearth front/rear/underside and the other three front views were inspected.
The Blender processes wrote their reports and outputs but required bounded shutdown
termination for the PulseAudio stall described above.

Configured Debug review/baker and four affected test targets built with two workers.
The focused `blueprints|blueprint_render|scenery|fixtures` selection passed serially
in 0.81 seconds. Its owned fixture checks that reduced footprints release the former
blocked cells for both party and NPC movement in all four rotations. Compact generated
blockouts and finished-mesh bounds use the same catalog contracts.

```sh
nice -n 15 ionice -c 3 /home/bison/clion/bin/cmake/linux/x64/bin/ctest \
  --test-dir cmake-build-debug \
  -R '^(blueprints|blueprint_render|scenery|fixtures)$' -j 1 --output-on-failure
```

Serialized UV Jacobian checks now measure 255.9829–256.0086 texels/cell; exporting
eight decimal places preserves tiny bevel-face UVs. Unrounded authoring checks
retain .001 tolerance. The four triangle counts are recorded above; no atlas,
mesh, scene or residency capacity was raised. The publication fixture and unrelated
renderer/residency suites from the initial delivery were not repeated: publisher,
resource ownership and runtime rendering are unchanged by this correction.

Final production bake succeeded using the configured CMake with one worker.
Publication fingerprint is `10468492512477437446`: 556 textures, 883,633,616 mip
bytes and 64,967,422 alpha bytes. Finished validation passes 192 room variants,
four pair orientations and every selected art binding. `./review-rooms --check`
passes with no further build work, so the normal walk command uses these assets.

Two hidden production runs (pair 0° and 90°, 15 views each) passed all 30 picking
comparisons with zero nonedge differences, GL checks, door traversal, reward access,
injected GPU preparation rollback and lease cleanup. Inspected both hearth close-ups,
room overviews, work/provisions and the table close/turn views. The reduced furniture
leaves more open floor inside the unchanged room dimensions, as expected.

Current empty-pair accounting: 114,669,160 resident mip bytes, 12,588,673 alpha,
710,784 source-mesh bytes, 23,135,629 renderer CPU bytes, 140,889,280 world GPU bytes
and 294,286,604 peak application GPU bytes. Scene CPU/upload are
1,387,448/1,386,672 bytes in both orientations. This includes the extra geometry
without changing existing resource limits; it excludes driver/process-wide memory.

[Six refreshed images](screenshots/blueprint-rooms-br02/README.md) replace the initial
eight. User visual acceptance and interactive play remain pending; no campaign,
benchmark or new configuration was run. Barracks/armory remain behind the pair
review gate. All task-owned runners are closed and disposable run output was
cleaned after promotion, with no diagnostic exception. Durable sources and normal
Debug caches remain.

## Initial delivery verification (artwork subsequently rejected)

Configured Debug targets built with two workers. Six focused suites passed in
1.03 seconds: blueprints, blueprint_render, scenery, fixtures, underkeep_render,
world_residency. Owned fixtures cover all kit orientations/raised-floor anchors,
art selection preserving collision, missing pack/model, height/footprint/triangle
failures, invalid UV/normal rejection, lifecycle/rollback and allocation-free CPU
frames. No second configuration, benchmark or broad generation sweep ran.

```sh
nice -n 15 ionice -c 3 /home/bison/clion/bin/cmake/linux/x64/bin/ctest \
  --test-dir cmake-build-debug \
  -R '^(blueprints|blueprint_render|scenery|fixtures|underkeep_render|world_residency)$' \
  -j 1 --output-on-failure
nice -n 15 ionice -c 3 env TMPDIR="$BR_RUN" python3 tests/blueprint_art_bake_tests.py \
  cmake-build-debug/blobber_bake_world \
  --cmake /home/bison/clion/bin/cmake/linux/x64/bin/cmake \
  --ninja /home/bison/clion/bin/ninja/linux/x64/ninja
```

The optional generated bake fixture passes: one tiny actor and kit, no production
artwork, no-op publication, catalog dependency invalidation, mesh/height rejection
and preservation of installed payload/metadata on failure. Its initial encounter
fixture was too small for BR01's 2×2 contract; corrected to a small 4×4 room and
reran only that check. The older broad world-build script was not run.

The first hidden pair review passed 14 views with zero nonedge CPU/GPU picking
differences, GL checks, door traversal, chest access, injected preparation rollback
and lease cleanup. Emission review of twelve front/rear/underside model views
exposed a coplanar overlap at the hearth's back/jamb seam; the exporter now fits
the back between the jambs. Source outputs and the publication were regenerated.
No layout/ceiling/collision changes were needed.

### Initial pair checkpoint

The corrected hearth's three emission views pass visual inspection. The other
nine original emission views remain applicable. Serialized final mesh UV Jacobian
checks measured 255.667–256.179 texels/cell on both axes (tolerance .5 texels/cell
for six-decimal serialized UVs on tiny faces). Export-time unrounded checks use
.001 tolerance. All four model bounds and winding passed the production baker.

Final publication: **556 textures, 883,633,616 mip bytes, 64,967,422 alpha bytes**;
fingerprint `2318549320953968193`. Total installed size includes unused other packs
and actors; it is not active GPU residency. Four final hidden runs passed:
pair 0° (14 views), pair 90° (14), standalone kitchen (8) and fixed pair combat (1).
All **37 views** have zero nonedge CPU/GPU ID differences and no GL errors. Both
room reward-access positions and the shared/exterior doors work. GPU acquisition
failure preserves the active image and byte counts. View teardown releases
texture/model leases; renderer teardown returns to the Presenter baseline, and
font accounting returns to zero before registry/window destruction.

| Measured bytes | Empty pair | Pair with goblin/archer |
| --- | ---: | ---: |
| Resident mip textures | 114,669,160 | 167,884,072 |
| Resident alpha | 12,588,673 | 22,583,929 |
| Resident source meshes | 388,032 | 388,032 |
| Renderer CPU accounting | 22,162,029 | 32,158,885 |
| World GPU accounting | 140,238,448 | 193,453,360 |
| Peak application GPU accounting | 293,635,772 | 346,850,684 |

Pair scene storage is at most 736,616 CPU bytes (90° case); mesh upload is 735,840
bytes. These remain below existing 64 MiB CPU/40 MiB GPU scene limits, 352 MiB world
CPU, and the current source-defined 928 MiB mip/96 MiB alpha/1024 MiB world GPU
ceilings. No capacity was raised. Accounting excludes driver/process-wide memory.

The sandbox's initial hidden capture could not open X11; authorized display access
was used for the successful hidden runs. No interactive desktop play, focus/input
automation, full campaign run, save migration, benchmark or second configuration
was performed. Generation regression is deferred to the final BR02 checkpoint as
planned; generation and campaign source code are unchanged.

The initial eight native images were presented for user review and have since been
replaced by the scale/hearth revision in the [current gallery](screenshots/blueprint-rooms-br02/README.md). **Next action: review kitchen/mead hall proportions, materials and
circulation readability; incorporate feedback before authoring barracks/armory.**
BR02 overall and BR03 are not marked complete or accepted. All runners are closed;
owned disposable captures/logs/exports were cleaned, with no diagnostic exception.
Durable art/prompts/models and the normal Debug prepared build cache remain.

After future review runs, close the launcher before cleaning:

```sh
python3 tools/agent_artifacts.py clean "$BR_RUN"
python3 tools/agent_artifacts.py clean "$BR_RUN" --apply
```

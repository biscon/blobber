# BR02: finished room kit and assembled review

Status: **Scale/hearth revision implemented and mechanically verified; awaiting
user review**, 2026-11-09. Initial pair artwork was rejected. Historical verification
below is explicitly separated from the current correction.
The user requested implementation of the detailed plan and chose pair-first review.
Stop after delivering the finished kitchen/mead hall in-engine review; complete
barracks/armory only after that checkpoint. BR03 remains separately proposed.

## Design and ownership

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

Four pair-stage kits are registered as Underkeep named models. Their source meshes
contain 1,794 hearth, 1,496 work-surface, 1,622 provisions and 1,176 table-group triangles.
The revised shared 2048² atlas uses 497 packed rows; its full mip chain costs
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

## Scale/hearth revision verification

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

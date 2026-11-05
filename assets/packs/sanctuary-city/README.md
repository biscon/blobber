# Sanctuary city — SC03 dressing

Original painted architectural kit for the development-only
`--sanctuary-city-review` town. Normal campaign Sanctuary remains the courtyard.

Ten named models provide low/gabled and hipped homes, a two-storey home, timber-
framed inn, masonry smithy with broad chimney, shuttered provisioner, sanctuary
with bell-cote, windowless dungeon entrance, paired closed gate and boundary wall.
Six residences plus four service/landmark buildings, dungeon and gate use 91 total
placements, including the complete modeled enclosure. Services and travel are not
enabled. SC03 adds the accepted courtyard tree, a smaller eastern tree, three
planted beds, bushes, two ivy facade variants, two cloth-canopied stalls, a low
stone fountain and tankard/anvil/provisions signs. Eight painted water phases
advance at 8 FPS on fixed opaque geometry using the paused gameplay clock.

## Regeneration

Run sequentially from the repository root, with at most two workers:

```sh
nice -n 15 ionice -c 3 timeout -k 5 120 env ALSOFT_DRIVERS=null OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 blender --background --factory-startup -t 2 --python-exit-code 1 --python tools/export_sanctuary_city.py
nice -n 15 ionice -c 3 cmake --build cmake-build-debug --target blobber blobber_review_sanctuary_city -j 1
```

`tools/city_architecture.py` owns the reusable offline geometry;
`tools/city_materials.py` owns extraction, padding, bounded atlas packing and mesh
splitting. The exporter produces packed Blender source, deterministic unlit GLB,
runtime mesh JSON, atlas PNGs, terrain textures, pack bindings and
`models/export-report.json`. Blender is not required to build or run the game.
The ordinary CMake publication consumes the checked-in runtime exports.
The full exporter also runs `tools/export_city_tree.py` and
`tools/export_city_dressing.py`. For tree-only iteration,
run that script with the same bounded Blender command. It preserves architecture
outputs and produces separate packed `courtyard-tree.blend`, unlit GLB, two runtime
mesh parts and `tree-export-report.json`. The original generated foliage and bark
masters and exact built-in ImageGen prompts are in `source/tree-art.prompt.json`.
Tree foliage uses 20 fixed double-sided cards; the trunk/branches are modeled.
The accepted tree geometry stays at (4,4), now inside the 3×2 courtyard garden
base. Geometry overhangs remain nonblocking.
Cutout threshold is 128/255, with prepared premultiplied mips/base alpha masks.
The user accepted the shorter tree before dressing expansion. Complete SC03 still
requires final user review. `tools/export_city_dressing.py` can regenerate only
dressing; `tools/city_mesh_export.py` retains packed Blender/unlit GLB collections.
The six new generated art masters and exact prompts are retained in
`source/dressing-art.prompt.json` (built-in ImageGen, including the ivy alpha fix).
The shared opaque atlas is 2048², cutouts 1024², water 2048×1024 with 16-pixel
frame padding. Planting reuses the approved foliage artwork; signs use generated
painted emblems. Water ripple/glint phases are prepared offline animation effects.
Variant bindings share existing source parts; the baker publishes them once per pack.

Model units are runtime cells, with no instance scaling. Blender uses Z-up and
runtime/GLB Y-up; source/GLB display the models in a grid. Footprints retain the
accepted low-house 3×3 and tall-house 3×4 conventions. Gate footprint is 5×2, with
local offset (-2,0); wall modules cover one cell. All geometry stays inside +/-4
horizontal cells and 0–4 vertical cells. Runtime model parts are capped at 6,000
vertices, eight parts and 24,000 vertices per model. Current dressed models use one to four parts, split by material ownership.

## Materials

The retained original `source/material-master.png` and `prompts.json` are the
accepted ImageGen source. SC02 reuses this artwork without new generated icons.
Limestone, plaster, slate, oak, paving and carved stone retain their physical scale;
iron derives from desaturated stone grain. Joint/recess/underside shading is baked
into the texture for the runtime painted mode. Blender materials use emission.

Preserve **256 texels per runtime cell**. Extraction discards eight pixels at each
source-panel edge to exclude divider/shadow bands. Continuous mirrored fields keep
fragmented walls coherent. Sixteen-pixel aligned, fully edge-filled atlas gutters
retain the SC01 correction. Two shared 4096² atlases fit the expanded kit without
reducing density; deterministic bounded packing rejects exhaustion. Mesh exports
verify UV area density and geometry bounds, including the hipped roof's tangent
projection. UV/mip fixture guarantees remain limited to the tested mip levels.

The dungeon pack supplies shared sky/fixture art through existing bindings and has
no dependency on city geometry. City textures/models are acquired only by city
views; active/staged limits and rollback remain unchanged.

## Review and checks

```sh
cmake-build-debug/blobber --sanctuary-city-review
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_sanctuary_city cmake-build-debug/assets /path/to/new-review-directory
nice -n 15 ionice -c 3 env OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 python3 tests/city_materials_tests.py
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-debug -R '^(sanctuary_city|architecture|scenery)$' -j 1 --output-on-failure
```

Capture output includes authoritative `layout.svg`, named views, sampled movement
and resource/pose reports. Append an exact shot name, `materials`, `stills`, or
`motion`, or `gate-motion` to reduce capture work. `tree` selects six tree stills and twelve
translation/turn samples. `dressing` selects final scenery/sign views; `water`
exports eight 125ms water samples and clock/color/picking revisions. `layout` exports only the SVG and needs no graphics
context or asset loading. Output directories must be new. Hidden windows and
in-process input are used; never automate desktop input or focus.

Owned tests use generated data independently of production art. The expanded town
replaces SC01's two-house review factory and shot names; historical SC01 captures
are retained. See [SC02 design](../../../docs/sc02_design.md) and
[verification/handoff](../../../docs/sc02_verification.md) for delivered evidence,
resource totals and current review status.

Current SC03 design and checkpoint: [design](../../../docs/sc03_design.md),
[verification](../../../docs/sc03_verification.md).

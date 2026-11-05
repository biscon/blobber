# Sanctuary city architecture — SC02

Original painted architectural kit for the development-only
`--sanctuary-city-review` town. Normal campaign Sanctuary remains the courtyard.

Ten named models provide low/gabled and hipped homes, a two-storey home, timber-
framed inn, masonry smithy with broad chimney, shuttered provisioner, sanctuary
with bell-cote, windowless dungeon entrance, paired closed gate and boundary wall.
Six residences plus four service/landmark buildings, dungeon and gate use 91 total
placements, including the complete modeled enclosure. Services and travel are not
enabled. Fountain, market and planting areas are reserved for SC03.

## Regeneration

Run sequentially from the repository root, with at most two workers:

```sh
nice -n 15 ionice -c 3 timeout -k 5 120 env ALSOFT_DRIVERS=null OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 blender --background --factory-startup -t 2 --python-exit-code 1 --python tools/export_sanctuary_city.py
nice -n 15 ionice -c 3 cmake --build cmake-build-debug --target blobber blobber_review_sanctuary_city -j 2
```

`tools/city_architecture.py` owns the reusable offline geometry;
`tools/city_materials.py` owns extraction, padding, bounded atlas packing and mesh
splitting. The exporter produces packed Blender source, deterministic unlit GLB,
runtime mesh JSON, atlas PNGs, terrain textures, pack bindings and
`models/export-report.json`. Blender is not required to build or run the game.
The ordinary CMake publication consumes the checked-in runtime exports.

Model units are runtime cells, with no instance scaling. Blender uses Z-up and
runtime/GLB Y-up; source/GLB display the models in a grid. Footprints retain the
accepted low-house 3×3 and tall-house 3×4 conventions. Gate footprint is 5×2, with
local offset (-2,0); wall modules cover one cell. All geometry stays inside +/-4
horizontal cells and 0–4 vertical cells. Runtime model parts are capped at 6,000
vertices, eight parts and 24,000 vertices per model. Current models use two parts
each, split by atlas ownership.

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
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-debug -R '^(sanctuary_city|architecture)$' -j 1 --output-on-failure
```

Capture output includes authoritative `layout.svg`, named views, sampled movement
and resource/pose reports. Append an exact shot name, `materials`, `stills`, or
`motion`, or `gate-motion` to reduce capture work. `layout` exports only the SVG and needs no graphics
context or asset loading. Output directories must be new. Hidden windows and
in-process input are used; never automate desktop input or focus.

Owned tests use generated data independently of production art. The expanded town
replaces SC01's two-house review factory and shot names; historical SC01 captures
are retained. See [SC02 design](../../../docs/sc02_design.md) and
[verification/handoff](../../../docs/sc02_verification.md) for delivered evidence,
resource totals and current review status.

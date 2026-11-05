# Sanctuary city architecture — SC01.2

Original painted low/two-storey exterior shells for the development-only
`--sanctuary-city-review` scene. No campaign hub replacement or interior routes.

`source/material-master.png` was generated with the built-in ImageGen tool; the
exact prompt is retained in `prompts.json`. The six source regions supply limestone,
plaster, slate, oak, paving and sculpted stone. The exporter excludes eight source
pixels around each 512-square panel to remove generated divider/shadow bands,
then samples their interiors as continuous mirrored fields. It
adds restrained baked recess/underside shading and prepares metric padded atlases.
Iron is a dark, desaturated derivative of the generated stone grain.

Regenerate from the repository root:

```sh
nice -n 15 ionice -c 3 timeout -k 5 90 env ALSOFT_DRIVERS=null blender --background --factory-startup -t 2 --python-exit-code 1 --python tools/export_sanctuary_city.py
nice -n 15 ionice -c 3 cmake --build cmake-build-debug --target blobber blobber_review_sanctuary_city -j 2
```

Run these sequentially. Blender is an offline authoring dependency, not a runtime
requirement. The normal CMake build bakes the checked-in PNG/mesh JSON files.
The script writes the packed Blender source, a deterministic unlit GLB, runtime
meshes, atlases, terrain textures, pack bindings and `models/export-report.json`.
It uses ordinary uncompressed Blender save because this environment's compressed
library export crashed; the GLB is written directly to avoid optional optimizer
plugins. Model units are runtime cells; no instance scaling. Blender uses Z-up,
runtime/GLB Y-up. The GLB displays the two models side by side.

Each mesh has real recessed arches, closed doors, inset shutters, roof undersides,
and a chimney. The local origin is the anchor cell center, at ground level.
Low house footprint: x/z [-1.5,1.5]. Tall house: x [-1.5,1.5], z [-2.5,1.5].
Their cardinal placement rotates both models and grid footprints together.
Geometry must remain inside +/-4 horizontal cells and 0–4 vertical cells.

All triangles retain 256 texels per runtime cell, checked during export. Repeated
wall fragments share continuous UV fields; 16-pixel gutters and aligned, fully
edge-filled slots isolate adjacent regions through mip level 4 in the owned fixture.
First-fit shelf packing reuses gaps while retaining the 4096-square capacity.
Both houses share one 4096-square architecture atlas to preserve density and reuse
the material fields. See the export report for exact
triangle totals. Terrain is prepared to 256 square with one repeat per cell.
The runtime uses painted materials with the existing distance/fog policy.

The ordinary dungeon pack provides shared sky/unused fixture art through existing
pack bindings; it has no dependency on these architecture payloads. Residency loads
this pack only for the city review. The 384/448 MiB reference lines are advisory.

See [design](../../../docs/sc01_design.md) and [verification](../../../docs/sc01_verification.md) for the
engine contract, reproducible review commands, evidence and acceptance status.

Offline regression checks (NumPy is required, as in the Blender exporter):

```sh
nice -n 15 ionice -c 3 env OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 python3 tests/city_materials_tests.py
```

The small synthetic fixtures cover panel-divider exclusion, phase continuity across
fragmented/negative UV coordinates, mirrored joins, mip isolation, packing overlap
and capacity rejection. They do not depend on this artwork or require a GPU.

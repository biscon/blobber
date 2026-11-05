# Sanctuary inn

Authored cozy common room for the normal campaign, entered with F at Sanctuary's
inn door. Runtime placement and collision live in `src/game/inn_interior.cpp`.
Nine named models provide plaster/stone/timber walls, exit door, two seating groups,
bar, shelves with pottery, ale barrels, plants, hearth and decorative upstairs stair.
The room has an 8×8-cell floor enclosed by modeled walls and an outer solid pad.
Furniture is authored against the fixed .5-cell eye height: the counter is .3465
cells high, tables .2516 and chair seats .1471. Loose props are narrower; the
doorway, wall trim and stair risers follow the same adult proportions. Geometry
is resized before metric UV packing; the export report retains final mesh bounds.
The ceiling is 1.5 cells high. Inn floor/ceiling boards use sampled plank interiors
and one regular joint pattern; structural timbers use joint-free grain at preserved
source aspect. The furniture retains the accepted city wood material.

`source/concept.png` and `source/flame-master.png` were created with the built-in
imagegen tool; exact prompts are retained in [prompts.json](prompts.json). Material
masters and foliage are reused from the accepted Sanctuary city pack. The exporter
bakes metric 256-texel/cell material fields with aligned, edge-filled 16-pixel gutters.
A shared 4096² opaque atlas, 256² foliage and 128×192 flame retain prepared mip/alpha
publication. Flame alpha is preserved from the generated master.

```sh
nice -n 15 ionice -c 3 env OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 ALSOFT_DRIVERS=null \
  blender --background --factory-startup -t 2 --python-exit-code 1 --python tools/export_inn.py
nice -n 15 ionice -c 3 cmake --build --preset debug --target blobber -j 2
```

The script writes this pack only. It retains packed Blender source, unlit GLB,
bounded multipart runtime mesh JSON, atlases and an export report. Blender is not
a build/runtime dependency. No furniture is interactive yet; stairs do not travel.
The inn leases assets only while active/staged. One fixed 24-particle flame/ember
emitter uses the paused presentation clock and depth testing, with no picking IDs,
RNG consumption, gameplay effects, dynamic lighting or per-frame allocations.

[Design](../../../docs/inn_interior_design.md) ·
[Verification](../../../docs/inn_interior_verification.md) ·
[Captures](../../../docs/screenshots/inn/README.md)

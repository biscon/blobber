# Arcane Emporium painted pack

Dedicated authored interior following the [approved room/sign concepts](../../../docs/mockups/magic-shop-v1/README.md).
The [interior design](../../../docs/emporium_interior_design.md) owns cell footprints,
heights, campaign integration and verification. Merchant position remains reserved;
no merchant, scroll-use mechanic or shop inventory is introduced by this pack.

Eight models / 15,206 triangles use a single 4096-square atlas, 256 texels per cell
and 16-pixel gutters. Painted stone, wood and palette variations reuse the city
material master; paving reuses the city and ceiling the inn. Potions, parchment,
books, copper still and ingredients are bounded offline geometry with painted
material shading. The counter carries the approved generated sign artwork.
No runtime Blender, procedural drawing, transparent-glass sorting or dynamic light.

The reproducible exporter writes packed Blender, unlit GLB, runtime triangle JSON
and texture sources together. Use an owned run and inspect before promotion:

```sh
emporium_run=$(python3 tools/agent_artifacts.py create)
ALSOFT_DRIVERS=null OMP_NUM_THREADS=2 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 \
  timeout 180 blender --background --factory-startup -noaudio --threads 2 \
  --python-exit-code 1 --python tools/export_emporium.py -- \
  --output-root "$emporium_run/assets" --report "$emporium_run/emporium-export.json"
# Inspect report, bounds, UVs, sources and runtime captures before promoting.
cp -a "$emporium_run/assets/packs/sanctuary-emporium/." assets/packs/sanctuary-emporium/
python3 tools/agent_artifacts.py clean "$emporium_run"
python3 tools/agent_artifacts.py clean "$emporium_run" --apply
```

Source meshes use Y-up runtime coordinates and retained Blender uses Z-up. Parts
respect existing 6,000-vertex splits / eight-part and 24,000-vertex model bounds.
Exporter asserts reservation/height bounds; publication validates all bindings.
Counter/bench work surface is .30 cells, cabinets .89 cells; color bottles are
hand-sized (.103-cell unscaled height) rather than oversized room decorations.

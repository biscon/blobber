# Underkeep

Abandoned fortress biome. Existing dungeon masonry, doors and terrain shading are
retained. New props/cutouts use painted AO-style form cues with distance dimming
and fog, without dynamic lighting or PBR.

Review sizing: runtime chest instances uniformly scale both authored variants by
0.5 about their floor anchor. Blender/GLB/JSON retain their source units. The same
prepared triangles drive drawing and picking; cage and rack scale is unchanged.

- `concepts/`: imagegen architecture, chest and cage references, shown before modelling.
- `prompts.json`, `source-prompts.json`: exact built-in imagegen prompts and provenance.
  The empty rack concept was rejected by the image tool; its model follows the
  written empty-furniture specification. No alternate image API was used.
- `source/`: original transparent cutouts, painted material master, and the generated prop
  atlases. Chest variants share one atlas. Generated alpha is preserved by the
  prepared texture pipeline; artwork is not camera-facing.
- `models/underkeep.blend`: editable, packed Blender scene; no default scene objects.
- `models/underkeep.glb`: the four named models/variants, spaced for inspection.
- `models/*.mesh.json`: narrow triangle-list bake inputs (position, normal, UV).
- `pack.json`: named art/model bindings. No runtime Blender or GLTF dependency.

From the repository root, regenerate the authoring exports with:

```sh
nice -n 15 ionice -c 3 blender --background --factory-startup -t 2 --python tools/export_underkeep.py
```

This creates only the dedicated `UnderkeepAssets` scene and writes the listed
Underkeep exports. Normal CMake builds bake prepared runtime assets. Blender 5.2
crashed on repeated temporary-scene exports during authoring; the reproducible
script uses one combined export in a headless process.

See [design and verification](../../../docs/underkeep_dressing_design.md).

## Prop surface revision (2026-11-01)

[Comparison gallery](../../../docs/screenshots/underkeep-props/README.md) and
[design/verification](../../../docs/underkeep_props_revision.md).
The exporter now derives three distinct padded atlases (512² chest, 1024² cage/rack) from the retained
painted master, with metric face UVs: 128 texels/source-unit for chests, 256 for
cage/rack, yielding 256 texels/runtime-cell for all four models. Four-pixel gutters
isolate face fields; shading, joints and wear are baked colour, with no lights.
Octagonal rollers replace intersecting boxes; chest wall/base faces no longer
overlap, and cage rails fit between posts. Triangle counts: 192/192/672/260.
Both chest states share an atlas. The 0.5 runtime transform and gameplay records
are unchanged. Export includes density assertions and deterministic atlas packing.

Headless Blender review: `blender --background --factory-startup -t 2 --python
tools/review_underkeep_blender.py -- OUTPUT_DIRECTORY`. In this environment Blender
finishes writing but stalls in PulseAudio shutdown; use an external timeout and
check the completion log/files. The interactive Blender process is not involved.

The gallery records the earlier 80-texel revision. Current textures match the
dungeon floor/wall density of 256 texels per cell and were approved by the user
on 2026-11-01. See the [prop modelling guide](../../../docs/prop_modelling_guide.md)
for the authoring lessons and workflow.

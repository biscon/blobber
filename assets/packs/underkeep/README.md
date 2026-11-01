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
- `source/`: original transparent cutouts, painted material master, and three 256²
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

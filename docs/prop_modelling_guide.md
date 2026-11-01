# Painted dungeon props: authoring guide

Based on the Underkeep chest (closed/open), cage and rack, approved by the user
on 2026-11-01. Use [the exporter](../tools/export_underkeep.py) as a working example;
[revision notes](underkeep_props_revision.md) contain the diagnosis and verification.

## Shape and materials

Build simple, readable medieval furniture with real thickness and clear joints.
Use solid prisms for rollers, fitted rails between posts, and raised iron bands.
Avoid stacks of intersecting boxes that leave overlapping coplanar exterior faces:
the old rack's rotated boxes produced overlapping end caps, and chest walls
overlapped the base's side faces. Fix those surfaces in the mesh, not with global
depth bias or camera changes. Check the open chest, rear faces and underside too.

Paint the form into the colour texture: dark joints, recesses and undersides,
restrained grain, dull iron and modest wear. Preserve readable material contrast
without noisy detail on every small part. Preview with emission materials in
Blender: lamps can conceal weaknesses that appear in the game's unlit painted
mode. Runtime distance dimming/fog remain; do not depend on Lambert/PBR lighting
or shadow maps for the prop to read correctly.

## Match the dungeon's texels

Measure the surrounding terrain before choosing prop resolution. Our dungeon
floor/wall textures are 256² with one repeat per cell: **256 texels per runtime
cell**. The first revision matched props to each other at 80 texels/cell but was
visibly too coarse beside the floor. The intended style retains fine pixel-art
detail; consistency within the prop set alone is insufficient.

For a uniform runtime scale `s`, use `256 * s` texels per source-model unit.
For an edge of source length `L` in an atlas of width `A`, its UV span is
`L * 256 * s / A`. The half-size chests therefore use 128 texels/source-unit;
cage and rack use 256. Size UV rectangles by physical edge lengths and align
grain along boards. Never map the same entire texture patch onto every face.
Check both UV axes for stretching; triangle UV/world area ratios are a useful
additional density check, but alone cannot detect anisotropic distortion.

Use sufficient atlas space: the accepted chest atlas is 512², cage/rack 1024².
Keep exact metric UV spans even when storage rectangles round up to integer
pixels. Replicate edge colours into gutters (currently four pixels), then review
minification and movement for bleed. Grow/check the atlas budget deliberately;
do not silently shrink detail to make packing fit. These sizes added about 11 MiB
of mip storage and stayed within the existing runtime budgets.

When increasing density, resample genuine detail from the retained high-resolution
master, not the old low-resolution atlas. Preserve the accepted recoloring,
physical grain scale and width of painted shading as resolution changes. Keep
both chest states on the same atlas.

## Reproducible delivery and review

1. Change the modelling/export script along with the art. Editing only the `.blend`
   leaves regeneration able to overwrite the fix. Retain texture masters and
   concepts; generate the packed Blender source, GLB and runtime mesh JSON together.
2. Preserve floor origins, source units, runtime instance scale, interaction IDs,
   chest state bindings and movement footprints. A texture-only change should
   leave exported vertex positions/normals and triangle counts unchanged.
3. Regenerate, then bake the prepared assets for the build the user will run:

   ```sh
   nice -n 15 ionice -c 3 blender --background --factory-startup -t 2 --python tools/export_underkeep.py
   nice -n 15 ionice -c 3 cmake --build cmake-build-release --target blobber_bake_world_assets -j 2
   ```

   Run costly jobs sequentially. Substitute the Debug build when needed. Some
   Blender runs here finished writing but stalled during PulseAudio shutdown;
   use a bounded timeout if necessary and verify completion/files separately.
4. Inspect Blender/model-viewer and actual game results independently: all sides,
   underside, adjacent views, movement and both chest states. Compare against the
   dungeon floor/wall at the same depth. Passing mesh/UV/GL checks does not establish
   acceptable visual quality. Use focused checks for changed behavior; hidden,
   application-owned captures are available through `--underkeep-props-review`.
   Never control the user's desktop. A gallery is optional; the user may prefer
   direct in-game/model-viewer review, as for the accepted density correction.

The historical comparison gallery shows the rejected 80-texel revision, not the
final approved density. Treat the current assets and this guide as the baseline.

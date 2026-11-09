# BR02 kitchen/mead hall source kit

Pair-first checkpoint; initial artwork rejected, revised scale/hearth awaiting review. These are prepared named
scenery models, not additions to the four legacy prop slots. Campaign placement
is unchanged. Barracks/armory art follows the pair review.

`hearth`, `work-surface`, `provisions` and `table-and-benches` bind directly to
the blueprint kit IDs. The user-requested half-size revision supersedes the initial
four kit bounds: hearth/work-surface/provisions occupy 1×1 cells; table-and-benches
occupies 1×2. Heights are .5/.175/.225/.2 respectively. Runtime scale is 1, origin is anchor-cell floor
center, axes are +X right/+Y up/+Z backward. All meshes stay within the matching
blueprint footprint/height. Atlas density is 256 texels per cell with four-pixel
gutters. Stone uses the actual dungeon wall texture; other materials use retained imagegen
masters. All shading is painted. Arch masonry, rounded cookware, supported ironwork
and chamfered timber add detail within the existing 2,000-triangle limit per kit.

The dedicated `tools/export_blueprint_rooms.py` generates these JSON exports,
the shared `source/rooms/pair-atlas.png`, packed `blueprint-room-pair.blend` and
unlit `blueprint-room-pair.glb`. Run in a separate background Blender process
with two threads, low priority and a bounded timeout; pass `--output-root` to
a new directory inside an owned agent run. Promote checked outputs to this pack.
The script never regenerates the accepted legacy chest/cage/rack artwork.

Concept, generated material source and exact prompts are retained in
`concepts/rooms/kitchen-hall.png`, `source/rooms/material-master.png` and
`room-prompts.json`; wood reuses `source/material-master.png`.

The pack manifest's optional `blueprintCatalog` names the source contract checked
by the baker. It permits the currently registered subset of kits but rejects
unknown IDs, multiple mesh parts and geometry outside the kit bounds. The review
launcher additionally requires every kit used by the requested room. Thus selecting
unfinished barracks/armory in finished mode fails rather than displaying blockouts.

See [BR02 design and reproduction](../../../../../docs/br02_blueprint_rooms_design.md).

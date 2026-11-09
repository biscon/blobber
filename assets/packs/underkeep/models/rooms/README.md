# BR02 kitchen/mead hall source kit

Pair-first checkpoint; fireplace accepted, revised furniture/storage/hall dressing
awaiting visual review. These are prepared named scenery models, not additions to
the legacy prop slots. Campaign placement is unchanged; barracks/armory follow review.

Nine named meshes bind to blueprint kits. `hearth` retains the accepted geometry;
`work-surface` and `table-and-benches` keep their floor area with taller supports.
`barrel`, `crate-large`, `crate-small`, and `grain-sack` replace the pallet with separate
nonblocking pieces. `tapestry` is nonblocking; two instances share the same woven
heraldry. Two `brazier` instances retain blocking bases and own local flame anchors.

Runtime scale is 1, origin is anchor-cell floor center, axes +X right/+Y up/+Z backward.
Catalog size/height bound visible geometry; `blocking` separately controls movement
and NPC reservations. All meshes fit their contracts. UV density is 256 texels/cell
with four-pixel gutters. Stone uses the actual dungeon wall texture; other materials
use retained imagegen masters. Painted shading and real thickness/fitted joints stay
within the existing 2,000-triangle limit per kit and one shared 2048² atlas.

The dedicated `tools/export_blueprint_rooms.py` generates these JSON exports,
the shared `source/rooms/pair-atlas.png`, packed `blueprint-room-pair.blend` and
unlit `blueprint-room-pair.glb`. Run in a separate background Blender process
with two threads, low priority and a bounded timeout; pass `--output-root` to
a new directory inside an owned agent run. Promote checked outputs to this pack.
The script never regenerates the accepted legacy chest/cage/rack artwork.

Concept, generated material source and exact prompts are retained in
`concepts/rooms/kitchen-hall.png`, `source/rooms/material-master.png` and
`room-prompts.json`; wood reuses `source/material-master.png`.
The new built-in-imagegen tapestry master and exact prompt are retained as
`source/rooms/heraldic-tapestry-master.png` and `source/rooms/tapestry-prompt.json`.
The exporter resamples the master once for metric UV density and maps it continuously
across the cloth folds; it never repeats the entire motif on each strip.

The pack manifest's optional `blueprintCatalog` names the source contract checked
by the baker. It permits the currently registered subset of kits but rejects
unknown IDs, multiple mesh parts and geometry outside the kit bounds. The review
launcher additionally requires every kit used by the requested room. Thus selecting
unfinished barracks/armory in finished mode fails rather than displaying blockouts.

See [BR02 design and reproduction](../../../../../docs/br02_blueprint_rooms_design.md).

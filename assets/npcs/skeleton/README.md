# Skeleton — B01

Approved reference: `reference/turnaround-v1.png` (2026-10-31, “yes concept art
approved”). Built-in imagegen supplied the reference, 21 animation masters, eight
corrective edits and the native initiative portrait (`assets/ui/skeleton.png`). Exact requests and references are retained
in `reference/`, `animation/prompts.json` and per-sheet/revision prompt records.
Superseded masters remain in `animation/revisions/`.

The four-direction idle/walk/hurt/death/prone set plus front melee contains 48
poses. The sword stays in anatomical right hand. Death falls into a bone pile;
knockdown uses an intact prone pose. Idle 600/600 ms; walk 300/300; hurt 150/150;
melee 700/250/250/350 with impact at 950 ms; death 200/200/200/600; prone static.

Measured `animation/sheets.json` registers one scale per sheet, explicit ground
rows/body pivots, and taller first rows in collapse sheets. Later collapsing
frames are never resized to standing height. Canvases are 480 pixels high, with
common widths within clips and fixed 464-pixel ground anchors. Full-resolution
masters are preserved. This registration keeps the complete three-appearance
publication inside its existing texture budget. The body is .45 cells wide and
the canvas .85 cells high; clip artwork may extend beyond the body without
changing one-subcell occupancy.

World baking extracts registered frames and prepares the current mip/alpha chain.
Runtime derivatives belong in the build tree; do not hand-edit or commit extracted
frames as alternative masters. The source sheets are the authoritative artwork.
The capture gallery provides registered pose previews and real arena images.

Mechanics, interfaces and validation: [design](../../../docs/b01_skeleton_design.md).
Delivery evidence/review: [verification](../../../docs/b01_verification.md).

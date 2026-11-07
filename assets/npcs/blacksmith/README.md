# Duncan Ironballs

Approved [character reference](../../../docs/mockups/blacksmith-v1/README.md),
2026-11-07. Tall, muscular human smith with full red beard and shaved-side braid.
Runtime appearance and animation acceptance remain separate from concept approval.

Five built-in imagegen masters preserve the approved identity: two idle poses
each for front/back/screen-left/screen-right, and four front talk poses. Exact
generation requests are beside each PNG in `animation/sheets`. No procedural
body animation or image edits substitute for authored frames.

Registration via `python3 tools/register_resident_art.py blacksmith` measures
alpha >15, planted ground and upper torso pivots. One uniform scale per sheet
fits the tallest body to 520 pixels on an 800x608 canvas anchored at (400,584).
Logical height .75 gives about .641 cells / 2.05 m of visible standing height.
Left and right art face screen left/right respectively, matching the corrected
innkeeper convention. Idle is 1800/140 ms; Talk 180/160/180/220 ms, with side/back
directional Idle fallback. The normal extractor and world publication generate
registered frames/cropped mip chains in the build tree.

Resident `sanctuary.blacksmith` occupies smithy cell (6,3), subcell (1,1), facing
south. YAML dialogue and shop definitions live separately under
`assets/dialogue/blacksmith.yaml` and `assets/shops/blacksmith.yaml`.
See [system design and verification](../../../docs/blacksmith_design.md).

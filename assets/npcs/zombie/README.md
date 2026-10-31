# Zombie — B04

Unarmed decayed human villager, gray-green skin, cloudy eyes, sparse dark hair,
ragged beige linen tunic, brown belt/trousers/shoes and restrained wounds. The user
selected this direction during planning and accepted
[turnaround v1](reference/turnaround-v1.png) with “reference accepted”.

All masters use the built-in imagegen tool. The reference used the actual shipped
melee-goblin idle master and archer turnaround v2 as style references. Every exact
request is retained beside its output; no API/CLI fallback or SRD illustration was
used. Native initiative art is `assets/ui/zombie.png`; its request is retained in
`reference/portrait.prompt.json`.

21 selected animation sheets contain 48 poses: four-direction idle, walk, hurt,
death/corpse and prone, plus front slam. Five superseded sheets and their requests
are retained in `animation/revisions`: three walking sheets were corrected to
distinct open/passing steps, left death to nonoverlapping cells, and back prone to
a low horizontal silhouette with its face hidden. Background RGB in transparent
pixels is harmless; the runtime uses alpha and never paints that backdrop.

`animation/sheets.json` records measured transparent gutters, upper-torso body
pivots, source ground rows and one uniform scale per sheet. Fallen/prone poses use
their body center; death keeps its initial upright pose's scale rather than
normalizing collapse height. All extracted canvases are 800×608 with ground at
584 and horizontal anchor 400. Attack scale uses the first upright crown-to-sole
measurement, leaving headroom for raised hands. Prone scale uses body length.
`animation/registration_audit.json` retains source sizes/bounds and registration.

Logical canvas height .75 cells and body width .45 preserve human world scale
and one-subcell occupancy. Idle 600/600 ms, walk 300/300, hurt 150/150, death
200/200/200/600, static prone; slam 700/250/250/350 with impact at 950 ms. Prone is
delivered for knockdown despite sleep immunity. No unused ranged/cast clip.

The ordinary offline extractor and full-resolution mip/crop/alpha publication
produce derivatives in the build tree, not source animation/frames. The alpha-
composited QA sheet is `docs/screenshots/b04/registered-poses.png`. Portrait ID 85
follows giant rat and does not overlap the reserved party portrait range 64–83.

See [design](../../../docs/b04_zombie_design.md) and the B04 verification record
for delivery checks and limitations. Reference v1 and the completed slice are
accepted. The user accepted B04 on 2026-11-01 with
“slice accepted, record it in the plan doc”.

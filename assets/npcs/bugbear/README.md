# Bugbear — B02

User accepted `reference/turnaround-v1.png` on 2026-10-31: “reference accepted,
looks great”. Built-in imagegen produced the concept, initiative portrait, 21
animation sheets and 12 corrections/regenerations. Exact prompts/references are
retained alongside sheets and revisions. Superseded masters remain in revisions;
the UI portrait is `assets/ui/bugbear.png`.

48 poses: four-direction idle, walk, hurt, death/corpse and prone; front melee.
Idle 600/600 ms, walk 300/300, hurt 150/150, death 200/200/200/600, melee
700/250/250/350 with strike at 950 ms. No unused ranged action. The right-hand
morningstar and left-hand wood shield match the accepted concept. Walk corrections
introduce distinct passing strides; the corrected back prone pose shows rear
armor/head. Two collapse sheets were regenerated with separated cells.

`animation/sheets.json` is measured registration; `registration_audit.json` retains
source bounds and measurements. Uniform per-sheet scales, individual ground/body
pivots, 536-pixel canvases and 512-pixel ground anchor preserve grounding. Clip
widths range from 416 to 560 pixels. Logical canvas height is .95 cells to retain
raised-weapon room, body width .5 cells; occupancy remains one subcell. Collapsing
poses keep their scale and become naturally low. Prone poses retain a separately
registered anatomical scale rather than standing-height normalization.

Current production baking extracts nearest/full-prepared-resolution derivatives,
then crops transparent margins with two-texel padding and original-canvas offsets.
No additional resolution reduction is applied. Masters remain untouched. Extracted
frames belong in the build/scratch tree; the review gallery retains QA copies.

Mechanics/design: [B02 design](../../../docs/b02_bugbear_design.md).
Review/evidence: [B02 verification](../../../docs/b02_verification.md).

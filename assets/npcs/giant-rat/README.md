# Giant rat — B03

Standard beast rat, no diseased variant. User approved
[reference/turnaround-v3.png](reference/turnaround-v3.png) on 2026-10-31:
“approved, thats a mean and nasty rat :)”. Earlier references retain the margin
correction and the request to replace a cute face with a lean, vicious animal.
Every request/reference/correction is retained beside its image. All art uses
the built-in imagegen tool, not the API/CLI fallback.

21 selected master sheets contain 48 poses: four-direction idle, walk, hurt,
death/corpse and prone, plus front bite. Nine superseded sheets are retained in
animation/revisions. Corrections fix rear facing, prone facing, background alpha
review and touching death-sheet silhouettes. Native initiative portrait is
assets/ui/giant-rat.png; its exact request is in reference/portrait.prompt.json.

animation/sheets.json records measured extraction: one uniform scale per sheet,
upper-torso body pivots, explicit local ground rows, transparent sheet splits,
536 px canvas height and 512 px ground anchor. Clip widths are 656–1256 px.
Death frames retain the first upright pose’s scale; prone matches anatomical
length rather than standing height. No collapsing frame is scaled independently.
An alpha-composited contact sheet is retained in docs/screenshots/b03.

Logical canvas height .30 cells and body width .40 cells preserve the rat’s low
silhouette and one-subcell occupancy. Idle 600/600 ms, walk 300/300, hurt 150/150,
death 200/200/200/600, static prone; bite 300/150/150/300 with impact 450 ms.
Extraction and full-resolution mip/crop preparation reuse the ordinary offline
pipeline. Derivatives live in the build tree, not source animation/frames.

See [design](../../../docs/b03_giant_rat_design.md) and
[verification](../../../docs/b03_verification.md) for actual checks and acceptance.
The user accepted the completed B03 slice on 2026-10-31: “slice accepted, record it in the plan doc”.

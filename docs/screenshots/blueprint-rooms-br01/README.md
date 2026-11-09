# BR01 layout and blockout review

Status: **BR01 accepted and closed by the user, 2026-11-09.**
Captured 2026-11-09. These are temporary color-coded blockouts, not BR02 finished art.

| Layout | Top-down plan | Native in-engine view |
| --- | --- | --- |
| Kitchen | [Plan](plan-kitchen.png) | [Hearth and preparation surface](view-kitchen.png) |
| Mead hall | [Plan](plan-mead-hall.png) | [Tables, central aisle and supports](view-mead-hall.png) |
| Barracks | [Plan](plan-barracks.png) | [Sleeping bays and lockers](view-barracks.png) |
| Armory | [Plan](plan-armory.png) | [Wall storage and work surface](view-armory.png) |
| Connected kitchen/hall | [Plan](plan-kitchen-hall.png) | [Shared doorway into the hall](view-kitchen-hall.png) |

Plan coordinates include the preview's outer padding; authored interior coordinates
and mesh pivots are in the [BR01 design](../../br01_blueprint_rooms_design.md).
Green indicates reserved circulation, blue encounter space, brown whole-cell movement
blocking, R an empty chest reservation and A a walkable access cell. Encounter areas
may also serve circulation. Unselected sockets remain walls; dashed outlines show
alternatives. Native captures are unscaled 1920×1080 application-owned output.

Retention exception: ten images remain as the accepted BR02 handoff reference.
The four individual plans establish exact furniture, lane and access reservations;
their four native views establish room proportions and visible collision bases.
The pair plan establishes the shared socket alignment; its native view establishes
the doorway and ceiling transition. Both representations are needed for each case
when replacing blockouts with finished art. Replace superseded references during
BR02 rather than dropping a distinct accepted layout or transition case now.

Reproduction: build the explicit review targets and bake a separate blockout publication
using the commands in the [design/handoff](../../br01_blueprint_rooms_design.md#reproduction).
Run `plans` for diagrams and `capture` for each room. These retained views use rotation
0, seed 5, empty walkthrough mode, default sockets and the existing preset party.
The connected view is the `connected-pair` capture; individual views are `overview`.
SVG exports can be rasterized with `rsvg-convert`; durable room input is
[`underkeep.json`](../../../assets/blueprints/underkeep.json).

Six focused CPU suites pass. Hidden OpenGL checks cover 25 views including an additional
90° pair and an optional two-actor preview: zero nonedge picking disagreements, checked
door/chest commands and GPU preparation rollback/owner lifetimes. User acceptance is
recorded above; the agent did not perform interactive desktop play. No OS input or focus control
was used. Unselected captures, SVG exports, logs and the isolated prepared publication
were cleaned; source JSON and mesh-generation code remain reproducible inputs.

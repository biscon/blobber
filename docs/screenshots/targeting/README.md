# Enemy targeting review

Refreshed on 2026-11-07 with the hidden Debug `blobber_capture_targeting`
diagnostic. Mechanically verified; user visual/interactive acceptance remains pending.

The fixture has two enrolled enemies and a third unrelated enemy exposed by
geometry visibility. Only the enrolled pair appears in picker rows and cycling.
The diagnostic checks production bow eligibility, partial dart assignment and
OpenGL errors; focused CPU tests cover filtering, paging, lifecycle and stale input.
No desktop input automation was used. Temporary outputs were cleaned.

- [Exploration](exploration.png): no Targets button or dock status line.
- [Bow picker](bow.png): two encounter rows; the unrelated visible enemy is excluded.
- [Bow focus](bow-tab.png): keyboard target selection with the dialog closed.
- [Blocked melee](blocked.png): existing eligibility feedback remains available.
- [Magic Missile](magic-missile.png): one dart assigned without casting.
- [Magic Missile at 1280×720](magic-missile-1280.png): reduced presentation check.

Reproduce into a new directory inside a managed agent run:
`cmake-build-debug/blobber_capture_targeting cmake-build-debug/assets RUN_DIRECTORY/captures`.
See the [design and verification](../../target_selection_design.md#encounter-scoped-picker--2026-11-07).

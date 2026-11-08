# Combat movement review — 2026-11-08

Status: mechanically verified; interactive balance and movement presentation await
user review. Earlier positioning acceptance is not changed by replacing evidence.

Six native 1920×1080 hidden application captures use owned safe combat mechanics
and production goblin/Underkeep artwork. Two centre enemies hold position while
two flankers walk through cardinal subcells. The diagnostic asserts both flankers
attack in the same round and initiative turn as their movement, all four bodies
are CPU-picked at the end, no GL errors occur and world GPU resources are released.
No desktop input, focus control or user-save mutation was used.

- [Initial centre pair, two offscreen flankers](initial-two-visible-two-flanking.png)
- [Left flanker walking into view](flank-2-walking.png)
- [Left flanker arrived, retaining its action](flank-2-arrived.png)
- [Right flanker walking into view](flank-3-walking.png)
- [Right flanker arrived, retaining its action](flank-3-arrived.png)
- [All four engaged after both flank attacks](all-four-engaged.png)

[Movement design and verification](../../combat_positioning.md#srd-combat-movement--2026-11-08).
These six replace the prior positioning captures; disposable diagnostics are cleaned.

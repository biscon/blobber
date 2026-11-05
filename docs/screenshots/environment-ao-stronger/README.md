# Accepted stronger AO

**Accepted and closed (2026-11-05):** the user approved keeping this strength.

Strength increased by 25%: .60 → .75; maximum darkening .30 → .375.
Radius remains .30 cells. This version is active in the Debug game build.

Matched poses and simulation times; AO-off captures are pixel-identical to the
initial set. Stronger shading changes only world-viewport pixels. The focused
hidden GPU suite passes, including creature exclusion and fog preservation.

| Scene | Original AO | Stronger AO |
| --- | --- | --- |
| Fountain | [Original](../environment-ao/city-fountain-on.png) | [Stronger](city-fountain-on.png) |
| Facade | [Original](../environment-ao/city-facade-on.png) | [Stronger](city-facade-on.png) |
| Facade mid-turn | [Original](../environment-ao/city-facade-turn-on.png) | [Stronger](city-facade-turn-on.png) |
| Closed door | [Original](../environment-ao/dungeon-door-closed-on.png) | [Stronger](dungeon-door-closed-on.png) |
| Door and creature | [Original](../environment-ao/dungeon-door-creature-on.png) | [Stronger](dungeon-door-creature-on.png) |
| Chest | [Original](../environment-ao/dungeon-chest-on.png) | [Stronger](dungeon-chest-on.png) |
| Chest mid-turn | [Original](../environment-ao/dungeon-chest-turn-on.png) | [Stronger](dungeon-chest-turn-on.png) |
| Corner | [Original](../environment-ao/dungeon-corner-on.png) | [Stronger](dungeon-corner-on.png) |

Each scene also has `-off.png` and `-ao.png` files in this directory.
[Design and verification](../../environment_ao_design.md). User visual acceptance is
recorded; no interactive desktop input or benchmark was performed.

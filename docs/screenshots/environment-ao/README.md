# Environment AO review

Default-on prototype, 2026-11-05. Each pair uses the same pose and simulation time.
Normal presentation is 1920x1080; AO-only images are the 1000x564 scratch field
(white means no darkening). No creature, billboard, ground-shadow, effect or UI
contributes to or receives AO. Existing creature ground shadows remain.

| Scene | AO off | AO on | AO only |
| --- | --- | --- | --- |
| City fountain base | [Off](city-fountain-off.png) | [On](city-fountain-on.png) | [AO](city-fountain-ao.png) |
| City facade/recesses | [Off](city-facade-off.png) | [On](city-facade-on.png) | [AO](city-facade-ao.png) |
| City facade mid-turn | [Off](city-facade-turn-off.png) | [On](city-facade-turn-on.png) | [AO](city-facade-turn-ao.png) |
| Dungeon closed door | [Off](dungeon-door-closed-off.png) | [On](dungeon-door-closed-on.png) | [AO](dungeon-door-closed-ao.png) |
| Open door with goblin | [Off](dungeon-door-creature-off.png) | [On](dungeon-door-creature-on.png) | [AO](dungeon-door-creature-ao.png) |
| Chest/floor contact | [Off](dungeon-chest-off.png) | [On](dungeon-chest-on.png) | [AO](dungeon-chest-ao.png) |
| Chest mid-turn | [Off](dungeon-chest-turn-off.png) | [On](dungeon-chest-turn-on.png) | [AO](dungeon-chest-turn-ao.png) |
| Dungeon corner | [Off](dungeon-corner-off.png) | [On](dungeon-corner-on.png) | [AO](dungeon-corner-ao.png) |

The effect is deliberately restrained: .30-cell radius, .01-cell bias, .60 strength,
30% maximum darkening. Look at the fountain's ground contact, window/door recesses,
wall/floor seams and the chest base. Fixed sampling can be seen in AO-only images;
composed artwork suppresses it. Turning can expose missing offscreen geometry,
an inherent limit of this technique. Interactive visual acceptance remains user-owned.

The capture tool uses the current prepared artwork and isolated scripted scenes;
it does not open or change campaign saves. All windows were hidden, with no OS input.
Pixel comparisons found changes only inside the world viewport for all eight pairs.
[Resource/draw report](report.txt); [implementation and checks](../../environment_ao_design.md).

Launch normally for AO, or pass `--ambient-occlusion off` for comparison.

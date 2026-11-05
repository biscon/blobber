# Stone dungeon lighting — C accepted

**C / Early accepted — 2026-11-05.** C is the stone-dungeon gameplay default.

[Open the comparison viewer](index.html). Switch Legacy / C on the same
image, then switch between Corridor and Room. Number keys 1–2 also select a variant.
The viewer uses the original, unedited 1920×1080 application captures.

| Choice | Lighting treatment | Corridor | Room |
| --- | --- | --- | --- |
| Legacy reference | Previous distance falloff | [Image](corridor-current.png) | [Image](room-current.png) |
| C — Early (accepted) | Faster falloff starting at the camera | [Image](corridor-c-early.png) | [Image](room-c-early.png) |

Four retained images compare the legacy reference with accepted C in two poses.
Corridor emphasizes wall/floor distance; room includes a goblin, cage and nearby
chest/stonework. Camera, animation time (zero), AO, supersampling, textures and UI
are fixed. C darkens nearby surfaces and increases falloff with distance.
Unselected A/B images were pruned after acceptance.

Production sanctuary city and inn outputs were pixel-identical across all four
presets. All screenshot changes are confined to the world viewport; no pixel
became brighter. Hidden OpenGL and focused CPU checks pass. No desktop input was
used. The user selected C from these static mood comparisons; no interactive
movement review was performed.

See [design, reproduction commands and verification](../../dungeon_lighting_design.md).
Raw logs and duplicate captures were removed using the task's artifact cleanup.

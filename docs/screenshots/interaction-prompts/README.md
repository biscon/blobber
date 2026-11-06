# Viewport interaction prompts

Implemented and mechanically verified on 2026-11-05. **Awaiting user visual review;
not accepted yet.** Four application-owned 1920×1080 captures from a hidden OpenGL
window; no desktop input or focus automation. See the
[design and verification](../../native_hud_design.md#viewport-interaction-prompts--2026-11-05).

| Case | Capture |
| --- | --- |
| White Display text and offset shadow on bright city stone | [Enter inn](city-enter-inn.png) |
| Short action over darker dungeon scenery; no background banner | [Open](dungeon-open.png) |
| Hovered loot takes precedence over the faced door | [Loot](dungeon-loot.png) |
| Active goblin combat beside an open door; no interaction prompt | [Combat](combat-no-prompt.png) |

Only these four review images are retained. Prompt-free baselines and superseded
captures were disposable run artifacts. Cleanup does not imply user acceptance.

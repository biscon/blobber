# Procedural melee swipe

Revised and mechanically verified 2026-11-05. The user rejected the first thin,
flat treatment; these replacement captures show the broader crescent and
translucent edges matched against the supplied screenshot. **Shape accepted by the user; 185 ms display time awaits user trial.** Six application-owned captures from a hidden 1920×1080 OpenGL
window, using real melee commands, production goblin/Underkeep artwork and owned
combat/party fixtures. No desktop input or focus automation.

These stills document the accepted shape at the previous 120 ms duration. Times
below are relative to attack acceptance in that earlier capture (swipe start 150 ms,
contact 200 ms). The current effect runs from 100–285 ms with the same shape and
200 ms contact; new stills would not demonstrate its longer duration. [Design and verification](../../melee_swipe_design.md).

| Case | Capture |
| --- | --- |
| Advancing tip, 17 ms into swipe | [Reveal](hit-167.png) |
| Full ivory crescent before contact | [Peak](hit-185.png) |
| Existing flash, recoil and damage text at contact | [Contact](hit-200.png) |
| Retracting/fading ribbon over existing hit feedback | [Retreat](hit-245.png) |
| Swipe expired; existing feedback continues | [Expired](hit-270.png) |
| Same stroke passes beside the target on a miss | [Miss](miss-185.png) |

Only these six review images are retained. The original enemy flash briefly makes
the ivory stroke less distinct at contact; preserving that feedback is intentional.
The hidden GPU fixture additionally checks translucent blending against scene pixels,
both diagonal orientations and both
supersampling modes. The new timing and tooltip interaction still await user trial.

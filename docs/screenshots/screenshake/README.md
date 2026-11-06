# World-window screenshake review

Status: mechanically verified; awaiting user timing/feel review (2026-11-05).
[Design, tuning and verification](../../screenshake_design.md).

Six native 1920×1080 application-owned screenshots from a hidden window. These are
isolated preset previews over a prepared Underkeep scene, with deterministic
classified cosmetic events and fixture party portraits. They demonstrate framing
and stationary HUD, not actual damage from a played combat sequence. CPU fixtures
separately exercise real committed area spells and the shared combat feedback path.
Stills cannot establish acceptance of animation timing or feel.

| Capture | Purpose |
| --- | --- |
| [Rest](rest.png) | Original framing |
| [Little, 25 ms](little-25ms.png) | Incoming melee preset |
| [Medium, 25 ms](medium-25ms.png) | Critical-hit preset |
| [Big, 25 ms](big-25ms.png) | Area landing preset |
| [Big, 80 ms](big-80ms.png) | Decay and diminishing crop |
| [Expired, 360 ms](expired-360ms.png) | Pixel-identical return to rest |

Pixel comparison confirms every changed pixel stays within the fixed native world
rectangle (12,54)–(1512,900); expired and rest captures are identical. Generated-fixture
OpenGL checks additionally cover moving enemy feedback/depth clips, stationary party
feedback, both quality settings, GPU cleanup and cached world-image reuse.

Reproduce with `blobber_world_integration_gpu_tests ASSETS RUN/captures
--screen-shake-review`, using the artifact workflow and a hidden graphics context.
No desktop input or focus control was used. No animation acceptance is implied.

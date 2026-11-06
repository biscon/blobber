# Camera framing comparison

**B accepted and implemented, 2026-11-06.** The user selected the current in-game
40% horizon after comparing 34/40/45%. The original 34% projection is retained as
the previous gold-standard reference; 45% was not selected.

[Open the comparison gallery](index.html). Use **1 / 2** to flip projections at
the same pose. Each PNG is an unmodified 1920×1080 application-owned capture from
a hidden window, supersampling enabled. Pullback .4 cells, eye height .5 cells,
horizontal FOV 84°, game clock 0 and the preset party are identical in all variants.
Angles below describe the centre ray; actual camera pitch stays 0°.

| Scene | A — original reference, 34%, 9.2° down | B — accepted, 40%, 5.8° down |
| --- | --- | --- |
| Underkeep intersection | [A](dungeon-gold-34.png) | [B](dungeon-trial-40.png) |
| Sanctuary fountain | [A](sanctuary-gold-34.png) | [B](sanctuary-trial-40.png) |
| Inn common room | [A](inn-gold-34.png) | [B](inn-trial-40.png) |

Six images remain: accepted B and the original reference in each environment.
The three unselected 45% captures were removed after the user's explicit acceptance.

## Reproduction and rollback

The render-only constant `world_render::viewHorizon` in
[`world_projection_parameters.hpp`](../../../src/render/world_projection_parameters.hpp)
controls CPU/GPU projection, culling, picking and the sky. Accepted value `.40f`;
restore `.34f` and rebuild to return to the gold standard. Update the independent
projection expectations in the CPU, AO and creature-occlusion checks when making
any future projection change. The current expectations already cover accepted B. The pullback is unchanged.

For each `.34f`, `.40f`, `.45f` value, build the explicit review tool and capture
into a fresh subdirectory of an owned agent run:

```sh
nice -n 15 ionice -c 3 cmake --build --preset debug --target blobber_review_camera_framing
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_camera_framing cmake-build-debug/assets NEW_OUTPUT
```

The tool selects an actual generated T/cross intersection with open front and side
cells plus solid diagonal corners. This capture uses Underkeep seed 5, depth 1,
generator 10/catalog 7, fingerprint `13806920274028980739`, cell `(16,14)` facing
south. Encounters and door fixtures are omitted to keep the framing unobstructed;
generated terrain and dressing use the production map. Sanctuary uses `(11,10)`
south, and the inn uses `(5,8)` north with their production maps and artwork.
No saves are read or modified and no OS input is injected.

Verification commands, results and limitations are recorded in the
[renderer design](../../renderer_world_design.md#camera-framing-trial--2026-11-06).

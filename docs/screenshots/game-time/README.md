# Game time and outdoor lighting

Status: mechanically verified; **awaiting user visual/interactive acceptance**
(2026-11-07). [System design](../../game_time_design.md).

Six unmodified application-owned 1920×1080 captures, produced with a hidden
OpenGL window and fixed animation/camera. No desktop input or focus automation.
The review tool uses controlled standalone sessions and explicit game-time samples;
it never changes a player save. City residents are canonical production placements.

| Capture | Time / purpose |
| --- | --- |
| [Sanctuary day](city-day.png) | Day 1 12:00, existing daytime appearance |
| [Sanctuary dusk](city-dusk.png) | Day 1 18:00, halfway through dusk |
| [Sanctuary night](city-night.png) | Day 1 22:00, full night with visible residents |
| [Map clock](map-clock.png) | Day 2 14:00, both key rows and development control |
| [Inn at night](inn-night.png) | Day 1 22:00, world pixels identical to daytime |
| [Underkeep at night](dungeon-night.png) | Day 1 22:00, world pixels identical to daytime |

Day/dusk/night city differences are confined to the world viewport. The maximum
supported day string measures 67 of the available 102 art units in the map sidebar.
All six captures were visually inspected. Night is visual only; spell light and
existing targeting/visibility rules remain independent of ambient brightness.

Reproduce after creating an ignored agent-artifact run:

```sh
nice -n 15 ionice -c 3 cmake --build cmake-build-debug \
  --target blobber_review_game_time -j 2
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 LP_NUM_THREADS=2 \
  nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_game_time \
  cmake-build-debug/assets RUN_DIRECTORY/NEW_CAPTURES
```

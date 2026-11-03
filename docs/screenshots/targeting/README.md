# Enemy targeting review

Hidden application-owned captures, 2026-11-03. Production Underkeep art and default
party; deterministic two-enemy fixture with an archer behind a melee goblin. The
fully body-covered case is separately covered by generated CPU fixtures.

| Scene | Native 1920x1080 | Reduced 1280x720 |
| --- | --- | --- |
| Bow picker, rear archer selected | [Native](bow.png) | [Reduced](bow-1280.png) |
| Tab selection with picker closed | [Native](bow-tab.png) | [Reduced](bow-tab-1280.png) |
| Blocked melee action | [Native](blocked.png) | [Reduced](blocked-1280.png) |
| One Magic Missile dart assigned | [Native](magic-missile.png) | [Reduced](magic-missile-1280.png) |

The popover shrinks to two rows and remains above the dock. Portraits/log retain
their positions. Selection outlines follow only visible body pixels. Blocked rows
explain eligibility and partial spell assignments do not cast automatically.

Controls: Tab next eligible enemy; Shift+Tab previous; Enter confirm. Targets opens
the mouse picker; Escape dismisses it. Multi-target spells retain Up/add and
Down/remove, row click/add and Shift-click/right-click removal. F2 toggles movement
animation. Direct world clicks retain their existing immediate action behavior.

Reproduce explicitly with a new output directory:

```sh
nice -n 15 ionice -c 3 cmake --build cmake-build-release --target blobber_capture_targeting --parallel 2
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 cmake-build-release/blobber_capture_targeting cmake-build-release/assets NEW_OUTPUT_DIRECTORY
```

The diagnostic asserts the hidden window flag, production rear-archer eligibility,
spell assignment identity and clean OpenGL draws. No desktop input is injected.
Mechanical and capture checks pass; interactive acceptance remains user review.

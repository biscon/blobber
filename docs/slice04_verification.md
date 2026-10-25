# Slice 04 verification and review

Implemented on Linux, 2026-10-25. User acceptance is pending.
See [HUD design](party_hud_design.md) and the
[delivery record](dnd5e_demo_design.md#slice-04-party-hud-and-commands).

## Builds and headless checks

Commands run from the repository root (build output was redirected to temporary
logs for warning inspection):

```sh
cmake --build --preset debug -j 4
ctest --preset debug
cmake --build --preset release -j 4
ctest --preset release
cmake --build --preset release --target blobber_gpu_smoke -j 4
git diff --check
```

Existing Debug/Release presets were already configured; builds automatically
reran CMake after the test/source registrations changed. Both builds succeeded
without compiler warnings. Final suites: **14/14 passed in Debug and Release**.
The two new suites are `commands` (graphics independent) and `picking` (CPU
composition, no window). The extended allocation suite passed with **zero
engine-owned C++ allocations** after preparation. Driver/raylib internals and C
allocation APIs are not covered by that guard.

Coverage includes preview-only successful/rejected commands, unchanged party and
actor resources, live formation revalidation, stale/dead/removed/foreign actors,
visibility invalidation, full 256-target paging/cycling, oversized input, log
rollover/truncation/scroll bounds, separate demo statuses, input routing, held
confirmation, focus cancellation and synthesized focus-key events. Mapping tests
cover logical/framebuffer sizes, high DPI, letterboxing/pillarboxing, invalid
sizes/nonfinite coordinates and half-open edges. Generated sprites exercise
transparent holes, translucent overlap, wall clipping, shadows, dead-flash
occlusion and expiry, stationary invalidation, cached frames, and all six movement
transitions. Picking output leaves CPU frame colors unchanged.

An initial picking fixture used an unsupported 0.8-cell sprite width; it was
corrected to the established 0.5-cell maximum before passing checks. Desktop
validation later revealed replay of a held confirmation after synthetic focus
key-up/key-down events. The release latch was fixed, an exact CPU regression was
added, and both suites and the live sequence were rerun. Held controls now require
a release observed while focused before a fresh press can act.

## Desktop, captures and integration

The sandbox could not connect to display `:0`; approved desktop execution was
used. Baseline and final commands:

```sh
timeout 60 cmake-build-release/blobber --capture /tmp/blobber-slice04-before
# Run from /tmp to also verify executable-relative assets:
timeout 60 /home/bison/Projects/blobber/cmake-build-release/blobber --capture /tmp/blobber-slice04-final
/home/bison/Projects/blobber/cmake-build-release/blobber_gpu_smoke
/home/bison/Projects/blobber/cmake-build-release/blobber --ui-demo
```

The final capture run exited cleanly with **123 PNGs**: 114 existing logical
captures, one window screenshot, and eight new `ui-*` logical captures. Compared
all existing logical viewports against the pre-change Release baseline using
Pillow `ImageChops.difference`, converting to RGB and cropping the 1920×1080
images to `(396, 24, 1896, 840)`. **All 114 viewport crops were pixel-identical.**
Full-frame HUD differences are intentional; the window screenshot is excluded
from pixel comparison. Final UI scenes were visually inspected for portraits,
status/resource samples, outline selection, unavailable commands, missing/removed
targets, and expanded log layout. A representative capture is retained at
[screenshots/party-hud.png](screenshots/party-hud.png).

The existing GPU lifecycle test passed on NVIDIA GeForce RTX 3060 / OpenGL 3.3.
Desktop input was exercised with xdotool against the launched test-owned game
window, using 120 ms presses so raylib's frame polling observes them. Clicked
Merrin, selected Back-row melee, clicked the front-right goblin, held Enter for
one second, and opened the log: one accepted preview appeared. At 1024×768,
clicked Lethiel through letterboxing; clicking the top black bar preserved the
selection. At 1400×600, clicked Alden through pillarboxing. F1 suspended
confirmation; F2/F3 advanced/reset scenarios. A separate test-owned xmessage
window exercised focus loss and return while Enter was held. No replay occurred
with the final release latch. Screenshots were inspected; the game exited cleanly.

The first automation used near-instant click pulses, which raylib frame polling
can miss; subsequent checks used explicit press/release intervals. The focus
fixture lookup was changed from PID to its unique title because xmessage did not
publish its PID. These harness failures were not counted as passing checks.
Temporary images/logs/scripts are disposable; the review steps below reproduce
the behaviors without the automation.

## User review

1. Run `ctest --preset debug -R '^(commands|picking|frame_allocations)$'`.
2. Start `cmake-build-release/blobber`. Check all four formation cards, real HP,
   unavailable mana, and selected-character highlighting. WASD/QE, Tab and F1
   retain their roles. No combat or resource spending happens.
3. Start `cmake-build-release/blobber --ui-demo`. The banner identifies sample
   values and shows NEXT/RESET controls (F2/F3). Eight cases cycle through default,
   melee preview, back-row preview, missing target, unavailable spell, sample
   status/resources, target removal, and log overflow.
4. In the default case, click Merrin or press 3, choose Back-row melee with the
   sidebar or Up/Down, then click a visible goblin or cycle Left/Right. Its visible
   silhouette and matching list row highlight. Enter/PREVIEW records the request
   without changing HP or resources. Defend needs no target. Front-row characters
   cannot preview Back-row melee. Ranged/Spell/Item explain their unavailability.
5. Click LOG to read complete messages, scroll to older entries, then click LOG
   again to close it. Hold Enter: only one request should appear. Right-click or
   Backspace cancels selection. Starting any movement clears targeting; transitions
   and the map suspend confirmation.
6. Hold a command key, switch focus away and back, then release it. It must not
   replay. A fresh press after release works. If released while unfocused, perform
   a press/release in the focused game to rearm that held control.
7. Resize to a tall or wide window. Portraits, target lists and monster clicks
   follow the fitted game rectangle; black margins do not select anything.
8. Generate `--capture /tmp/blobber-slice04-review` and inspect `ui-melee-preview`,
   `ui-back-row`, `ui-unavailable`, `ui-sample-status`, `ui-target-removed` and
   `ui-log-overflow` PNGs. Sample statuses/mana never become production resources.

## Limits

No Windows/macOS execution, physical high-DPI desktop test, large-scale benchmark,
combat playtest, or manual dungeon traversal was performed. High-DPI coordinate
mapping was covered headlessly; normal-size, letterboxed and pillarboxed desktop
input was exercised. No new portrait artwork was generated: colored silhouettes
are intentional placeholders. The development clock is fixed, and sample mana is
not a progression rule. Initiative, combat resolution, range/LOS rules, death
saves, spells, inventory, AI and slice 05 remain unimplemented by this slice.

# Slice 04 verification and review

Implemented on Linux and accepted by the user on 2026-10-25, including all
review corrections recorded below.
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


## Typography review correction — 2026-10-25

The user requested supplied Inconsolata log text, IBM Plex Sans controls/labels,
smaller text, one atlas per face/size, and rounded text positions. Implemented
native 1920×1080 text after enlargement of the art layer: Plex regular 21/24 px,
Plex bold 27 px and Inconsolata 24 px, compared with the former 30 px enlarged
default font. All four atlases load at startup and draw/measure at their exact
base size. `UiFonts::draw` rounds target coordinates before the sole DrawTextEx
call. No default-font DrawText/MeasureText calls remain in application sources.

Checks executed:

```sh
cmake --build --preset debug -j 4
ctest --preset debug
cmake --build --preset release -j 4
ctest --preset release
# Executed from /tmp with approved desktop access:
timeout 60 /home/bison/Projects/blobber/cmake-build-release/blobber --capture /tmp/blobber-font-captures
```

Builds completed without warnings. Debug **14/14 passed (18.10 s)**; Release
**14/14 passed (2.67 s)**, including the existing allocation guard. The desktop
run loaded the four separate atlases (regular at both 21 and 24) and exported
123 PNGs. Logs confirm all four font textures unloaded before the window closed.
All 114 non-UI logical dungeon viewport crops remain pixel-identical to the
pre-correction captures. Visually inspected portraits/buttons, unavailable
statuses, and expanded Inconsolata log text. Updated the retained
[HUD screenshot](screenshots/party-hud.png) and added an
[expanded log screenshot](screenshots/party-hud-log.png).

An isolated --assets fixture linked the existing rules/textures/NPC/map resources
but contained only a copy of the regular font in its fonts directory. Startup
loaded both regular sizes, failed with the missing bold font path, returned 1,
unloaded both completed atlases and closed the window. The initial verification
script overcounted matching cleanup log lines because raylib logs its default
font texture differently; checking the two actual custom texture IDs confirmed
cleanup. No fixture changed production fonts. A build-time helper-name collision
was corrected before the passing builds. This remains a slice 04 review correction;
acceptance was pending at that checkpoint; the user has since accepted slice 04.


The first resized-window check exposed thin glyph strokes lost through the art
layer's nearest-neighbor reduction. A separate persistent transparent 1920×1080
text target now uses bilinear filtering only for final aspect-fit sampling;
font glyph draws still use exact loaded sizes and rounded coordinates. Correct
separate RGB/alpha factors produce premultiplied text, avoiding double-darkened
antialiased edges when composited. The point-filtered art path is unchanged.
The Presenter now reuses three persistent targets; capture export alone creates
and releases a temporary composite target. Debug/Release rebuilt without warnings
after this platform-only correction; headless behavior was unchanged.

Repeated the desktop input/resizing sequence and inspected the 1280×720 map and
1024×768 letterboxed portraits/log. Text strokes now remain intact, hidden sidebar
text does not leak through the debug map, and targeting coordinates still match.
Held confirmation, focus return, F2/F3 and portrait clicks continued to work. The
final 123-capture run rechecks native compositing and dungeon pixel preservation.


Button-alignment review follow-up: the user noticed that legacy top offsets left
labels off-center. Action/target rows, page arrows, PREVIEW and F2/F3 now center
using their atlas glyph offsetY/bitmap-height bounds, then round at the final
pixel boundary. Rebuilt Debug/Release and regenerated the 123 captures. Measured
all 15 populated button labels in ui-melee-preview: top/bottom visible-ink margins
differ by at most **one native target pixel**. Visually inspected the corrected
capture and refreshed both retained screenshots. All 114 original dungeon
viewport comparisons still pass. Prior to this positional correction, an isolated
native comparison of direct text versus premultiplied-layer composition differed
by at most 1/255 per RGB channel, confirming that the layer did not darken glyphs.


## Live demo animation review correction

The UI demo mistakenly held its animator at the capture timestamp (700 ms).
Interactive scenarios now advance from that pose using per-scenario elapsed wall
time before refreshing frames, picking and HUD targets. Capture scenarios remain
fixed. Automatic idle phases derive from stable slots at animator construction;
nonzero authored phases and active walk/attack timing are preserved. No combat or
initiative behavior was added.

Rebuilt Debug/Release without warnings. All **14/14 suites passed** in both
(Debug 18.14 s; Release 2.72 s), including the zero-allocation guard. New generated
fixtures cover distinct idle transition times, deterministic reconstruction,
explicit phases, unchanged attack timing, stable phases after movement/removal,
and lethal-hit pose freezing with the original 100 ms expiry.

Desktop `--ui-demo` exited cleanly after taking 12 screenshots about 200 ms apart;
all 11 consecutive pairs changed. Inspected a four-frame montage: idle breathing
and the authored walking loop visibly advance with a stationary camera. The
walk/attack placements were still presentation examples at that point; the
idle-only correction below supersedes that behavior in UI scenarios.
The capture run exited cleanly with **123 PNGs**. All eight UI captures match the
previous fixed-time typography captures exactly. Side/back NPC and pillar-reveal
captures intentionally differ because directional idle fallback now uses staggered
phases. The earlier 114 unchanged viewport result documents typography only and
does not apply to this animation correction.

This correction reached Ready for review; the user subsequently accepted slice 04.


### Idle-only demo follow-up

The UI demo inherited walk/attack clip selections from the original animation
showcase. It now prepares a private idle-only placement copy for both live and
capture scenarios, retaining stable slots and phases. All four stationary goblins
idle; preview commands do not trigger fake movement or attacks.

Debug/Release builds passed. Focused `commands`, `picking`, `npc_animation`, and
`frame_allocations` suites passed **4/4** in both configurations (9.05 s Debug,
1.30 s Release). No engine behavior changed, so the full suites were not rerun
for this application-only setup correction. Live desktop frames show staggered
idle poses without walking in place; the process exited cleanly. The fresh capture
run produced 123 PNGs. All **114 non-UI captures** are pixel-identical to the
previous animation correction; seven of eight UI captures changed as expected
(the expanded log covers the dungeon in the eighth). Inspected UI default and
removed-target views and refreshed both retained HUD screenshots. Slice 04 was awaiting user acceptance at that checkpoint; acceptance is now
recorded above.


### Compact log spacing and wrapping

Removed five fixed 39-art-pixel entry slots. Expanded log entries now consume
their actual wrapped height plus a two-art-pixel gap. Inconsolata remains loaded
at 24 native pixels. Native captures show **16 short entries with 36-pixel line
spacing**, compared with five entries at 117 pixels previously. The panel stops
before its bottom inset; scrolling a partially shown entry to the top exposes
its remaining lines. Entry-based scrolling and the sidebar preview are unchanged.

Debug/Release builds passed without warnings; Release `commands` and
`frame_allocations` passed 2/2. Rendering was verified separately on desktop:
123 captures exported, with only `ui-log-overflow.png` changed among 122 logical
images relative to the idle-only correction. Updated the retained log screenshot.
An isolated temporary --assets fixture supplied 64-character names. Targeted
back-row commands wrapped to two lines at word boundaries, subsequent entries
flowed below them, and the last visible entry stayed inside the panel. Scrolling
to the oldest entry and back restored the newest panel pixel-for-pixel. Shorter
distinct-name entries also verified one-step scrolling. The initial long-name
Defend messages still fit one line, so targeted commands were used to actually
exercise wrapping. No production content was modified for these checks.

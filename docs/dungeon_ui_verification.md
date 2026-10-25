# Dungeon-first UI verification and handoff

2026-10-25 — Implemented and accepted. The user said: “much better, this slice
is accepted, close it in the document”. This closes slice 05, including the
revised presentation, and milestone A. Slice 06 has not started.
Acceptance does not change the recorded verification scope or limitations.

Design: [dungeon-first UI](dungeon_ui_design.md). Current controls: [README](../README.md#controls).
The original sidebar/target-list/confirmation review recipes are superseded.

## Executed checks

```sh
cmake --build --preset debug -j4
ctest --preset debug --output-on-failure
cmake --build --preset release -j4
ctest --preset release --output-on-failure
cmake --build --preset debug --target blobber_gpu_smoke -j4
./cmake-build-debug/blobber_gpu_smoke
./cmake-build-release/blobber --capture /tmp/blobber-dungeon-ui-final
git diff --check
```

Both builds passed without compiler warnings. **17/17 suites passed in Debug and
Release**, including the new `hud` suite and the extended frame allocation guard.
Routine tests use owned fixtures and need no window. Final Debug/Release suite
runs took approximately 24/3.4 seconds on this machine. After final player-facing
wording changes, the affected combat/HUD/command/allocation suites passed again
in both builds (4/4), and focused hidden UI captures were regenerated successfully.
CLI help, local documentation link targets and whitespace checks also passed.

Coverage includes immediate reachable attacks, rejected/protected clicks without
turn consumption, disabled actions, direct shield/2, Aid/6 and portrait commitment,
acting-character identity during inspection, pause and exclusive modal input,
cancel/reset, fresh/presented-turn gating, and persistent keyboard target feedback.
Existing focus/held-key, natural-20 recovery, outcome/restart, picking, movement,
formation, damage/MISS and combat-rule regression coverage remains in the suite.

The initiative test compares the displayed current/next participant against actual
execution for 20 seeds across 30 turns, including group expansion and round refill;
additional cases exclude dead/protected enemies. Log checks cover word wrapping,
exact-boundary words, invalid queries, scrolling and new-message read-position
preservation. The allocation guard now also exercises hover formatting, details,
menu open/close and wrapped-log queries. It observed **zero engine-owned C++ frame
allocations**; loading/capture work and third-party/driver internals are excluded.

The GPU lifecycle executable passed in a hidden GLFW window, including an assertion
that the window is hidden, real resource creation/upload/render/readback/release,
stale handles, exhaustion and clean shutdown. Final capture mode also completed
successfully on Linux/X11, NVIDIA GeForce RTX 3060, OpenGL. It exported **165 PNGs**,
including **33 `hud-*` images**, and cleanly unloaded fonts, textures, targets and
window. Full mode retains prior diagnostic names; viewport pixels intentionally
change with the new framing. No pixel-identical claim is made against the old UI.

## Visual evidence

Capture mode opens a hidden game-owned window, feeds its own `UiInput` structures
through production routing and exports its own render targets. It does not inject
OS input, capture/control the cursor, select another window, or switch desktops.
This follows the user's explicit authorization for game runs and screenshots.
The sandbox could not connect to X11; the authorized hidden checks ran with the
required display access. No packages or toolchains were changed.

Native 1920×1080, reduced 1280×720 and letterboxed 1024×768 exports exercise the
same art/text aspect-fit composition. Visually inspected cases include reachable
and unreachable hover, Defend/disabled tooltips, portrait details, Aid targeting,
history/scrolled history, map, damage/MISS on both sides, victory/defeat, and the
sample-only UI demo. Refinements from inspection included measured tooltip widths,
overlay text clearing, result-panel layering and explicit outcome dock labels.

Retained examples:

- [Reachable attack hover, 1280×720](screenshots/dungeon-ui-hover.png)
- [Party damage and MISS feedback, 1280×720](screenshots/dungeon-ui-feedback.png)
- [Read-only character details, 1280×720](screenshots/dungeon-ui-details.png)
- [Expanded history, letterboxed 1024×768](screenshots/dungeon-ui-history.png)
- [Victory result, 1920×1080](screenshots/dungeon-ui-victory.png)

For a shorter capture run:

```sh
./cmake-build-release/blobber --capture-hud /tmp/blobber-dungeon-ui-review
```

Four party portraits, one goblin thumbnail and six action icons were generated
with the built-in imagegen tool. Selected images and exact prompt/source records
are versioned in [assets/ui](../assets/ui/prompts.json). Borders, HP bars, highlights,
gear and feedback splashes are code-native. Font licenses are bundled with fonts.

## User review

1. Launch `./cmake-build-release/blobber --combat-demo --seed 5`. The dungeon is the
   primary interaction surface; portraits occupy the right rail, and the shallow
   dock keeps actions and four log lines visible.
2. Hover a goblin: check a single tooltip, outline, graphical HP bar, equipped
   damage range and reach explanation. Click a reachable body once to attack.
   The action resets to basic melee for the next participant. Unreachable clicks
   spend no turn. Left/Right and Enter provide keyboard targeting.
3. Click shield/press 2 to Defend immediately. Ranged, Spell and Item stay disabled.
   Click Aid/press 6, then a dying ally's portrait to commit. Right-click/Escape
   cancels. Other portrait clicks open paused, read-only details without changing
   the active combatant. Escape closes the overlay; a further unarmed Escape opens
   the pause menu. Gear provides Resume/Quit.
4. Read initiative left to right: active actor first, then remaining eligible
   participants this round. Resolved turns disappear. Enemy numbers correspond to
   target/log IDs; SAVE identifies a death-save turn. Check red damage/white MISS
   markers on the affected body/portrait.
5. Scroll the dock log while combat continues; new messages should not pull the
   reading position to the bottom. LATEST returns to recent entries. + opens paused
   history. Closing an overlay must not also activate the surface underneath it.
6. Finish a fight and Continue, or Defend until defeat and Restart. Check result
   buttons, held-input protection and unchanged health/formation mechanics.

Physical mouse/keyboard play, OS focus switching, physical high-DPI hardware,
Windows/macOS and production-scale performance benchmarks remain unverified.
Aspect-fit/input math and focus behavior have CPU tests; exported reduced images
verify composition, not actual OS event delivery. The previous accidental CLion
input test is invalid and is not reused as evidence. No desktop automation is
permitted. No spell/item pickers, mana mechanics, moving AI or slice 06 work was
introduced.

## Next-agent handoff

Slice 05 and milestone A are accepted and closed. Slice 06 is eligible for
planning in a clean context; inspect its code and propose its plan before
implementation. Preserve the shared UI/projection rectangles,
prepared-image renderer, fixed 1920×1080 presentation and bounded frame storage.
HUD state/hover formatting belongs in the command controller, layout/routing in
`ui/`, art/font lifetime in Presenter, and authoritative commands in GameSession.
Read the design before modifying these boundaries. Use `--capture-hud` for focused
visual review and full `--capture` for outcomes and existing diagnostics, always
through hidden application-owned export without OS input automation.

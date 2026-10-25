# Dungeon-first UI — slice 05 review revision

Status: implemented and accepted by the user (2026-10-25), including the combat
mechanics and presentation revision. Slice 05 and milestone A are closed;
slice 06 is eligible for planning and remains unstarted.

## Layout and assets

Preserve 640×360 art / 1920×1080 presentation, prepared CPU images, 90° horizontal
FOV and discrete movement. Viewport is (4,18), 500×282; right rail (508,18),
128 wide; bottom dock (4,304), 632×54 (15% height). Update projection, picking,
transitions and feedback together, without stretching the dungeon. Header carries
title, compass/depth and a pause-menu control. Right portraits follow actual
formation. Six action icons and four readable log lines share the bottom dock.

Generated four race/class-correct portraits, a goblin initiative thumbnail and six
coherent action icons with the built-in imagegen tool; assets and exact prompts live in
`assets/ui/`.
UI borders/text remain code-native. A presenter-owned startup atlas holds prepared
images and survives session restarts. Bundled licensed DejaVu Serif Bold for names
and headings; retain native-resolution readable body/log fonts. Missing production
assets fail at startup; test fixtures own their generated data.

## Commands and read models

Numbers 1..6 map to Melee, Defend, Ranged, Spell, Item, Aid. Melee clicks submit
immediately against the active character; Defend click/2 submits immediately.
Aid arms friendly targeting, then a valid portrait click commits. Unavailable
Ranged/Spell/Item icons explain their deferred mechanics. No confirmation panel,
monster list, or unsupported back-row action icon. Keyboard target cycling and
Enter remain an alternative. Reset targeting to basic melee after each action and
turn change. Right-click/Escape cancel; Escape otherwise closes overlays or opens
the pause menu, never implicitly exits through raylib.

CommandController separately owns hover, keyboard target, inspected character,
targeting mode and overlays. All requests retain world/turn/life/visibility/rank
validation and presented-turn/focus/fresh-edge gates. Hover is read-only (no RNG,
log writes or commands). One tooltip shows the hovered element or keyboard target,
weapon damage range/cost and exact rejection reason. Enemy HP is a graphical bar.
Portrait inspection is a read-only paused overlay of existing stats/equipment.
Expanded log and pause menu also pause; collapsed log browsing does not. Overlay
input is exclusive, with no click-through. Resume/Quit are explicit menu choices.

CombatController publishes a bounded upcoming-participant preview (8 entries),
derived from the existing order/cursor without changing it. Active first, then
remaining eligible actors this round, with groups expanded in actual slot order.
Resolved turns disappear; death saves remain identifiable; health changes refresh
eligibility. Initiative numbers stay hidden. Mechanics and RNG consumption do not
change.

The 32×192-byte log remains bounded. Wrapped-line scrolling preserves the read
position on new messages while older history is being read; latest view follows
new messages. Full history is an optional overlay. Feedback retains its one-second
session-time lifetime and stable identity, with anchors adapted to the new layout.
All normal frame storage/formatting remains fixed-capacity and allocation-free.

## Verification and handoff

Headless tests cover immediate actions, Aid, cancel/reset, disabled actions,
non-mutating hover, modal input/pause, held/focus/stale/new-turn input, initiative
preview versus execution, log wrapping/scrolling, feedback positioning, projection
and resources. Run Debug/Release suites and zero-allocation guard.

Use hidden-window application-owned captures and in-process UiInput scripts for
reachability/tooltips, Aid/details, initiative, feedback, history and outcomes.
No OS input injection, mouse capture, desktop switching or foreign-window control.
User explicitly authorized running the game and capturing its output for this
revision. Inspect native and reduced output, refine, then mark Ready for review;
record actual limitations. Update system design, HUD docs, controls and roadmap.

## Implemented boundaries and storage

- `ui/layout.hpp` is the shared geometry source for presentation, inverse input
  mapping, portrait feedback and diagnostics. `render/layout.hpp` owns projection
  dimensions. No image stretching or FOV change was introduced.
- `CommandController` keeps a fixed `HudHover` (four 96-byte text lines), optional
  inspected character, targeting and exclusive overlay flags in `HudSnapshot`.
  `hud_state.cpp` owns hover formatting and bounded log wrapping. Character detail
  strings borrow the immutable catalog, which outlives the session/controller.
- `CombatController::previewTurns` fills an eight-entry participant array in the
  copied HUD combat snapshot. It scans existing entries/cursors and eligibility;
  it consumes no RNG and never advances simulation. Enemy group members appear in
  the actual execution order. Skipped/protected/dead participants do not appear.
- `routeHudInput` uses existing fresh-edge input and presented-turn gates. Modal
  routing returns before any world action, including the frame that closes it.
  `runGameplay` also pauses the session for details/history/menu and handles
  explicit result acknowledgement/restart. Inspection never changes the acting ID.
- `HudPainter` shares geometry/text routines across `hud.cpp` and
  `hud_overlays.cpp`. Geometry is drawn at 640×360; text uses its native atlas size
  at 1920×1080. Overlay rectangles clear underlying text before drawing their
  own glyphs, preventing duplicate text through opaque panels. Tooltip widths use
  the same font metrics in both passes. One hovered element produces one tooltip.
- Presenter owns `UiArt`, which prepares the eleven PNGs into one 256×192 RGBA
  atlas (~192 KiB) through the existing generation-checked GPU registry. Source
  image loading/resampling happens once at startup; the atlas survives restarts.
  Missing files fail explicitly. Custom character IDs get a neutral silhouette.
  Five RAII font atlases and GPU resources unload before the window closes.
- Log storage remains 32 messages of 192 bytes. Wrapped-line queries scan this
  small bounded ring without scratch allocation. A full ring evicts oldest
  messages; a scrolled position remains anchored until its history is evicted.
  Collapsed/expanded wrapping uses 76/116 columns and displays four/21 lines.
  Changing panel width preserves the numeric line offset, not a text selection.
- Picking adds a body top edge to existing visible-body center anchors, positioning
  the graphical hover HP bar without another pixel scan. Existing prepared-body
  outlines and one-second splashes retain bounded work and fixed storage.

Spell/item pickers remain deferred with their underlying actions, visibly disabled
in this revision; no usable spell, item, ranged attack or mana rules were added.
`--ui-demo` remains isolated and visibly marks its sample resource values.

Executed checks, retained images, review steps and platform limits are recorded in
[UI verification](dungeon_ui_verification.md). No slice 06 work was started.

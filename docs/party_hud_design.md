# Party HUD and commands — slice 04

Status: implemented and accepted by the user, including all review corrections (2026-10-25).
The user approved the slice 04 plan with “Implement the plan.”

## Boundaries and ownership

The application owns a graphics-independent command controller borrowing the
existing const party and actor owners. UI sends typed selections/commands and
reads a bounded snapshot; it never edits authoritative resources. Melee,
back-row melee and defend are previews only: no rolls, costs, damage, initiative,
range promises or combat progression. Ranged, spell and item actions explain
their deferred availability. Mana progression remains slice 10. Names/catalog
strings are borrowed for their session lifetime, never copied per frame.

Four bottom portrait cards follow formation order. The sidebar contains compact
location information, six actions, four paged target rows, confirmation feedback,
and a scrollable log. Clicking LOG opens a compact, word-wrapped panel for full messages;
scrolling works on either log surface. Placeholder silhouettes are drawn with existing facilities.
The 500×272 viewport, 640×360 art grid and 1920×1080 presentation remain unchanged.
Active character means UI selection, not initiative. Normal cards show real HP,
ready status and unavailable mana. A separate development fixture supplies sample
low HP, mana, unconscious/dead status; no sample values enter Party.

## Commands, input and bounds

Commands identify a party-local character, action and optional world-scoped actor.
Confirmation revalidates current state. Dead/removed/foreign/hidden targets fail
without spending resources or selecting a replacement. Back-row preview requires
a back-row member; defend is targetless. A fixed 32-entry ring stores 192-byte
messages, overwriting oldest entries on exhaustion and safely truncating text.
Log writes occur only for explicit requests and meaningful invalidations.
Targets use fixed arrays bounded to 256 actors, ordered by stable slot.
Character/action availability and borrowed display strings form a fixed HUD
snapshot; the controller itself is immovable and rejects temporary owners.
Confirmation rechecks the live formation, even before a HUD refresh.

Mouse selects; a separate button/Enter confirms. Keys 1–4 select formation cards,
Up/Down select actions, Left/Right cycle targets, Backspace/right-click cancel.
WASD/QE, Tab, F1 and Escape retain their functions. Command controls are edge
triggered, gated until a focused release after focus loss. Held latches survive
platform-synthesized key-up/key-down events during focus changes. If a held
control was released while unfocused, press/release it after returning before
using it again. Focus loss clears pending
targeting; movement start clears targets. Transitions and debug map suspend target
selection/confirmation while retaining character/action. Shared HUD rectangles
drive hit testing and drawing. Mouse mapping inverts logical-to-framebuffer and
aspect-fit transforms, rejecting margins, nonfinite inputs and invalid sizes.

## Picking and presentation

Optional renderer output records frontmost nontransparent body slot per pixel
using the existing prepared sprite pass, alpha handling, wall depths and draw
order. Shadows never write IDs; dead flash bodies occlude picking but cannot be
selected. A pipeline owns one startup-allocated 500×272 uint16 buffer (~266 KiB),
with world/actor revision metadata. A final scan constructs living target IDs;
no per-pixel handles or additional asset preparation are needed. Transition
images are not pickable. The stable destination buffer is reused when movement
ends. A selected silhouette outline is drawn after the viewport without changing
CPU image composition. The additional scan is bounded O(view pixels + actors),
performed with changed composition, not each idle frame. The target mask uses any
nonzero prepared alpha: a translucent foreground pixel wins picking even though
background color still contributes. This is a presentation choice, not combat LOS.
The outline scans the cached mask during HUD drawing. All normal engine-owned
CPU operations reuse storage and allocate nothing.

## Development and verification

Explicit --ui-demo mode and deterministic ui-* captures exercise previews,
unavailable/invalid requests, status/resource samples, target loss, and ring
wraparound. Fixture state is separate from production resources. Existing captures
retain names; compare their viewport pixels with the baseline, since HUD changes
are intentional. Routine tests own generated maps/catalogs/sprites. Cover command
validation and unchanged state, focus/held input, layout mapping, capacity,
transparency/overlap/walls/shadows/dead flash, cache refresh and transitions.
Extend allocation checks through command, log, input, snapshot and picking paths.
Run Debug/Release CTest and available GPU, deterministic capture and interactive
desktop checks. Record unexecuted checks honestly. Implementation stopped at
Ready for review; the user accepted slice 04 on 2026-10-25. Slice 05 remains
unstarted and is now eligible for its own planning session.


## Delivered interfaces and diagnostic lifetime

- `game/commands.*`: `PartyCommand`, `CommandReason`, `TargetSnapshot`, fixed
  `CombatLog`, `HudSnapshot`, and `CommandController`. Submit returns a reason;
  PreviewAccepted never means an action was resolved. Call movementStarted for
  instant as well as animated pose changes. Refresh with current visibility and
  focus/map/transition flags before routing input or confirming.
- `render/picking.hpp`: optional composition output and read-only picking queries.
  `FramePipeline::picking()` returns null during transitions; otherwise the buffer
  belongs to that pipeline. Destination picking survives transition completion.
  Actor slots in the pixel buffer must be interpreted with its world identity.
- `ui/layout.hpp`, `input/ui_input.hpp`, `ui/interaction.*`: graphics-independent
  coordinates, layout, focus/edge gates and input routing. Platform input adapts
  raylib's frame-polled state. Presenter draws background HUD, dungeon, then
  target/log/development overlays; unchanged dungeon frames skip uploads.
- `app/ui_demo.*`: eight isolated scenarios with fresh actors, movement, animator,
  frames and command owner per scenario. The shared prepared renderer is reused.
  Scenario preparation may allocate; ordinary frame operations do not. Captures use 700 ms; interactive time advances from 700 ms for each scenario. Camera chooses an unoccupied adjacent cell facing a placement;
  absent targets produce normal missing-target feedback. F2 advances, F3 resets.
  Only the removal scenario mutates its diagnostic actor owner. Production party
  resources are always borrowed unchanged. Mana values are examples, not a rule.

The user-approved scope is unchanged. The expandable log is a readability detail
within the bounded log requirement. No new catalog schema, rules source data,
combat controller, session clock or production RNG was introduced.
See [verification and review](slice04_verification.md). Slice 05 is not started.


## Typography review correction (2026-10-25)

Requested: use supplied Inconsolata for log text and IBM Plex Sans for buttons and
labels, reduce text size, rasterize every used face/size separately, and pass only
rounded coordinates to raylib text drawing. Implemented correction: keep HUD
geometry and the dungeon on the 640×360 art surface, but draw HUD text after that
surface is enlarged onto the fixed 1920×1080 target. This avoids magnifying a tiny
TTF atlas. Font sizes are native target pixels: Plex Sans regular 24 for controls,
regular 21 for small labels, Plex Sans bold 27 for headings/names, Inconsolata 24
for log entries. Each face/size owns its own startup-loaded raylib Font. DrawTextEx
and MeasureTextEx always use the matching baseSize; drawing rounds target-space
coordinates and uses integer spacing. Layout/hit rectangles remain in art units.

Presenter owns a fixed UiFonts set, whose noncopyable RAII font members unload
before the window closes, including partial startup failure. Missing/invalid fonts
fail with their asset path instead of silently using raylib's default. Fonts copy
beside the executable through the existing asset target and honor --assets.
No live font loading, font-size synthesis or frame-time heap storage is introduced.
Validation passed: all 14 Debug/Release suites; all four font atlases loaded and
unloaded on the desktop; an isolated missing-bold startup failed explicitly and
released the two atlases already loaded. Inspected updated normal/expanded-log/
status captures and compared 114 unoverlaid dungeon viewports (pixel-identical).
Size-specific widths drive wrapping/truncation, including measured ellipses for
proportional text. Font glyph storage is the default 95 printable ASCII glyphs,
matching the existing content text contract. Font files remain unmodified.


Resized-window correction: native text must not inherit the dungeon's point
minification, which discards thin antialiased glyph strokes at 720p. Implemented one fixed
1920×1080 transparent text target (a third persistent target). Text remains drawn
at exact atlas sizes and integer positions; only final aspect-fit resampling uses
bilinear filtering for that layer. The art/game targets retain point filtering.
Use separate RGB/alpha blending when producing premultiplied text, then premultiplied
compositing so glyph edges are not darkened twice. Captures composite both layers
at native resolution into a temporary diagnostic target, released immediately;
this allocation is outside the frame path. Inspected the corrected 720p/letterboxed text and map overlay on desktop;
the final native capture comparison checks that layering preserves glyph colors
and unoverlaid dungeon pixels.


Button alignment follow-up (implemented): replaced the default-font-era fixed y offsets with
vertical centering from the selected atlas's actual glyph ink bounds (offsetY
through offsetY + bitmap height, ignoring whitespace). Center in the shared
button rectangle, then round only at the final target-pixel draw boundary.
Applied to action rows, target rows, paging arrows, preview and demo controls; use
the same alignment band for their adjacent labels. Retain horizontal text insets
and existing hit rectangles. Measured top/bottom ink margins in the native capture
for all 15 button labels: equal within one pixel.


## Live demo animation review correction (2026-10-25)

The interactive demo incorrectly reused the capture-only fixed 700 ms clock.
Scenario construction/captures stay at 700 ms; animation samples 700 ms plus
per-scenario elapsed wall time during interactive play, before frame/picking/HUD
refresh. F2/F3 reconstruct a scenario and restart its visual clock. This remains
presentation time, follows normal application's focus behavior, and introduces no
combat phase/cost clock. NpcAnimator now supplies stable automatic idle offsets for
placements with phase_ms zero/default. A nonzero authored phase remains exact.
Offsets depend only on immutable slot and idle cycle, not world identity, pose,
visibility or wall-clock/random seed; movement/removal never rephase survivors.
Attack/walk clip phases retain their existing authored timing. Directional idle
fallback shares the actor's idle offset. Lethal flashes freeze the same offset
pose at hit time; their 100 ms lifetime never uses animation phase offsets.

Fixed-capacity per-slot idle phase storage is computed during preparation. It derives
well-spaced phases using the fractional golden-ratio sequence over stable slot
indices, scaled into the idle cycle; slot zero retains phase zero. Modulo
arithmetic remains overflow-safe for uint64 timestamps. Regression tests cover
distinct transition times, deterministic world reconstruction, explicit phases,
mutation stability, idle fallback and frozen death poses. All 14 Debug/Release
suites pass, including allocation tests. Inspected live desktop frames and
fixed-time captures; see [verification](slice04_verification.md#live-demo-animation-review-correction).


### Idle-only UI demo follow-up

The UI scenarios show idle actors while movement/combat are absent. Preparation
creates an isolated copy of loaded placements in app/ui_demo.cpp, preserving type IDs,
slot order, transforms and phases while selecting Idle for every instance. Both
interactive scenarios and UI captures use this copy. ActorWorld copies it
during construction; no mutable actor API or frame-time work is needed. Existing
non-UI animation showcase assets and captures keep their authored clip choices.
Debug/Release builds and four focused suites passed. Inspected live idle playback
and fixed-time UI captures; all 114 non-UI captures remain unchanged from the
previous animation correction. Retained HUD screenshots are refreshed.


### Compact expanded log follow-up

The five fixed 39-art-pixel entry slots are replaced with a flowing layout. The
allocation-free, font-measured wrapper returns its consumed line height;
subsequent entries begin immediately after that height plus a two-art-pixel gap.
Entries draw newest-first from the existing entry scroll offset until the content
area is full. Wrapping prefers word boundaries, breaks overlong tokens at a
character boundary, and honors explicit newlines. A partially visible last entry is fully readable by
scrolling it to the top; no text may cross the panel's bottom inset. The
sidebar's one-line preview remains unchanged. No command/log-storage or
scrolling-state changes. Verified live scrolling and captures with both short and
two-line entries using a temporary content fixture; exact-size rasterization is
retained. The panel fits 16 one-line entries at 36 native pixels per entry,
compared with five entries at 117 pixels before this correction.

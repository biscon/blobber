# Party HUD and commands — slice 04

Status: implemented; Ready for review, user acceptance pending (2026-10-25).
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
and a scrollable log. Clicking LOG opens a five-entry panel for full messages;
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
desktop checks. Record unexecuted checks honestly. End at Ready for review, with
slice 05 untouched and user acceptance pending.


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
  Scenario preparation may allocate; ordinary frame operations do not. Demo time
  stays at 700 ms. Camera chooses an unoccupied adjacent cell facing a placement;
  absent targets produce normal missing-target feedback. F2 advances, F3 resets.
  Only the removal scenario mutates its diagnostic actor owner. Production party
  resources are always borrowed unchanged. Mana values are examples, not a rule.

The user-approved scope is unchanged. The expandable log is a readability detail
within the bounded log requirement. No new catalog schema, rules source data,
combat controller, session clock or production RNG was introduced.
See [verification and review](slice04_verification.md). Slice 05 is not started.

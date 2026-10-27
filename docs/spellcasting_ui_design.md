# Spellcasting UI review correction

Status: implemented, **accepted and closed**, 2026-10-27. The user explicitly
accepted slice 10 and its UI corrections; slice 11 remains Planned/unstarted.

## Interaction and ownership

CommandController owns explicit Normal, SpellPicker, Targeting and Resolving
states. Pending cast data is independent of ordinary actions and combat-owned
accepted casts. The picker pauses and owns all input, including its dismissal
click. Seven scrollable icon/name/cost rows feed a detail column. Remember cursor
and offset per character. Unavailable entries explain why and cannot be chosen.
Back/B clears assignments and restores the picker position; Cancel/Escape clears
the pending cast without costs. Targeting is live and exclusively consumes action
input. Single-target clicks commit immediately; other modes require Cast/Enter.
Submission validates once, consumes its event batch once, and clears previews.
Resolving follows existing caster presentation holds, not a new clock.

Focus loss suspends input and preserves pending choices. Fresh input is required
on return. Remove only invalid recipients, returning their darts to the unassigned
pool. World/turn/caster changes and movement cancel. Inspection, menus and history
pause without discarding valid pending choices. No mouse/keyboard desktop control.

## Presentation

Compact two-column picker above slot 4; portrait rail and combat log stay visible.
Casting replaces actions with a roughly 60%-width dock, leaving four wrapped log
lines. Fixed Cast, Back, Cancel positions are shared across modes. Gold outer
caster border coexists with teal recipient portrait frame/checkmark. Ally chips
remove recipients. Preview buffs say “Will receive …”, distinct from live effects.
Maximum selection never auto-casts or replaces another recipient.

Magic Missile begins empty: left-click adds one dart, right-click/Shift-click removes
one. At three, explain that removal is required. Thin visible-body outlines and
constant-size icon/count badges replace ID rectangles; only hover has detail text.
Keyboard Left/Right navigates, Up adds/toggles, Down removes, Enter confirms.

Area preview and resolution share a pure query: three candidate cells with legal
flags and at most twelve living actors, stable slot order, existing front-band LOS
rules. Shared recipient eligibility supports partial selections; complete command
validation retains the resource/turn/effect checks. No spell semantics change.
Floor preview uses canonical floor projection, faint fill and thin borders, with
wall depth clipping before NPC composition. Picking remains untouched. Area count
includes body-occluded occupants, but outlines never reveal hidden silhouettes.

## Storage, assets and interfaces

Fixed snapshots hold three assignments, four recipient flags/reasons, twelve area
actors and bounded feedback. No frame-time allocation. Renderer accepts an optional
bounded floor preview and includes its changes in cache invalidation. UI uses the
existing visible-body picking mask for a single bounded outline scan.
Seventeen individually generated painted icons (fourteen spells and three ability
entries) extend startup atlas preparation through typed spell/feature mappings.
Retain current portraits/dungeon art and store generated sources/prompts in-repo.

## Verification

Test input ownership, scrolling/back, eligibility, target limits, per-dart changes,
single/double submission, cancellation resources/RNG, focus edges, invalid target
pruning, and resolving. Compare area query/commit across headings, walls, protected
and occluded occupants. Check floor occlusion, picking invariance and cached-preview
removal. Extend first/repeated frame allocation checks. Run Debug/Release suites,
focused sanitizers and hidden GPU checks. Inspect deterministic captures at native
1920×1080 and reduced 1280×720 for all modes, errors and focus preservation.
Implementation stopped at Ready for review; the user subsequently accepted the
slice on 2026-10-27. Unavailable checks remain documented.

## Implementation details

`MagicHud::pending` is independent of `HudSnapshot::selection`; the latter remains
ordinary actions. `CastingState` is authoritative; open/armed are derived flags.
`spellRecipient` checks one partial candidate; complete validation still owns
costs, readiness, duplicate/count checks and effect capacity. `spellArea` returns
three cells/eligibility flags and up to twelve living occupants in stable order.
No RNG or mutation occurs during queries. New rejection reasons distinguish dead,
armored, non-dying and wrong-cell recipients without changing spell acceptance.

`SceneRenderer` prepares a 141,000-byte cell/border mask and 1,128-byte row-depth
array during construction, using the same floor mapping as scene baking. Only an
active eligible footprint enters the bounded tint pass; walls clip by depth and
NPCs then composite normally (translucent edges can reveal tinted floor). No bank
format changes. FramePipeline compares cell flags/positions/kind for cached-frame
invalidation. Area changes do not alter the picking buffer. Visible-body outlines
use one bounded viewport scan; twelve fixed marker rectangles avoid badge overlap
without changing body hit targets or exposing occluded actors.

The original revision grew the UI atlas from 11 to 28 entries. The subsequent
[native-resolution correction](native_hud_design.md) replaces that low-resolution
atlas with a 2048×1024 atlas and renders all HUD content at native resolution. Typed SpellKind/Feature mappings select icons;
unknown portraits retain their placeholder. Generated PNG sources and exact prompts
live in `assets/ui/spells/`; loading/resizing/upload occurs only at startup.
Log width changes rewrap by logical message position using fixed scratch buffers.

Actual results and retained captures: [UI verification and handoff](slice10_ui_verification.md).

## Review polish: badge stability and spacing (2026-10-27)

Implemented and accepted with slice 10 on 2026-10-27. Keep body-derived anchors for hit feedback, but provide separate
badge coordinates from the shared actor projection and canonical idle geometry.
Picking still gates visibility; pose alpha bounds must never reposition badges.
This uses fixed per-slot coordinates, no temporal smoothing, retained pointers or
allocations. Actor/camera travel moves anchors normally. Regress with visibly
different idle silhouettes, movement and complete occlusion.

Inset spell-row images within the row border, vertically center header text using
existing ink metrics, separate portrait and active-card borders, and add breathing
room above/below action/log content without reducing the four-line log capacity.
Verify with hidden native/reduced captures and focused CPU/allocation checks.

`TargetAnchor` keeps its original visible-body fields and adds `badgeX/badgeTop`.
`PickingBuffer` reuses a 256-slot coordinate array (2,048 bytes); NPC composition
sets projected positions from fixed front-idle geometry and current travel offsets.
The visibility scan publishes badge coordinates only for visible bodies. Badge
collision layout uses these stable coordinates. No prepared-bank format changes.
See [polish verification](slice10_ui_verification.md#review-polish-badge-stability-and-spacing).

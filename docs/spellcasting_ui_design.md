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

## Class Features presentation

Implemented correction, **accepted and closed** (2026-10-27). `CommandController` derives the
picker's Spells/Class Features identity from class casting capability, independent
of remaining mana. Its bounded HUD entries distinguish spells, activated features
and automatic features; passive identities never become gameplay commands.
Capacity is 22: at most 16 class spells, three activated features and three passive
entries. Immutable descriptions and stack formatting retain allocation-free refresh.
No catalog, save, combat cost or action-economy changes are involved.

Non-casters use CLASS FEATURES, no mana header, and one new native-resolution laurel
badge in slot 4. The fighter lists Second Wind plus selected Defense; the rogue lists
Sneak Attack and rear-row melee. Automatic rows are readable, never disabled-looking,
and mouse/Enter only selects details. Their text explains existing conditions.
Active features show labelled uses rather than numeric mana; Second Wind shows one
regular action and remaining uses, including on the confirmation dock. Casters retain
their spellbook and mana, with feature-specific resource text for their utility entries.
Exploration shows the existing six-second character cooldown instead of a combat
action, including Arcane Recovery's existing readiness cost; this corrects the old
"no action" recovery hint without changing its execution. End concentration remains
free. The Second Wind button uses the small text style so its full name and Enter
shortcut fit the shared button bounds.
The details rectangle owns all text below the header, including a generic empty state.
Seven-row scrolling, saved positions, input capture and cancellation remain shared.

Verify passive mouse/keyboard browsing cannot commit, resource/action costs and
failure paths are unchanged, empty/long lists stay bounded, and exhausted casters
retain spell identity. Inspect hidden native/reduced fighter, rogue, caster and empty
captures; run command/HUD/spell, allocation and GPU checks. Slice 10 remains accepted;
this correction is now also accepted and does not advance slice 11.

## Class Features verification

**User acceptance:** Recorded on 2026-10-27. The user described the result as
“very nice” and requested that acceptance be recorded. This closes the Class Features
correction, including the layout fix, non-caster labels/icon, passive browsing and
resource clarity. The automated verification below remains distinct from user review.

- Debug build and full CTest: **26/26 passed**, 56.30 s. Final focused spells,
  commands, HUD and allocation checks after tooltip/button refinements: **4/4 passed**,
  14.33 s. Hidden GPU lifecycle/native-atlas checks passed with the 29th icon.
- Generated fixtures cover passive mouse/keyboard inspection, unchanged resources
  and clocks, full-health/used Second Wind, cancellation, one regular action/use
  at confirmation, duplicate rejection, immediate caster switching, zero-mana caster
  identity and an empty legacy class. A ten-entry mixed spell/feature list verifies
  bounded scrolling and Back restoration. First/repeated passive browsing passes
  the engine allocation guard.
- Hidden application captures cover fighter and rogue in exploration and initiative,
  full-health/injured/used Second Wind, Defense, both rogue passives, the generic empty
  state and the wizard utility feature at native and reduced resolution. The empty
  capture is explicitly a presentation-only diagnostic; empty-class behavior also has
  a separate CPU fixture. Agent-performed interactive desktop review was not run;
  no OS input or focus automation was used. User acceptance is recorded above.
- Final capture export completed with **274 PNGs**, including 48 feature views and
  63 reduced views. Selected native/reduced views were inspected, not every export.
  Nine final images are retained in [class-features](screenshots/class-features/), including
  [rogue browsing](screenshots/class-features/feature-2-combat-book.png),
  [Second Wind confirmation](screenshots/class-features/feature-0-combat-confirm.png),
  [exhausted uses](screenshots/class-features/feature-0-combat-used.png), and the
  [bounded empty state](screenshots/class-features/feature-2-explore-empty-1280.png).
- The laurel emblem was generated with built-in imagegen, saved as
  [class_features.png](../assets/ui/class_features.png). Its exact final prompt and
  original generation path are recorded in [UI artwork provenance](../assets/ui/prompts.json).
  Existing master artwork was preserved, and atlas memory remains 8 MiB.

Commands:

```sh
cmake --build cmake-build-debug -j 4
ctest --test-dir cmake-build-debug --output-on-failure -j 4
ctest --test-dir cmake-build-debug --output-on-failure -j 4 -R '^(spells|commands|hud|frame_allocations)$'
cmake --build cmake-build-debug --target blobber_gpu_smoke
cmake-build-debug/blobber_gpu_smoke
cmake-build-debug/blobber --capture-spells /tmp/blobber-class-features-final
```

# Slice 12.3: menu and launch

Status: implemented, **accepted and closed**, 2026-10-29. The user accepted the
completed slice, including formation dragging and search-caret corrections.

The application owns the catalog, window, presenter and persistent library. Menu
startup does not load dungeon data. Dungeon preparation and live run ownership are
separate explicit transitions. Run borrowers must die before map/catalog/render
owners; GPU resources die before the window. Loading and disk transactions may
allocate; ordinary menu input, filtering and rendering use bounded storage.

A graphics-independent menu model owns search/filter state, 128 visible record
indices, stable UUID selection, four tagged saved/preset choices and a formation
permutation. Library snapshots remain owned by CharacterLibrary. Delete captures
identity/revision before confirmation and defaults to Keep. Committed durability
warnings reflect the committed snapshot and require reload. Failed writes retain
cards; corrupt stores and recovery artifacts are never automatically replaced.

Main menu has Start new game, Character Creator, Quit. Library supports read-only
review and confirmed deletion. New character controls remain deferred to 12.4.
Picker requires four distinct identities; selecting an assigned member swaps slots.
Default party is staged without library mutation. Launch revalidates saved records,
constructs a candidate session and publishes only after complete success. Party
construction alone grants three kits. Immutable launch builds/formation/seed support
defeat restart independently of library changes. Pause adds confirmed Main Menu,
default Stay, while retaining Quit Game.

Presentation follows approved screens 01–04 and read-only 13 at native 1920×1080.
Menu input owns text editing independently of gameplay shortcuts. Modal transitions
consume activation and preserve held-input suppression. Diagnostics bypass the real
library. Menu captures use temporary deterministic fixture libraries.

Validation: headless model, persistence failure and launch/lifecycle tests; full
Debug/Release suites, focused sanitizers, allocation guard, hidden GPU lifecycle and
native/reduced/letterboxed captures. No OS input automation. Stop at Ready for review;
12.4 requires separate acceptance and planning.

## Implemented interfaces and ownership

`MenuModel` owns bounded filter indices and four tagged choices in formation-slot
order. Resolution constructs an owning PresetParty with identity formation; slot
swaps never change the saved UUID. `MenuView` caches focus, detail offset and at most
160 button rectangles/action values. Menu rendering borrows catalog/library data
and uses native fonts/portrait atlases. `runMenus` performs explicit library I/O and
owns the screen transition loop. `launchMenuParty` prepares map, NPC metadata and
immutable scene resources before calling gameplay with the owning starting builds.
The existing PlayView owns session, animation, frame pipeline and commands. It
reconstructs from the same immutable builds/seed on defeat. Returning destroys the
run and dungeon owners before the menu resumes.

The menu background is a separate generated text-free PNG with recorded provenance.
`Presenter::prepareMenu` runs before menu frames and adds one UI texture (seven of eight texture slots total); two
presentation targets remain in use. Its one-time decode/upload is preparation,
not steady-state rendering. Native menu output is bilinear fitted; dungeon output
retains its point-filtered base. No frame image resizing or dungeon preloading.

Search is bounded printable ASCII (64 bytes); filters cycle the four supported
races/classes. Content frames use bounded linear scans. Explicit library writes,
launch/restart, exception reporting and capture export may allocate. No change to
catalog/library formats, combat capacities or persistent starting-build ownership.

Read-only review lists final abilities, HP/AC, weight, class features, mana,
class/racial casting statistics, spell grant categories and equipped/carried items.
Long detail lists scroll independently of fixed navigation. Creation controls remain
omitted until 12.4. The future caller context consists of the originating screen,
active formation slot and retained MenuModel; saving must not auto-launch or replace
an occupied slot.

See [verification and next-agent handoff](slice123_verification.md).

## Review correction: direct formation drag and search focus

Implemented and accepted. Replaces pointer-based slot selection/Assign buttons with dragging an
available character card onto any full formation card. Dragging a filled formation
card rearranges it; dropping an already assigned identity swaps slots. Assignment
commits on release over a valid slot only. A small movement threshold distinguishes
a click from a drag; outside release, Escape, right-click, blur or screen changes
cancel without mutation. Fixed drag state captures stable identity, not row index.
Hovered slots highlight and a portrait follows the pointer. Remove remains explicit.
Keyboard card/slot focus retains the existing assignment path without Assign buttons.

Search becomes a left-aligned text field with a blinking insertion caret. Blink
resets on focus/edit; long queries show the trailing text and caret inside the field.
Clicking elsewhere, navigation or window blur clears text focus. Both library and
picker share this field. CPU tests cover drag commit/swap/cancel/stale identity and
caret timing; hidden captures inspect drag targets, ghost and focused search.

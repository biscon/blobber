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
`Presenter::prepareMenu` runs before menu frames and adds the background and
title UI textures; two presentation targets remain in use. Their one-time decode/upload is preparation,
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

See [verification and next-agent handoff](verification/archive/slice123_verification.md).

## Stone & Shadow title artwork — 2026-11-08

Status: **artwork accepted; implemented and mechanically verified**. The user approved the gold
`Stone & Shadow` wordmark and requested removal of the Continue location/party
summary plus a centered, brighter navigation hint below the controls. Source
artwork and exact built-in imagegen prompt are durable assets under `assets/ui/`.

`Presenter::prepareMenu` calls `UiArt::prepareMenuTitle` before menu frames.
`UiArt` owns one `GpuLease<TextureHandle>` backed by the existing registry. Decode,
alpha-border selection (`.1f`), crop and bilinear texture upload occur only in
preparation. Cropping to 2058×270 reduces the upload from 6.00 MiB to 2.12 MiB;
preparation may temporarily hold the full 2172×724 decoded image. Empty/invalid
artwork fails explicitly; repeated preparation reuses the existing texture.
Registry exhaustion remains an explicit preparation error. The lease releases
the texture with the presenter before the window closes.

`drawMenu` borrows `UiArt` and draws the image at the approved native rectangle
`{450, 165, 1020, 134}` without Continue, or `{450, 88, 1020, 134}` with Continue.
`menuLayout` owns those integer pixel destinations. The title is only drawn on
the main menu; submenus keep their current headers. Ordinary drawing creates no
CPU or GPU resources. The unused cached Continue summary and its startup
formatting are removed. Save inspection and Continue availability retain their current rules.
The navigation hint is centered in the menu's 220-art-unit column, retaining its exact
24 px atlas and integer glyph positioning, using bright warm ivory with a one-pixel
dark shadow for contrast against the floor. Background, controls, fixed native
presentation and persistent data remain unchanged.

Validation selection: one Debug game build and existing headless menu test, plus
a focused hidden production capture runner for both Continue layouts and a
submenu. Check idempotent preparation, engine-owned C++ frame allocations, stable
GPU counts/uploads, texture release and OpenGL errors. Export native and reduced
captures for visual review; no OS input or user library/save access is needed.

The separately requested window caption correction removes ` | Blobber` from
`Window::Window`, leaving `Stone & Shadow`; it needs only a focused build and
hidden-window initialization check, with no gameplay test matrix.

Initial preview and caption build passed; the user accepted the artwork before
integration. The original substitution runner and two preview images are now
superseded by the [production runner](../tools/review_menu_title.cpp) and
[five production captures](screenshots/menu-title-preview/README.md).

Final verification commands (low priority, at most two build workers and one test):

```sh
nice -n 15 ionice -c 3 cmake --build cmake-build-debug --target blobber blobber_review_menu_title blobber_menu_tests --parallel 2
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-debug -R '^menu$' -j 1 --output-on-failure
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_menu_title cmake-build-debug/assets NEW_CAPTURE_DIRECTORY
```

All passed. The hidden runner verified failed title-upload retry without leaked
textures, idempotent preparation, six alternating Continue/no-Continue frames
with zero engine-owned C++ allocations, stable GPU texture/target counts and
uploads, title lease release, and no OpenGL errors. Its C++ guard excludes C and
driver allocations. Total added prepared-menu GPU bytes: 6,942,696 (background
and cropped title). Native and 1280×720 images for both menu layouts, plus a
native library submenu, were visually inspected. The summary is absent, the
brighter hint is centered, and the image title does not appear on submenus.

Display access outside the sandbox was needed, as established by the initial
preview's failure to open `:0`. The capture window remained hidden throughout;
no desktop input/focus automation or interactive review was performed. Artwork
acceptance is preserved; final interactive review remains with the user. Selected
images, artwork and prompt are retained; disposable logs/captures were cleaned.

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

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

## Metallic gleam — 2026-11-08

Status: **implemented and mechanically verified; motion review pending**. Combine a soft
diagonal metallic sweep, emphasis on the artwork's brighter bevels and restrained
warm torchlight shimmer between sweeps. Keep the generated texture and native
layout. The sweep lasts two seconds, beginning after one second and repeating on
a twelve-second cycle (ten seconds between sweeps). The slow warmth modulation
is continuous, deterministic and seamless at the cycle boundary.

`UiArt` owns the texture and one shader through registry-backed RAII leases.
`src/ui/menu_title.cpp` contains title preparation, shader source and drawing;
the generic art file retains other UI artwork responsibilities. Preparation
stages both resources transactionally, validates the cached uniform location,
and publishes only on complete success. Failed shader compilation/acquisition
releases the staged texture; retry is supported. Missing/invalid artwork retains
the existing explicit error behavior. Repeated preparation reuses both resources.

`runMenus` owns the decorative clock, storing elapsed seconds in `MenuView` before
each frame. Drawing receives that explicit time, enabling deterministic captures
without changing the OS clock or injecting input. `UiArt::menuTitle` folds double
precision elapsed time into a bounded twelve-second phase, treating negative or
nonfinite diagnostic times as zero, then updates one cached shader uniform.
The shader samples the original logo, uses luminance as an approximate bevel
mask, adds a soft gold band with a narrow champagne highlight and applies a small
warm color variation. It preserves source alpha. Shader scope surrounds only
the logo draw and restores the default before controls/text are drawn. There is
one logo draw, no additional render target/pass, no frame allocation, and no new
texture uploads during ordinary frames. All fonts retain native atlas sizes.

The registry's existing eight shader slots cover seven world programs (including
AO and billboards) plus the persistent title program. Capacity and exhaustion
behavior remain unchanged. Check coexistence using a tiny generated world with
the menu resources still alive; this is important for menu-to-game transitions.

Validation: one Debug game/review build, focused existing menu CPU test, hidden
OpenGL preparation failure/retry, idempotency, phase/cycle and off-logo pixel
invariants, resource release and engine-owned C++ allocation checks. Use fixed
times in both menu layouts and a submenu; export a bounded twelve-second reduced
animation for visual timing review. Driver/C allocations are excluded from the
C++ guard. No OS input/focus automation or user save/library access.

Verification completed in Debug with the existing `menu` CPU test and the opt-in
`blobber_review_menu_title` runner. The game and runner build with two workers;
the CPU test passed. Hidden OpenGL checks passed for title texture and shader
acquisition failure/retry, atomic resource publication, preparation reuse, exact
twelve-second repetition, negative/NaN/infinite diagnostic time handling, exact
off-logo pixel preservation and visible logo changes during the sweep. A 7×7
generated interactive world exercised all seven world shaders alongside the
title shader in the unchanged eight-slot registry. Returning to menu drawing
produced identical pixels at the same phase. Six alternating menu frames had
zero engine-owned C++ allocations and stable texture/target/shader counts, bytes
and texture uploads. Destruction released both title leases. No OpenGL errors.
The first coexistence attempt used a geometry-only fixture and correctly rejected
its missing interactive effect clips; the final check uses the existing complete,
tiny generated visibility fixture. No production world assets are used for this
coexistence check.

The [gallery](screenshots/menu-title-preview/README.md) now retains five production
PNGs plus a 768×432 animated WebP: twelve seconds at ten frames per second, with
the complete rest/sweep cycle. Native and reduced title captures and selected
motion frames were inspected. The single-threaded low-priority export command is:

```sh
nice -n 15 ionice -c 3 ffmpeg -hide_banner -loglevel error -threads 1 -filter_threads 1 -filter_complex_threads 1 -framerate 10 -i NEW_CAPTURE_DIRECTORY/frames/frame-%03d.png -c:v libwebp_anim -lossless 1 -compression_level 4 -q:v 100 -loop 0 -threads 1 menu-gleam.webp
```

Run the capture command from the preceding section with `TMPDIR` set to the
task-owned agent run so the generated world fixture stays inside that run.
The encoded animation was decoded to confirm dimensions, 120 frames, twelve
seconds and infinite looping. Raw frames, phase-check files, logs and the failed
capture directory were cleaned after selecting evidence. Artwork acceptance
remains unchanged; animated appearance/timing awaits user review. Interactive
desktop review and third-party/driver allocation measurement were not performed.

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

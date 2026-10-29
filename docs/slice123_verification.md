# Slice 12.3 verification and handoff

Status: **Accepted and closed**, 2026-10-29. The user explicitly accepted the
slice: “slice accepted, record it in the doc”. Acceptance includes the formation
dragging and search-caret review corrections.

## Delivered

Normal startup opens Main Menu without loading the map, NPC definitions, dungeon
banks or a GameSession. Start new game opens the formation picker; Character
Creator opens the saved library; Quit closes the application. The library supports
bounded search, race/class filters, scrolling, read-only details and confirmed
UUID/revision-based deletion. Built-in defaults work independently of saved records.

Launch reloads saved records and requires renewed review after a revision change.
An explicit Loading frame precedes synchronous dungeon preparation. Exceptions
return to the picker with choices preserved. GameSession receives copied starting
builds; Party alone grants three camp kits. Defeat restarts those original builds
and seed, without rereading or writing the library. Pause includes Main Menu with
a discard confirmation; Enter/Escape keep the current run. Quit Game remains.

New character controls are deliberately omitted until 12.4. No creator, rest
repreparation, XP progression or run persistence is delivered by this slice.

## Verification

- Full Debug and Release CTest: 35/35 passed. New `menu` suite uses generated
  creation fixtures and covers full filtering, identity-preserving swaps/deletion
  snapshots, stale choices, all-one-class parties, defaults independent of the
  library, fresh health and exactly three kits per constructed party.
- Focused Debug menu/HUD/commands/allocation checks: 4/4 passed. The allocation
  guard includes repeated menu filtering, selection and confirmation snapshots.
- Focused ASan/UBSan menu/HUD/commands: 3/3 passed with `detect_leaks=0`.
  Leak sanitizer was not run. Existing command aggregate-initializer warnings remain.
- Hidden RTX 3060 GPU lifecycle check passed. Menu capture mode exports native,
  1280×720 and letterboxed 1024×768 images using an isolated temporary library and
  deterministic UUID entropy. No actual user library is resolved in capture mode.
- 37 review images cover main/library/detail/delete, ready/incomplete formation,
  many-row and no-result lists, a displayed launch error, custom-party launch,
  reconstruction and pause/discard screens. Launch/reconstruction images match
  byte-for-byte. Selected images were inspected for clipping, portrait identity,
  controls and letterboxing.

These checks use synthetic state and in-process calls, not OS input injection.
The launch-error image is a presentation fixture; it is not evidence of an
injected filesystem failure. Library transaction failures remain covered by the
existing fixture-owned `library` suite. Interactive desktop use is unverified.

## Retained review images

[Main menu](screenshots/slice123/main-1280.png),
[library](screenshots/slice123/library-many-1280.png),
[no results](screenshots/slice123/library-no-results-4x3.png),
[party picker](screenshots/slice123/picker-ready-1280.png),
[delete](screenshots/slice123/delete-4x3.png),
[wizard review](screenshots/slice123/detail-wizard-1280.png),
[custom-party launch](screenshots/slice123/launch.png),
[pause menu](screenshots/slice123/pause-menu.png),
[discard confirmation](screenshots/slice123/discard-run.png).

## Reproduction and review

```sh
cmake --build cmake-build-debug -j4
ctest --test-dir cmake-build-debug --output-on-failure -j4
cmake --build cmake-build-release -j4
ctest --test-dir cmake-build-release --output-on-failure -j4
cmake-build-debug/blobber --capture-menu /tmp/blobber123-review
cmake --build cmake-build-debug --target blobber_gpu_smoke -j4
cmake-build-debug/blobber_gpu_smoke
```

Run `cmake-build-debug/blobber` for human review. Browse the library, type in Search,
cycle filters, View a character and cancel a deletion. Start new game, choose a
character card and drag it into a formation slot; dropping an already selected character swaps slots.
Remove clears a slot. Use default party stages the built-ins without a library
write. Enter Dungeon launches the current test level. In Pause, choose Main Menu,
then Stay or Discard Run. Check Quit Game separately. Tab/arrows/Enter navigate;
Escape returns or keeps a confirmation; wheel and Previous/Next scroll lists.

## Next-agent contracts

`MenuModel` has fixed storage and uses UUIDs/preset indices, never library positions
as durable identity. Choices are stored in formation-slot order; resolution emits
an identity formation permutation for those copied builds. `MenuView` holds focus,
search/detail scroll and fixed button geometry. Rendering contains no filesystem
operations. `runMenus` owns explicit I/O and transitions; `launchMenuParty` owns
map/prepared resources through the synchronous gameplay call. Borrowers die before
their owners. Main Menu teardown releases dungeon resources; presenter/UI assets
persist until application shutdown.

For 12.4, preserve the originating MenuScreen and activeSlot while editing an
isolated draft. On save, refresh the fixed library snapshot, select the new UUID
and restore the caller without replacing an occupied slot or launching. Existing
library errors/committed flags must remain authoritative. The library schema,
portrait IDs, spell-grant rules and run-save separation are unchanged.

Legacy demo/capture modes omit the Main Menu return control and retain their deterministic fixture/default-party routes
and never initialize the user's library. Menu capture uses a unique `/tmp` library.
Normal library paths follow the accepted XDG/HOME policy. Recovery artifacts are
not repaired automatically. Slice 12.3 is accepted; plan 12.4 separately before implementation.

## Review correction: formation dragging and search caret

Implemented and accepted. Available cards now drag directly onto the full
formation cards; filled formation cards also drag to move/swap. Assign buttons and
clickable slot-heading buttons are removed. A floating portrait and teal drop-target
outline show the pending operation. Drop outside, Escape, right-click and focus
loss cancel; no party mutation occurs before a valid release. Remove stays explicit.
Search is a left-aligned field with a blinking caret in both library and picker.
Focus/edit resets blink, long queries scroll their displayed suffix, and navigation,
clicking elsewhere or window blur releases text focus.

Focused Debug and Release `menu|frame_allocations`: 2/2 each. ASan/UBSan `menu`:
1/1 (`detect_leaks=0`). Tests cover pointer press/move/release, threshold clicks,
assignment, swaps, outside release, Escape, blur, stale source rejection and caret
timing. Allocation guard includes repeated drag/drop and caret updates. The earlier
35/35 full suites above record the preceding slice baseline, not a new full-suite run.
Hidden in-process capture run: 49 images, native/reduced/letterboxed; inspected drag,
search and target layouts. Caret on/off image differences are confined to the search
field. No OS input automation or interactive desktop verification.

[Dragging onto a slot](screenshots/slice123/picker-drag-1280.png),
[dropped character](screenshots/slice123/picker-dropped-4x3.png),
[caret visible](screenshots/slice123/search-caret-on-1280.png),
[caret blink-off](screenshots/slice123/search-caret-off-1280.png).

`MenuInteraction` is graphics-independent fixed drag/focus/blink state. Shared
`menuLayout` rectangles drive hit testing and rendering. Drag state owns a tagged
choice identity; drawing borrows its current portrait. Keyboard card/slot activation
retains assignment without the removed pointer Assign buttons. Accepted on 2026-10-29.

## Accepted closeout

The user accepted and closed slice 12.3 on 2026-10-29, including all formation
assignment and search-focus corrections. The recorded automated checks and their
limitations are unchanged; acceptance does not claim additional tests or automated
interactive desktop verification.

Slice 12.4 is next eligible for planning and remains Planned/unstarted. In a fresh
context, read this handoff, the system overview, roadmap and approved creator
references, then inspect current code and propose a plan for 12.4 only. Obtain
approval of that plan before implementation. Preserve direct formation dragging,
search focus/caret behavior, independent library/run ownership, checked library
transactions and prepared rendering/allocation contracts. This documentation-only
closeout does not begin 12.4 or advance milestone C to accepted.

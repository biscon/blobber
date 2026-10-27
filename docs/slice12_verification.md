# Slice 12 verification and handoff

Status: **Ready for review**, 2026-10-27. Implementation and available checks are
complete. User acceptance is pending; do not start slice 13. The plan and camp
mockup were explicitly accepted before implementation. See the
[implemented design](loot_recovery_design.md) and [roadmap](dnd5e_demo_design.md#slice-12-loot-and-recovery).

## Delivered behavior

Deaths publish seeded gear, coins and optional potions exactly once; removing a
corpse cannot remove or recreate its rewards. Direct clicks acquire an entire
stack through staged inventory validation for the selected gameplay character.
The viewed Inventory/Character identity remains independent. Pickup requires a
conscious able recipient, stationary safe Exploration and current/three-front-cell
reach with corner checks. Failure preserves both owners. Six floor sprites per
cell and wheel paging expose crowded drops; hover explains quantity, recipient,
page and rejection. Walls, bodies and corpses occlude picking.

R or the pause menu opens exclusive camp. Short Rest spends the chosen Hit Dice
for class die + Constitution healing and refreshes Second Wind. Long Rest consumes
one kit outside authored sanctuaries and restores living HP/mana, half-maximum
Hit Dice (minimum one), Second Wind and Arcane Recovery. The latter remains a
manual ability. No rest resurrects dead allies; dying allies block it. Recovery,
cost and one/eight gameplay hours commit once, followed by two four-frame bonfire
loops and fade. Cancel before starting has no cost; Escape after starting skips
cosmetics only. Focus loss pauses cosmetics; NPCs do not catch up during rest.

Normal play starts with one kit in Alden's bag and has kits at (7,15)/(19,19), with
no sanctuary or random kit drops. `--recovery-demo` uses three separated encounters
and explicitly marks only its starting cell as a free sanctuary. Schema 6 and
bounded recovery metadata load strictly; older catalog versions remain supported.

## Checks and evidence

| Check | Result |
| --- | --- |
| Full Debug suite | 29/29 passed, 59.55 s |
| Full Release suite | 29/29 passed, 5.98 s |
| ASan + UBSan focused suite | Recovery, loot rendering, inventory, party content: 4/4 passed, 21.46 s |
| Final expanded recovery/render tests | Passed in Debug, Release (2/2, 0.85 s), and ASan/UBSan (2/2, 20.20 s) |
| Engine frame allocation guard | Zero tracked C++ allocations, including first death activation, 64 pickup/rest cycles, hover/paging, prepared floor composition and camp commands |
| Separate Release capacity stress | 256 deaths × eight rewards plus 64 authored stacks (2,112 total), collect/reconcile exhaustion; full-capacity floor composition passed |
| Offline loot bake | Repeated production bake is byte-identical; generated-fixture bank round trip/corruption rejection passed |
| Production prepared check | Recovery-demo metadata/placements validated; resident prepared data 183,683,048 bytes under 536,870,912-byte limit |
| Hidden GPU lifecycle/native UI | Passed with generated camp/loot fixtures and original native-detail assertions |
| Hidden application captures | 63 PNGs: 21 each at 1920×1080, 1280×720 and letterboxed 1024×768 |

The full suites preceded final additions to test coverage and capture sizing;
the affected recovery/render tests were rebuilt and rerun in all three configurations.
No gameplay/render behavior changed after those full-suite runs. A final CMake
link dependency explicitly connects level recovery loading to core world validation.

Tests own catalogs/maps/generated images. Coverage includes selected dice and HP
caps, stable/dying/dead members, exhausted kits/dice, no-benefit/foreign/replayed
requests, dead-bag supply ownership, cooldown/effect expiry, frozen NPC positions,
settled action presentation, cancellation/focus/skip behavior, deterministic potion
categories and kill ordering, removal before reconciliation, duplicate/foreign/stale
pickup, selection versus viewed sheet, full-bag merge rollback and weight failure.
Rendering checks cover all headings, six sprites in front diagonals and against
adjacent walls, transparent picking, wall/body/corpse occlusion, cache invalidation,
paging, collection and disabled transition picks. Existing tests retain input
coordinate and held-edge coverage. Application captures route R, floor clicking and
wheel input through production routing, and exercise shared camp controller commands.

Corrections during verification: the enlarged fixed session data exceeded the
existing allocation test's stack frame, so its large fixture owners are now allocated
before tracking. Unsupported-schema tests now use version 7. Camp playback shows
healing results and remaining dice instead of stale selected/available controls.
Current-cell and diagonal sprite fitting preserves all six targets. An expanded
rest test confirms the existing one-second feedback tail must settle before camp.

Allocation claims cover engine-owned C++ allocation, not graphics-driver or all
C-library internals. LeakSanitizer was disabled; leak-specific diagnostics remain
unverified. Interactive feel and the complete encounter/recovery playthrough remain
for user review. No mouse/keyboard injection, focus changes or desktop automation
were used; captures use hidden application windows and in-process actions.

## Selected application captures

- [Short Rest and Hit Dice](screenshots/slice12/camp-short.png)
- [Bonfire playback and results](screenshots/slice12/camp-result.png)
- [Long Rest cost/source, reduced](screenshots/slice12/camp-long-reduced.png)
- [Letterboxed layout](screenshots/slice12/camp-letterbox.png)
- [Floor items](screenshots/slice12/loot-floor.png)
- [Corpse occlusion](screenshots/slice12/loot-corpse.png)
- [Loot after corpse removal](screenshots/slice12/loot-after-corpse.png)

The [capture manifest](screenshots/slice12/capture-manifest.json) records all 63
export names, dimensions and hashes. Full temporary sets are in
`/tmp/blobber12-native`, `/tmp/blobber12-reduced`, `/tmp/blobber12-letterbox`.
Generated masters and prompts remain in `assets/ui/camp` and `assets/ui/items`;
the approved concept mockup remains under `docs/mockups/slice12`.

## Reproduce

```sh
cmake --build --preset debug -j6
cmake --build --preset release -j6
ctest --preset debug --output-on-failure
ctest --preset release --output-on-failure
cmake-build-release/blobber_recovery_tests --stress
cmake-build-release/blobber_loot_render_tests --stress
cmake --build --preset debug --target blobber_gpu_smoke
cmake-build-debug/blobber_gpu_smoke
cmake-build-debug/blobber --recovery-demo --check-prepared
cmake-build-debug/blobber --capture-recovery /tmp/blobber12-native
cmake-build-debug/blobber --capture-recovery /tmp/blobber12-reduced --size 1280 720
cmake-build-debug/blobber --capture-recovery /tmp/blobber12-letterbox --size 1024 768
cmake-build-debug/blobber_bake_loot assets/ui/items/loot-sheet.png /tmp/blobber12-loot-rebake.bin
cmp cmake-build-debug/assets/prepared/loot.bin /tmp/blobber12-loot-rebake.bin
```

Sanitizers reused the pinned raylib checkout, without package/toolchain changes:

```sh
cmake -S . -B /tmp/blobber-slice12-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie' \
  -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/bison/Projects/blobber/cmake-build-debug/_deps/raylib-src
cmake --build /tmp/blobber-slice12-asan --target blobber_recovery_tests \
  blobber_loot_render_tests blobber_inventory_tests blobber_party_content_tests -j6
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/blobber-slice12-asan \
  -R '^(recovery|loot_render|inventory|party_content)$' --output-on-failure
```

## User review recipe and next-agent handoff

1. Run `cmake-build-debug/blobber --recovery-demo --seed 5`. Start at (3,21),
   face north and approach the first goblin at (3,15). Later encounters are at
   (13,19) and (18,17); the map overlay can help find them.
2. After an encounter, Continue. Hover and collect drops, switching the selected
   portrait to choose a recipient. Open another character's sheet, close it, and
   confirm the original selection still receives floor loot. Leave a drop until
   its corpse disappears and collect it afterward.
3. Press R in safety. Select Hit Dice with +/- and Start Rest. Confirm healing,
   depleted dice, unchanged mana and refreshed Second Wind. Cancel another preview,
   and try Escape after starting to skip the cosmetic animation.
4. Compare Long Rest away from spawn (one kit) with Long Rest at this demo's spawn
   (no kit). Inspect HP/mana/Hit Dice/class charges in Character. Normal gameplay
   has no spawn sanctuary. Try camp near a threat and inspect the rejection.
5. Check floor click targets, wheel paging, camp controls and readability at your
   preferred window size. Review the bonfire loop and fade interactively.

Address requested corrections within slice 12. Preserve accepted Character/Inventory
controls, prepared-image rendering and discrete movement. XP/levels, hub/shops,
player dropping, arrow recovery and persistence remain outside this slice. Record
explicit acceptance before marking slice 12 closed or planning slice 13.

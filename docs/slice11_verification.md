# Slice 11 verification and handoff

Status: **Ready for review**, 2026-10-27. User acceptance is pending.
The [inventory design](inventory_design.md) records the approved boundaries and
implementation. Slice 10 was accepted before this work began. Slice 12 remains
Planned and must not start before the user accepts slice 11.

## Delivered

Four independent 8×6 bags, separate body/main-hand/shield/ranged positions, typed
stack handles, weight enforcement and atomic move/swap/merge/split/transfer/equip
transactions. All live equipment consumers now use current inventory rather than
startup indices. Derived AC, attack/damage previews and versatile grip update;
equipping armor ends Mage Armor immediately. Schema 5 adds typed ammunition,
potions and stack limits while retaining strict schemas 1–4.

Healing and mana potions use checked live ally commands and existing action /
six-second readiness clocks. Healing revives living downed allies and clamps at
maximum HP; mana restores at most two. Rejected use preserves items and RNG.
Party bows spend one compatible arrow on acceptance, with no miss/interruption
refund. NPC ammunition remains abstract.

The full-screen paused overlay supports mouse drag, button actions, keyboard
navigation, description pages and Shift-drag default-one split confirmation.
Transfers/equipment require safe Exploration; bag organization works during paused
danger. A neutral silhouette and fifteen painted item icons use a prepared native
UI atlas. Existing portraits, dungeon rendering and movement are unchanged.
Original generated images and prompts are in [assets/ui/items](../assets/ui/items/).

## Checks

| Check | Result |
| --- | --- |
| Full Debug correctness suite | 27/27 passed, 115.84 s |
| Full Release correctness suite | 27/27 passed, 12.94 s |
| Focused Debug / Release | 8/8 each after refinements; final selection cleanup also passes inventory/allocation checks |
| ASan + UBSan | 6/6 passed: inventory, spells, effects, impact, HUD, party content |
| Allocation guard | Zero tracked C++ allocations in first/repeated inventory and existing frame paths |
| Separate Release stress | 100,000 split/merge reuse cycles, plus capacity/integration cases; 210 ms |
| Hidden GPU lifecycle/native atlas | Passed, including single-native-pixel item detail and padding |
| Hidden inventory captures | 41 PNGs; native 1920×1080, reduced 1280×720, letterboxed 1024×768 |

The full suites preceded the final UI refinements; affected suites were rerun after
them (Debug 8/8 in 14.40 s; Release 8/8 in 1.85 s). The final selection cleanup
passed inventory/allocation rechecks in both builds and the inventory sanitizer
recheck. Final application/GPU builds and all 41 captures also completed successfully. No further rendering, world or prepared-bank changes were made. The tests own
their catalogs and generated images; real-content diagnostic captures are separate.
Frame tracking covers engine-owned C++ allocations, not driver or all C-library
internals. Session fixture preparation allocates outside the guard. Leak detection
was disabled under the inherited sanitizer environment restriction and is unverified.
Interactive desktop review remains unverified and belongs to the user. No OS input,
focus changes, or external-window automation were used.

Reproduction:

```sh
cmake --build --preset debug -j6
cmake --build --preset release -j6
ctest --preset debug --output-on-failure
ctest --preset release --output-on-failure
cmake-build-release/blobber_inventory_tests --stress
cmake --build cmake-build-debug --target blobber_gpu_smoke -j4
cmake-build-debug/blobber_gpu_smoke
cmake-build-debug/blobber --capture-inventory /tmp/blobber11-captures --seed 5
```

Sanitizers reused the existing pinned raylib checkout:

```sh
cmake -S . -B /tmp/blobber11-sanitizers -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined' \
  -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/bison/Projects/blobber/cmake-build-debug/_deps/raylib-src
cmake --build /tmp/blobber11-sanitizers --target \
  blobber_inventory_tests blobber_party_content_tests blobber_effects_tests \
  blobber_impact_tests blobber_spells_tests blobber_hud_tests -j4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/blobber11-sanitizers \
  -R '^(inventory|party_content|effects|impact|spells|hud)$' --output-on-failure
```

## Coverage and corrections

- Item/weight conservation across all four full bags, row-order transfer, merge-only
  full-bag success, partial-merge failure rollback, exact stack/weight boundaries,
  equipped ownership, full-bag replacement and formation-independent ownership.
- Stale revisions, foreign handles, consumed/merged generations, repeated slot reuse,
  invalid equipment and legacy starting-loadout normalization/capacity failure.
- Defense/versatile recalculation, immediate Mage Armor ending, potion revival/caps,
  dead/noncaster/full-resource rejection, shared cooldown, turn feedback handoff,
  last-arrow acceptance, interrupted projectile costs and rejected-request RNG parity.
- Drag/button split parity, default one/no early mutation, cancellation, focus-held
  pointer isolation, stale dialog confirmation and keyboard destination commits.
- Schema 5 malformed/missing stack/ammunition/effect fields, strict legacy fields,
  and runtime conversion of large legacy quantities.

The allocation harness initially exceeded its stack after adding another session;
its inventory fixture now uses explicitly prepared heap storage before tracking.
Visual review corrected ineligible recipient checkmarks and potion/log overlap.
A stale drag-source case and premature keyboard-Tab commit were fixed. Split
previews share authoritative staging, including weight, before opening a dialog.
A GCC 16 warning in the existing three-target effect sort was removed with a bounded
insertion pass preserving canonical recipient/RNG order. Final builds emit no warnings. No toolchain or dependency installation was needed.

## Review

Run `./cmake-build-debug/blobber --inventory-demo --seed 5`. This opens Merrin's
bag on the ordinary map. I, Item/5, and the character inspector also open it.
Try splitting arrows, transferring via tabs, removing/replacing shields, and using
a potion after taking damage. Inspect another character during combat to check the
separation between viewed inventory and active actor; gear/transfers should reject.

Thirteen selected images are retained in [screenshots/slice11](screenshots/slice11/):

- [Fighter, native](screenshots/slice11/inventory-fighter.png),
  [rogue, reduced](screenshots/slice11/inventory-rogue-1280.png),
  [wizard, reduced](screenshots/slice11/inventory-wizard-1280.png),
  [letterboxed presentation](screenshots/slice11/inventory-fighter-4x3.png).
- [Default-one split](screenshots/slice11/inventory-split-default.png),
  [reduced split](screenshots/slice11/inventory-split-default-1280.png),
  [shield removed](screenshots/slice11/inventory-shield-removed-1280.png).
- [Full bag](screenshots/slice11/inventory-full-bag-1280.png),
  [description page two](screenshots/slice11/inventory-description-page2-1280.png),
  [unsafe equipment rejection](screenshots/slice11/inventory-danger.png).
- [Potion targeting](screenshots/slice11/inventory-potion-targeting-1280.png),
  [healing result](screenshots/slice11/inventory-healing-result-1280.png),
  [mana potion](screenshots/slice11/inventory-mana-1280.png).

The full-bag captures use a diagnostic catalog clone with 100 starting arrows and
long fixture text, then fill the bag through production splits. Normal starting
content remains twenty Rogue arrows. Native/reduced examples were inspected for
art, text, quantity badges, modal coverage, feedback and letterboxing.

No loot, arrow recovery, resting, progression, save/resume, item dropping or layered
clothing is delivered. See the [source/adaptation ledger](srd_content_ledger.md#slice-11-inventory-ammunition-and-potions).
After user acceptance, record it in the checkpoint/tracker/execution record before
planning slice 12. Do not infer acceptance from these checks or captured images.

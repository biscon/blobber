# Native HUD and impact-alert verification

Status: **Accepted and closed**, 2026-10-27. The user reported that the result looks
good and works well, and explicitly said “I accept the slice, close it in the doc”.
This closes slice 10 including all UI and impact-alert corrections. Slice 11 remains
Planned/unstarted.
[Design and ownership](native_hud_design.md).

## Delivered

The entire HUD draws directly at 1920×1080. Original portraits and icons are
prepared for their actual native display sizes, without the old threefold pixel
enlargement. The dungeon still uses prepared pixel images. Thin native borders,
selection indicators, framed spell-row icons and a new eight-tooth gold cog share
one ordered layer with text. Portraits preserve their proportions; icons fit their
bounds. Modal fills cover artwork and text together. Logical layout/hit regions
remain unchanged.

Delayed outgoing weapon attacks and spells provoke local alerts at impact,
including misses. Alerting before damage lets lethal hits warn surviving neighbors.
Canceled attacks and absent targets do not alert. Magic Missile recipients resolve
at their own arrival times, with existing local propagation. Normal sight/contact
detection remains active independently. Instant hostile effects still alert on
their immediate resolution. Costs, rolls and damage timing are preserved.

## Executed checks

| Check | Result |
| --- | --- |
| Debug full CTest | 26/26, 106.98 s |
| Release full CTest | 26/26, 12.36 s |
| ASan/UBSan: spells, impact, hybrid, HUD, commands | 5/5, 0.17 s |
| First/repeated frame allocation guards | Passed in both full suites |
| Hidden GPU lifecycle and generated native-art fixture | Passed, NVIDIA RTX 3060 |
| Hidden spell captures | 231 PNGs, including 39 reduced captures |
| Hidden HUD captures | 73 PNGs, including 20 reduced captures |
| Seeded missile trace | Unchanged: two mana at t=0, fifteen damage at t=342 ms |
| `git diff --check` | Passed |

The generated GPU fixture owns its temporary images and directory. It verifies
one-native-pixel stripes survive atlas preparation and drawing, padded edges avoid
bleed, square icons fit wide destinations, portraits crop without stretching,
modal fills cover underlying content and borders are one native pixel wide.

Combat regressions exercise pre-impact unawareness, hits/misses, staggered missiles,
local flood boundaries, lethal hits, removed targets, paused flight, coarse updates,
independent sight detection and canceled melee weapon/spell windups. Existing
hybrid checks now begin approach-only death-save timing when impact starts approach.

Separate stress checks passed: 256 NPCs plus twelve spell recipients, 528 events,
100 bursts (mean 243.938 µs, max 391.581 µs); 100 area/effect-capacity cycles in
57.372 ms. Weapon stress drained 260 attacks/520 events for 100 bursts (mean
435.871 µs, max 648.650 µs). These local timings are observations, not guarantees.

Commands used:

```sh
cmake --build cmake-build-debug -j 4
ctest --test-dir cmake-build-debug --output-on-failure
cmake --build cmake-build-release -j 4
ctest --test-dir cmake-build-release --output-on-failure
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/blobber10-sanitizers --output-on-failure -R '^(spells|impact|hybrid|hud|commands)$'
cmake --build cmake-build-debug --target blobber_gpu_smoke
cmake-build-debug/blobber_gpu_smoke
cmake-build-release/blobber_spells_tests --stress
cmake-build-release/blobber_impact_tests --stress
cmake-build-release/blobber_spells_tests --trace
cmake-build-debug/blobber --capture-spells /tmp/blobber-native-ui-final
cmake-build-debug/blobber --capture-hud /tmp/blobber-native-hud-final
```

## Retained visual review

Selected native/reduced captures were inspected; not every PNG. Fourteen are retained
in [slice10-native-ui](screenshots/slice10-native-ui/), including:

- [Native spellbook](screenshots/slice10-native-ui/spell-magic_missile-book.png)
  and [1280×720 spellbook](screenshots/slice10-native-ui/spell-magic_missile-book-1280.png).
- [Bless selection](screenshots/slice10-native-ui/spell-bless-target-1280.png),
  [missile allocation](screenshots/slice10-native-ui/spell-magic_missile-target-1280.png)
  and [area preview](screenshots/slice10-native-ui/spell-burning_hands-sparse-target-1280.png).
- [Portrait inspector](screenshots/slice10-native-ui/hud-character-details.png),
  [pause menu](screenshots/slice10-native-ui/hud-pause.png),
  [history](screenshots/slice10-native-ui/hud-history-1280.png) and
  [hover feedback](screenshots/slice10-native-ui/hud-hover-reachable-1280.png).

Run `cmake-build-debug/blobber --spell-demo --seed 5` for interactive review. Check
the sharper portraits/icons, cog and picker frames; use the inspector and pause
menu to check layering. Shoot an unaware group facing away and verify attack-induced
pursuit starts after impact. A group that sees the party can still react earlier.

No OS mouse/keyboard/focus automation was used. The user completed review and
accepted the slice. Agent-driven interactive play and real high-DPI input checks
remain unverified. LeakSanitizer was disabled for the existing environment
tracing limitation; ASan/UBSan passed. Source-free startup was not rerun for this
correction; the prepared-bank format and source-file requirements did not change.

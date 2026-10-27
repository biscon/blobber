# Slice 10 verification and handoff

Status: **Ready for review**, 2026-10-27. Implementation was authorized with
“Implement the plan”. User acceptance is pending. Slice 11 remains Planned and
must not begin until slice 10 is accepted. See the [implemented design](spells_design.md),
[source/adaptation ledger](srd_content_ledger.md#slice-10-spells-and-class-identity)
and [headless trace](slice10_trace.txt).

## Delivered

Fourteen curated spells, schema-4 class spell lists, real mana and one-use class
resources, Life Domain healing/heavy armor, Rogue rear-row melee/Sneak Attack,
Second Wind and safe Arcane Recovery. Inflict Wounds replaces Healing Word as
approved. Racial features retain the existing numeric/training passives only.
There is no inventory, progression, resting, upcasting or second-level casting.

The paused spellbook resumes into live targeting. Single-target body/portrait
clicks cast; multi-target and area spells require explicit confirmation. Mana,
charges, rejection reasons, effect status, concentration dismissal and healing/save
feedback are visible. Magic Missile assignments, three-cell areas, blocked
Thunderwave pushes and delayed projectile/melee consequences use the existing
simulation clocks and fixed storage. Prepared spell glints reuse existing rasters.

## Checks executed

```sh
cmake --build --preset debug -j4
cmake --build --preset release -j4
ctest --preset debug --output-on-failure
ctest --preset release --output-on-failure
cmake-build-release/blobber_spells_tests --stress
cmake-build-release/blobber_impact_tests --stress
cmake-build-release/blobber_effects_tests --stress
cmake-build-debug/blobber_spells_tests --trace
cmake-build-release/blobber_spells_tests --trace
git diff --check
```

Debug **26/26 passed** (107.65 s); Release **26/26 passed** (11.93 s). No compiler
warnings were emitted by these builds. The final ability-description wording
cleanup was followed by focused spells/HUD/allocation rechecks in both builds.
Debug and Release diagnostic traces match byte-for-byte.

The frame allocation guard reports **zero tracked C++ allocations**. It now
includes first casts of every spell, spellbook/model updates and spell projectile
playback, alongside the existing repeated frame/effect/impact exercises. Fixture
construction, loading, image preparation and export are outside tracking. This
is not a claim about driver allocations or all third-party internals.

Separate stress fixtures exercise 100 bursts of **256 NPC attacks plus four
three-recipient Magic Missiles**, draining **528 release/impact events** per burst
without loss or replay. Two extra release notices per caster are consumed at
submission, before the drain. Mean drain time was **161.525 microseconds**, maximum
**257.49 microseconds**. The additional 100 area/full-effect-pool cycles took
**46.1545 ms**. Existing weapon stress drained all 260 attacks / 520 events, mean
**366.988 microseconds**, maximum **536.522 microseconds**; 100 effect-pool cycles
took **33.0649 ms**. These are bounded CPU diagnostics, not total frame benchmarks.
The population fixture deliberately moves its viewpoint through the grid to book
every attacker; it does not represent an ordinary reachable encounter.

Focused sanitizer checks reused downloaded raylib source:

```sh
cmake -S . -B /tmp/blobber10-sanitizers -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined' \
  -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/bison/Projects/blobber/cmake-build-debug/_deps/raylib-src
cmake --build /tmp/blobber10-sanitizers --target \
  blobber_spells_tests blobber_effects_tests blobber_impact_tests \
  blobber_hybrid_tests blobber_hud_tests blobber_party_content_tests -j4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/blobber10-sanitizers \
  -R '^(spells|effects|impact|hybrid|hud|party_content)$' --output-on-failure
```

ASan/UBSan **6/6 passed** (0.75 s). Leak detection remains disabled following the
previous handoff's tracing restriction; it is not verified. These are CPU tests.

## Coverage and findings

- Schema 4, invalid/duplicate/over-capacity spell data and legacy 1–3 compatibility;
  casting profiles, class eligibility, mana bounds and Life heavy-armor validation.
- All fourteen spells; successful/failed saves, attack damage, cantrip versus
  first-level costs, capped healing, downed recovery, death-save reset and no
  resurrection. Full-health/full-mana and exhausted-charge rejection.
- Bless/Bane recipients, concentration replacement and free dismissal during
  cooldown, Mage Armor restrictions and full-pool rejection before costs/RNG.
- Shared Magic Missile damage roll, distributed and staggered recipients,
  exact 342 ms example impact, pause, split/large update agreement, target loss,
  released-shot survival and interrupted 200 ms Inflict Wounds without refunds.
- Guiding Bolt application at impact, consumption by the next accepted attack
  and expiry; Rogue rear reach, unchanged formation and spent Sneak allowance.
- Twelve-recipient areas, no party damage, stable push ordering, protected
  occupants, walls/full destinations and range/view restrictions.
- Safe/unsafe Arcane Recovery, one-use Second Wind, picker pause, closing-click
  isolation, multi-target selection, charges and focus-loss cancellation. Existing
  hybrid/input/projection tests preserve phase and coordinate contracts.

Verification caught a legacy action-selection regression and an empty-book input
regression; both were corrected. The obsolete schema-rejection fixture was moved
from version 4 to version 5, and the class-feature error now names the unsupported
feature. A stress fixture initially put one recipient outside the view cone; its
viewpoint was corrected without relaxing target validation. Older diagnostic
callers now consume the complete submission batch, matching production input.

## Rendering and runtime assets

```sh
cmake --build cmake-build-debug --target blobber_gpu_smoke -j4
cmake-build-debug/blobber_gpu_smoke
cmake-build-debug/blobber --capture-spells /tmp/blobber10-captures
cmake-build-release/blobber --assets /tmp/blobber10-runtime-only --check-prepared
```

The GPU lifecycle check passed on **NVIDIA GeForce RTX 3060**, raylib 6.2/OpenGL.
Its initial sandboxed attempt could not open X11; the approved display-access
rerun passed. All windows were hidden and owned by the application; no OS input,
window focus changes or desktop automation was used.

The capture harness exports **159 PNGs** plus a timing/mana TSV: fourteen spells
at eleven stages each, three feature pickers, and two used-feature states. It
chooses spells through in-process UI input and confirms checked target commands.
Selected native 1920×1080 captures were inspected for spellbook layout, dart
travel, area targeting, healing feedback and exhausted ability display. The
complete generated set is in `/tmp/blobber10-captures`; representative images and
the trace are retained under [screenshots/slice10](screenshots/slice10).

Source-free validation passed with **zero NPC PNGs and no texture directory**.
The temporary tree contains JSON metadata, rules, prepared banks, UI/fonts,
preparation profile and maze. Resident prepared memory is unchanged:
scene 114,664,792 + NPC 66,637,718 + projectile 1,012,160 = **182,314,670 bytes**,
under the 512 MiB limit. On this build, `CombatEvents` is 409,808 bytes,
`GameSession` 682,000 bytes, `CommandController` 23,168 bytes and the visual
snapshot 63,632 bytes; fixed storage trades this predictable memory for no
per-cast allocation. These ABI measurements are descriptive, not portable limits.

Interactive desktop play and human acceptance are **unverified**. Native captures
were reviewed selectively, not every PNG. Reduced-size spell screenshots and a
full source-free screenshot comparison were not run; source-free startup and
existing projection/HUD coordinate tests passed. Final art remains slice 15.

## Review recipe

```sh
cmake-build-debug/blobber --spell-demo --seed 5
```

The Wizard's book starts open. Select Magic Missile, distribute darts by clicking
visible enemies, then Cast/Enter. The picker pauses; target selection resumes
simulation. Switch characters in Exploration and open dock slot 4 for their
spells/abilities. In turns, act with the active character. Bless accepts up to
three portraits; area spells show the three front cells before confirmation.
Escape cancels selection. Left/Right moves the target cursor, Up adds/toggles it,
and Enter casts. Concentration can be ended freely through its book entry.

For a window-free review, run `blobber_spells_tests --trace`: the fixed-seed example
spends two mana at time zero without changing enemy HP, then applies three darts
for 15 damage at 342 ms. Restart restores mana and one-use charges; encounters do
not. Rest-based refresh is intentionally deferred to slice 12.

## Next-agent contract

- Stop here pending user review. Do not mark this slice Accepted or start slice 11.
- Session `submit()` returns the primary acceptance outcome; consume **all** of
  `events()` before the next simulation operation. Recording both would duplicate
  the primary event. Update batches remain borrowed under the same contract.
- The timeline has 260 main slots plus eight extra spell-recipient slots. Their
  per-caster payload remains alive through the latest recipient's impact/hold.
  Never overwrite it on cosmetic expiry or retarget after acceptance.
- Mana and charges belong to Party; readiness/Sneak allowance to Combat;
  concentration/Guiding deadlines to Effects. Preserve gameplay versus presentation
  clocks, effect-expiry tie ordering and saved attack outcomes.
- The 2,048-event proof assumes four party casters and no NPC spellcasting.
  Future automatic cast chains, NPC spells, new effect kinds or larger areas must
  revisit storage/event bounds before expanding content.
- Catalog spell/feature descriptions borrow immutable storage. No frame-time
  vectors, temporary strings or per-projectile allocation are required.
- Equipment transactions in slice 11 must retain Life armor eligibility and Mage
  Armor reconciliation. Do not add rest refresh, progression or new racial traits
  as incidental follow-up work.

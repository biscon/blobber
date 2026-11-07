# Shadowtart — Arcane Emporium shopkeeper

Status: implemented and mechanically verified; runtime visual/interactive review
pending (2026-11-07).
The user approved the [sorceress reference](mockups/emporium-shopkeeper-v1/README.md),
named her **Shadowtart**, and authorized placement behind the emporium counter.

## Ownership and integration

Reuse the existing peaceful resident, prepared-image animation, dialogue and
bounded merchant systems. Appearance `shadowtart`, resident identity
`sanctuary.shadowtart`, shop `sanctuary_emporium`. The emporium constructor owns
exactly one canonical south-facing resident at cell (6,3), subcell (1,1). Add a
placement helper and validate identity, role, type, pose, guard routine and idle
phase on Continue, like Duncan. Existing actor checkpoint validation handles
runtime state. Preserve the counter, staff bypass, cell movement and fixed
1920×1080 presentation; no new runtime abstractions, capacities or allocations.

## Artwork and conversation

Generate four two-frame cardinal idle masters and one four-frame front talk
master with built-in imagegen from the approved reference. Preserve her adult
face, ebony hip-length waves, amber catlike eyes, jeweled circlet, indigo/black
velvet gown with gold embroidery and small red/blue belt vials. Idle has subtle
breathing/blinking, Talk restrained mouth/head motion, planted feet. Side/back
Talk uses existing directional Idle fallback. Use the existing 800×608 registered
canvas/anchor (400,584), one measured scale per sheet and 464-pixel maximum body
height, around .572 cells / 1.83 m at the current eye reference. Retain source
masters, exact prompts and registration; build extraction creates derivatives.
Idle durations 1800/140 ms; front Talk 180/160/180/220 ms. No procedural body edits.

YAML dialogue shows Shadowtart, with an automatic introduction and three topics:
About yourself, Your potions, and Advice for the depths. Trade and Farewell are
separate. Keep her mysterious and wry, with useful potion advice that matches
current mechanics. No new spells, quests or scroll functionality.

## Stock and compatibility

Use existing `healing_potion` and `mana_potion`, six of each, current prices
50/12 and 65/16 buy/sell, three-day restock, 24-hour buyback and 25% markup.
Move temporary potion retail stock from the inn to Shadowtart; inn retains camp
kits and existing sell/buyback price coverage. No item rules or icons change.
Dialogue history, finite stock, buyback and purse already persist in campaign
state. Schema/simulation remain 16/19; no fields or limits change. Appearance
fingerprint changes and canonical empty-emporium saves reject, so a new campaign
is required. Do not edit user saves or introduce migration.

## Verification strategy

Extend self-owned emporium fixtures for canonical placement, reachable routes,
occupancy, customer facing/range, resident exclusions, active/inactive disk
Continue with history/stock, tamper rejection and warmed allocation guards.
Reuse generic dialogue, trade, resident and animation checks. Run only the focused
headless Release emporium/smithy/inn/resident/animation/dialogue/trade/save suites
and one extended persistence test at integration. One intensive job, max two
low-priority build workers and one test process; no benchmarks or desktop input.

Build the Debug game and a hidden application-owned emporium resident review.
Inspect front, both sides, Talk, dialogue and stock. Verify all four talk frames,
finished response returning to Idle, correct names/topics, purchases/sales/buyback,
frozen game time/pose, production Continue and final world resource release.
Retain at most six selected images in `docs/screenshots/shadowtart/`; user runtime
approval remains separate. Own scratch in an artifact-helper run and clean after
promotion. Record commands, results and limitations here at completion.

## Verification results — 2026-11-07

All five generated masters and exact built-in imagegen prompts are durable in
`assets/npcs/shadowtart/animation/sheets`. Four idle sheets are 1774×887; the
four-pose talk master is 1086×1448. All decode as RGBA, alpha spans 0–255, and
each matches its original generated source byte for byte. Measured registration
fits twelve poses to the 464-pixel body limit with shared planted anchor. The
normal publication/extraction succeeds: 555 textures, 861,263,996 mip bytes and
64,967,422 alpha bytes for the complete publication, not simultaneous residency.

Focused headless Release build and eight-suite selection pass in 2.80 s:

```sh
OMP_NUM_THREADS=2 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 \
  cmake --build --preset headless-release --target blobber_emporium_tests \
  blobber_smithy_tests blobber_inn_tests blobber_residents_tests \
  blobber_animation_tests blobber_dialogue_tests blobber_trade_tests \
  blobber_campaign_save_tests -j 2
OMP_NUM_THREADS=2 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 \
  ctest --test-dir cmake-build-headless-release \
  -R '^(emporium|smithy|inn|residents|animation|dialogue|trade|campaign_save_smoke)$' \
  -j 1 --output-on-failure
OMP_NUM_THREADS=2 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 \
  ctest --test-dir cmake-build-headless-release -R '^campaign_save$' \
  -j 1 --output-on-failure
```

The one extended persistence check passes in 7.21 s. Coverage includes all
usable room routes with occupied resident cell, required appearance and canonical
pose/identity, facing/range, normal rest, third doorway failure/cancel semantics,
active and inactive Continue retaining history/stock/discovery/drops, empty/extra/
forged appearance/role/pose/runtime actor rejection and warmed zero-allocation
engine C++ simulation/commands. Existing generic resident combat exclusions,
animation/fallback, dialogue and trading boundaries/failure paths pass unchanged.
Tests own their small fixtures; no production artwork dependency is introduced.

The Debug game and emporium review build successfully. A review-tool compile
error from comparing typed item IDs directly was corrected to index comparison;
the subsequent build and runtime review pass. No passing CPU suite was rerun for
that tool-only correction. Production review uses a hidden NVIDIA GeForce RTX
3060/OpenGL 3.3 window and in-process logical input only:

```sh
OMP_NUM_THREADS=2 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 \
  cmake --build --preset debug --target blobber blobber_review_emporium -j 2
OMP_NUM_THREADS=2 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 \
  cmake-build-debug/blobber_review_emporium cmake-build-debug/assets \
  RUN_DIRECTORY/review shadowtart
```

Actual production YAML/catalogs verify the name/prompt, three conversation topics,
Trade/Farewell, both six-unit potion stocks and inn camp-kit-only retail stock.
All four Talk poses present, completed topics return to Idle, and game time/grid
pose stay fixed during conversation/trade. Both potion purchases, a mana sale
and buyback, then a healing sale succeed with exact stock/purse conservation.
Production disk Continue retains Shadowtart, introduction, three topic histories,
five remaining of each potion, the healing buyback and party purse.
World resources are 199,952,024 GPU bytes (190.69 MiB) and 25,910,901 CPU bytes
(24.71 MiB), within existing limits; texture and alpha leases return to zero.
These are engine-owned measurements, not driver-internal allocation claims.

[Six selected captures](screenshots/shadowtart/README.md) were inspected for
human scale, both profile orientations, counter occlusion, hair/circlet/wardrobe,
speech, readable dialogue and potion stock. Rear master is inspected, but rear
gameplay presentation is not claimed: shelving blocks a direct rear approach.
Interactive timing/feel and runtime visual acceptance remain user-owned. No
desktop input/focus automation, benchmarks, additional build matrix or user-save
edits. Current schema/simulation remain 16/19; changed appearance fingerprint and
canonical placement require a new campaign. The owned run is cleaned after
promoting six PNGs; source art/prompts/registration remain, with no diagnostic
exception retained.

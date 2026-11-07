# Duncan Ironballs

Status: implemented and mechanically verified; runtime visual/interactive review
pending (2026-11-07).
The user approved the [character mockup](mockups/blacksmith-v1/README.md), named
him **Duncan Ironballs**, and confirmed the smithy counter as his location.

## Content and ownership

Reuse the existing resident, dialogue, merchant and prepared-image systems.
Appearance `blacksmith` lives under `assets/npcs/blacksmith`; stable resident
identity is `sanctuary.blacksmith`. Exactly one peaceful, stationary Duncan stands
at the smithy's reserved cell (6,3), subcell (1,1), facing south. His occupancy
blocks the existing cell route; customer (6,5) faces north within conversation
range. Keep the staff bypass, approach, counter and all grid movement unchanged.
The innkeeper remains in the inn.

The smithy constructor owns canonical placement and validates it on Continue,
including type, role, identity, pose, idle phase, guard routine and absent monster
binding. Existing floor checkpoint validation protects runtime resident state.
No new actor classes, animation clips, dialogue commands or frame containers.

## Artwork and conversation

Generate four two-frame cardinal idle masters and one four-frame front talk
master from the approved reference using built-in imagegen. Preserve Duncan's
very muscular tall human build, full red beard, shaved sides/braid and leather
bib apron. Feet stay planted; breathing/blinking and speech are subtle. Side
art explicitly faces screen left/right, using the corrected innkeeper binding.
One uniform measured scale per sheet, standard 800x608 canvas, anchor (400,584)
and maximum 520-pixel body height give approximately .641 cells / 2.05 m at
the existing eye reference. Inspect in-engine rather than changing camera scale.
Use existing durations: idle 1800/140 ms; talk 180/160/180/220 ms. Side/back Talk
falls back to directional Idle. Retain masters, exact prompts and registration;
extract and publish derivatives in the build tree.

YAML dialogue displays Duncan Ironballs, with an automatic introduction and
three conversation topics: About yourself, Your craft, and Advice for the depths.
Trade and Farewell are separate topics. Use existing history and reveal/Talk
lifetime; keep all content printable ASCII. No quest, crafting or repair system.

## Shop and persistence

Shop `sanctuary_smithy` uses the existing bounded merchant ledger, finite stock,
three-day restock, 24-hour buyback and 25% markup. Stock every currently defined
weapon, body armor and shield, plus arrows for the bow: longsword, mace,
shortsword, shortbow, dagger, scimitar, quarterstaff, chain mail, scale mail,
leather armor, shield and arrows. Give equipment a target of two and arrows 100;
reuse the inn's existing prices. Move its temporary longsword/chain-mail retail
stock to Duncan; preserve inn prices for selling/buyback and retain its supplies
and temporary potions. Do not change item definitions or icons.

Campaign-owned dialogue history and vendor state already serialize. No new
schema fields or larger capacities. Adding the appearance changes the existing
NPC-content fingerprint, so previous campaigns require a new campaign; leave
user saves untouched and retain strict rejection. Canonical empty-smithy states
also reject. Schema/simulation remain 15/18; no migration layer.

## Verification

Use small self-owned fixtures to check canonical placement, movement occupancy,
routes, conversation admission/facing, three topics/trade routing, resident
combat exclusion, new and inactive save/Continue, merchant purchase/sale/buyback,
stock persistence and tamper rejection. Reuse the existing generic animation,
dialogue and merchant tests and allocation guards; add assertions only for the
new integration. Register masters and inspect orientation, frame alignment and
alpha without procedural image edits.

Build the Debug game and focused headless Release CPU tests with at most two low-priority workers,
one job at a time. Run focused smithy/inn/resident/animation/dialogue/trade/save
checks, not a broad configuration matrix. Hidden application-owned capture
checks front, both sides, Talk, conversation and trade, including resource release
and frozen game time. Never inject desktop input. Keep at most six selected PNGs
in `docs/screenshots/blacksmith/`; record outcomes and limitations here. Own and
clean all scratch under the artifact-helper run after evidence selection.

## Verification results — 2026-11-07

Five generated masters, exact requests and measured registrations are retained
under `assets/npcs/blacksmith`. All twelve poses decode as RGBA with alpha 0–255;
visible bodies fit their frames, share the ground anchor and use one scale per
sheet, capped at 520 pixels. Publication succeeds with 540 textures,
766,002,564 mip bytes and 64,012,448 alpha bytes. These are the entire publication,
not simultaneously resident texture costs.

Focused headless Release build and tests pass:

```sh
nice -n 15 ionice -c 3 cmake --build --preset headless-release --target \
  blobber_smithy_tests blobber_inn_tests blobber_residents_tests \
  blobber_animation_tests blobber_dialogue_tests blobber_trade_tests \
  blobber_campaign_save_tests -j 2
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-headless-release \
  -R '^(smithy|inn|residents|animation|dialogue|trade|campaign_save_smoke)$' \
  -j 1 --output-on-failure
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-headless-release \
  -R '^campaign_save$' -j 1 --output-on-failure
```

The seven-suite selection passes in 2.52 s; the single extended save check passes
in 7.12 s. New smithy checks cover required appearance, canonical identity/pose,
blocking occupancy, routes to every usable cell, customer facing/range, history
and finite stock across disk Continue and inactive travel, empty/extra/forged
placement rejection, runtime actor tampering and warmed allocation-free simulation.
Existing resident/combat exclusion, animation fallback, dialogue and trade failure/
capacity tests pass unchanged. Test catalogs and artwork fixtures remain self-owned.

The Debug game and both affected review tools build. Two hidden NVIDIA RTX 3060 /
OpenGL runs pass, using bounded library threads and application-owned input:

```sh
OMP_NUM_THREADS=2 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 \
  cmake-build-debug/blobber_review_smithy cmake-build-debug/assets \
  RUN_DIRECTORY/review blacksmith
OMP_NUM_THREADS=2 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 \
  cmake-build-debug/blobber_review_trade cmake-build-debug/assets \
  RUN_DIRECTORY/trade-review
```

The smithy review loads actual production YAML/catalogs, verifies all 12 current
equipment/ammunition types, the three topics, correct prompt/name and no offensive
picking. All four Talk poses are presented, completed responses return to Idle,
buying/selling/buyback work, and production Continue retains Duncan, introduction,
three completed topics, remaining stock, buyback and party purse. Conversation
and trade leave game time and the player's grid pose unchanged. World resources
are 190.84 MiB GPU / 22.03 MiB CPU in this smithy review, within existing ceilings;
all texture/alpha leases release. No claim concerns driver-internal allocations.

The existing merchant review now targets Duncan's stock rather than removed
temporary inn equipment. It additionally passes weapon/armor comparisons, drag
buyback, close-to-dialogue, purse and checkpoint checks. Its two comparison images
were inspected; temporary evidence is discarded rather than duplicating the gallery.

[Six selected captures](screenshots/blacksmith/README.md) were visually inspected
for scale, both side orientations, beard/hair/wardrobe, counter occlusion, speech,
readable topics and full shop stock. Runtime visual acceptance and interactive
timing remain with the user. The rear source was inspected; the existing shelves
block a direct rear approach, so no rear gameplay capture is claimed. No OS input,
benchmarks, unrelated test matrix or user-save edits. The new NPC fingerprint
requires a new campaign; schema/simulation stay 15/18. Owned raw captures/logs are
cleaned after selecting exactly six images; no diagnostic exception is retained.

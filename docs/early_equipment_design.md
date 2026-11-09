# Early equipment and continued campaign play

Status: implemented and mechanically verified (2026-11-09); interactive user
review pending. New campaigns are required; existing saves remain untouched.

## Content and boundaries

Add the nine missing SRD 5.1 body armors, rapier/warhammer/battleaxe/greatsword/
maul/longbow/light crossbow, bolts, eight wearables and six +1 weapons. The
catalog becomes 50 entries within its existing 64-entry bound. The smithy stocks
all ordinary armor, weapons and ammunition; the inn stocks camp provisions. New ordinary prices use SRD gold
prices, sale floor(25%), minimum one; bolts retain arrow pricing 2/1. Stock two,
except breastplate/half plate/plate one; bolts 100. Existing prices and recovery
supplies remain. Magic has no retail stock; smithy buys magic weapons and the
emporium buys wearables for 100 gp (400 gp reference, normal buyback).

Wearables: Circlet of Focus +1 spell attack; Gloves of Striking +1 melee weapon
damage; Boots of Balance +1 initiative/DEX saves; Belt of Vigor +3 maximum HP;
Cloak of Protection +1 AC/all saves; Ring of Resolve +1 all saves; Amulet of
Reserves +1 cleric/wizard maximum mana; Apprentice's Bracers +1 AC without body
armor/shield, compatible with Mage Armor. Weights in pounds: 1/1/2/1/3/0/1/1.
There is no attunement or magic equipment cap. Bonuses from equipped items add.
Armor stealth and Strength movement penalties remain explicitly deferred.

## Ownership, interfaces and storage

Schema 16 adds fixed typed item bonuses, heavy/loading metadata, race size and
bolt ammunition. Ordinary legacy definitions default to zero bonuses/Medium.
Rules derivation remains pure, fixed-storage and allocation free. Party owns
atomic equipment/resource commits; every inventory mutation reuses one commit
helper. Added maximum capacity never fills HP/mana; shrinking clamps current
values. Advancement grants only its normal level increase; Aid stays separate.
Spell attack bonuses remain distinct from ability modifiers, save DC and damage.
Flat weapon bonuses do not double on criticals. Small wielders have heavy-weapon
disadvantage through the existing advantage resolver. Loading allows one shot
per action; Action Surge permits another action/shot. Bolts consume at acceptance.

The startup-prepared UI owns one additional 2048x1024 RGBA atlas (32 256-pixel
tiles, 8 MiB GPU). New icon IDs start at 160. Static mappings preserve existing
IDs. Icons are imagegen-produced assets with prompts; no frame image work or
procedural substitute icons. Tooltips/comparisons derive actual candidate stats.

## Campaign rewards and continuation

An immutable starting reward profile records union weapon training, spell attack,
class mana and unarmored/no-shield eligibility. The profile travels with canonical
definitions and is persisted in the save envelope for deterministic reconstruction.
Dedicated Reward-domain seeds select one wearable per guardian: floor 1 boots/
belt/ring; floor 2 circlet/gloves/bracers; floor 3 cloak/amulet. Filter ineligible
wearables using the initial profile only. Two optional supply chests (floors 2/3)
gain distinct +1 weapons from longsword/mace/rapier/shortbow, filtered by training;
supplement dagger/quarterstaff when fewer than two qualify. Diagnostics without
a party use an all-eligible profile. No rerolls on equipment changes/deaths/load.

All floors have three chests. Guardian chest rewards are reserved at preparation
and published once on guardian completion; floors 1/2 retain their camp kits.
Final objective completion is persistent progress, not campaign victory. Remove
the final-floor active/save/restore/continuation exclusions. Endpoint remains a
content boundary; no fourth floor or victory action. Defeat and arena remain.

Compatibility: catalog 16, campaign schema/simulation 21/24, generator/templates
12/9. Older campaigns reject untouched; no migration. Compact baselines and
metadata-only inspection remain; no save-time generation. Library format and
starting equipment choices remain unchanged.

## Verification strategy and handoff

Use owned small fixtures for parser/validation, armor/weapon derivation, bonuses,
HP/mana swapping/Aid/rest/advancement, bolt ownership, heavy disadvantage, spell
attack/DC separation, deterministic eligible rewards and unique weapons. Exercise
full bags, stale requests, partial chest collection, travel/revisit/load and final
guardian continuation through save/return to Sanctuary. Keep XP and encounter
tuning. Run focused Debug suites/allocation guards, then affected persistence and
route/replay integration once. Hidden application-owned captures cover shops,
wearables and final chest. No desktop input automation. Use configured CLion
CMake/Ninja, at most two low-priority workers and one intensive job/test process.
Generate disposable evidence in the owned agent run, retain at most six selected
captures and clean the run. Interactive acceptance remains pending.

### Delivered verification — 2026-11-09

The Debug game, headless runner and owned review tool build with the configured
CLion CMake/Ninja. Production catalog validation reports 50 items and preserves
the original party's HP, AC, attacks, resources and starting equipment. Individual
source artwork and exact built-in imagegen prompts are indexed in
[the asset manifest](../assets/ui/items/README.md).

Focused Debug integration selection (26 suites) passes across the recorded runs:
`rules`, `rules_content`, `party`, `party_content`, `inventory`, `trade`, `dressing`,
`generation`, `generation_automation`, `generation_cli`, `population`, `guardians`,
`campaign`, `campaign_save_smoke`, `campaign_save_cli`, `campaign_route_tools`,
`campaign_route_fixture`, `progression`, `progression_allocations`, `spells`,
`level_spells`, `effects`, `recovery`, `hybrid`, `automation_allocations` and
`fixtures_automation`. Selection uses owned small fixtures and includes fresh
process strict replay and warmed engine-owned allocation guards. Initial failures
were corrected and their affected checks repeated, not the entire matrix.

New fixtures exercise eight simultaneously equipped wearables without a cap,
conditional AC, weapon/glove bonuses, Small heavy wielders, bolt-only consumption,
HP/mana swap clamping, stat comparisons, advancement with capacity items and
spell attack/DC separation. The extended `campaign_save` suite passes (7.71 s),
including unchanged unvisited magic rewards after caster deaths and reload,
invalid reward-profile rejection, active completed-final-floor restoration,
resource admission, chest ownership and existing failure/rollback coverage.

The route script now refreshes actor positions after reaction-time advancement
and chooses reachable approaches; its regression tests and direct route pass.
The separate opt-in full-clear route passes with the unchanged **793 XP + 9/12**
per four-survivor character and active campaign status. Its first 30-second attempt
timed out; only `--full-clear` now has a 60-second bound, while the ordinary route
retains 30 seconds. This is harmless-fixture integration, not production balance
or a benchmark. No production combat-rate or performance study was run.

The final comparison follow-up keeps real hypothetical weapon statistics visible
when a shield must first be removed; its `trade`/`inventory` checks pass (0.03 s).
Actual equipment transactions retain strict training, weight and hand validation.
The preview uses the same derivation with incompatible hands cleared and reports
the original restriction; missing armor training still shows hypothetical AC.

Commands: configured CMake `--build --preset debug --target <affected targets>
-j2`; `ctest --test-dir cmake-build-debug -R '^(<selection above>)$' -j1
--output-on-failure`; `ctest --test-dir cmake-build-debug -R '^campaign_save$'
-j1 --output-on-failure`; `python3 tests/campaign_route_fixture.py
cmake-build-debug/blobber_campaign_route_fixture --full-clear`;
`cmake-build-debug/blobber_rules_content_tests --validate assets/rules/catalog.json`;
and `git diff --check`. Long jobs use low CPU/I/O priority and bounded library
threads. The initial integration run briefly overlapped final asset publication;
both finished before a requested test pause could take effect. Subsequent work
was serial, with at most two build workers and one test process.

The owned `blobber_review_early_equipment ASSETS PREPARED NEW_OUTPUT` tool uses
hidden windows and in-process setup only. It verifies shop parsing, all eight
wearables, final objective/chest opening with active Exploration, checkpointing,
constant tracked GPU residency during presentation, and no OpenGL errors.
Sandbox display access was unavailable; the authorized hidden capture ran outside
the sandbox. No OS input or focus control was used. Selected evidence is indexed
in [the review gallery](screenshots/early-equipment/README.md); it does not imply
user acceptance. Armor Stealth/speed penalties remain deferred as approved.

Four selected review images are retained in the gallery. Disposable task output
was previewed and cleaned after closing its runners; no retained diagnostic
exceptions. Source artwork/prompts, build caches and user saves are preserved.

### Armor merchant correction — 2026-11-09

Moved all nine added body-armour stock and price entries from the inn to Duncan's
smithy at the user's request, preserving quantities and prices. All twelve
ordinary body armors are now stocked at the smithy; the inn's retail stock is
camp provisions only. Existing historical sale/buyback prices remain. The native
`loadRules`/`loadShops` check passed for all production shop files, all twelve
smithy armors, nine moved prices/quantities, and provisions-only inn stock.
`git diff --check` passes. This is an editable YAML correction: no game rebuild,
save migration, or graphics run. Restart loads the templates; an already-visited
smithy's newly added items arrive on its next existing delivery deadline.
The obsolete innkeeper armor screenshot was removed and the gallery/review tool
updated; no replacement capture was generated. Interactive acceptance remains
pending. The owned disposable validation output was previewed and cleaned.

# Damp Caves encounter pressure

## Design

**Implemented. Ready for review, 2026-11-20.** The player cleared the biome twice with little attrition and could consistently limit melee contact to two opponents by waiting at doors. Preserve this tactic in the Seep. Give two later rooms distinct answers to it, and spend some of the remaining monster XP budget on a more dangerous mixed group.

* In the first Galleries scouting party, two goblins screen two archers. Both archers have a shot down the opened entrance. A player moving inside has rock cover from one angle. Place these actors relative to the mouth and use ordinary sight, contact and attack rules.
* Give the Vault approach a broad, divided passage from the previous room. Spiders and newts start on both sides of a rock support. Keep the passage open back to the player's likely waiting position. The terrain must support two approaches before any attacker dies.
* Promote one spider and one newt in the Vault's middle pack. The adult already has a binding ranged attack. The Bull Cave Newt uses the blind newt's warning, Defend response, speed, bite damage, art and audio. Give it AC 12, 39 HP, Strength 16 and a +5 bite. Use the SRD 5.1 Giant Toad's HP and CR 1 reward (200 XP) as reference values. Its attack and warning are authored for this game.

The generator owns terrain and encounter membership. The monster catalog owns attack statistics and kill XP. The broodmother's 200 party XP belongs to her death, with the floor objective continuing to require all surviving young. Use the existing credited-death save state to prevent a second award. Regular attacks, party accuracy, other groups and finite recovery supplies retain their existing rules.

| Four conscious survivors | Seep | Galleries | Vault |
| --- | ---: | ---: | ---: |
| Ordinary party XP on floor | 700 | 1,250 | 1,450 |
| Optional party XP on floor | 250 | 300 | 300 |
| Ordinary entry XP each | 675 | 850 | 1,162.5 |

An ordinary clear ends at 1,525 XP each. A full six-floor clear ends at 1,856.25 each, below the level-four threshold of 2,700.

## Verification plan

Run the same seed-five continuous expedition before and after the change with a paladin, Life cleric, wizard and ranger. Begin at the Seep's development start. Use ordinary commands, collect floor supplies, spend Hit Dice, potions and kits as needed, and carry the party through each descent. Report defeat or incomplete coverage explicitly. Check ordinary and optional reward totals separately. Exercise both doorway positions and interior cover, the divided approach, forced fallback generation and the mother-first save/Continue boundary. Leave final difficulty judgment to a player running the same route.

## Baseline

Seed five, ordinary route, before the pressure changes. The development Seep entry supplied three camp kits, seven healing potions and nine mana potions. The party defeated the mother and surviving young, used four kits and finished with one kit, ten healing potions and zero mana potions. Paladin, cleric, wizard and ranger ended at 28/28, 0/30, 17/17 and 28/28 HP. The cleric reached zero HP. This runner prioritizes warned Defend, Smite, offensive spells, then weapon attacks. After fights it spends a Hit Die at or below half health, can drink a potion, and considers a kit below one-third HP or two mana. It collects the ordinary camp supplies and guardian kits, and skips optional pockets.


## Implementation and checks

The populated Galleries inlet has a door at the chamber mouth and a column between two archer positions. Generation checks the walkable shoulder, projectile range and line of sight from all three stances. In the Vault the two chambers extend to meet a single rock pillar. Two separate, one-cell connections record the north and south passages. Both spiders at the openings can alert nearby allies. The system uses its existing cell, path and actor capacities. The shared geometry validator recognizes the paired sockets only at the Vault approach. The actors use ordinary navigation and initiative.

The Bull Cave Newt binds its own monster and attack records to the regular newt appearance, portrait, audio and hearing trait. The broodmother uses the same persistent kill credit as ordinary monsters. Clutches and renewable young give zero. The mother-first campaign fixture saves after her death with one hatched young alive, decodes and restores the save, then kills the young. XP stays fixed and victory occurs on the last kill. The starting-floor picker uses the revised entry budget. This revision uses campaign schema/simulation 29/33 and generator/templates 15/11. Start a fresh campaign for these layouts.

| Seed 5, same four-class party and ordinary route | Before | After |
| --- | ---: | ---: |
| Galleries scouts, party HP lost | 4 | 12 |
| Vault mixed pack, rounds / party HP lost | 3 / 4 | 5 / 16 |
| Vault approach, rounds / party HP lost | 3 / 0 | 5 / 6 |
| Long rests across all three floors | 4 | 3 |
| Kits / healing potions / mana potions left at victory | 1 / 10 / 0 | 2 / 10 / 0 |
| Paladin / cleric / wizard / ranger HP at victory | 28 / 0 / 17 / 28 | 21 / 3 / 17 / 28 |
| Final XP each | 1,437.5 | 1,525 |

Both runs defeated the brood. The revised party entered the boss fight with two mana each on the paladin, cleric and wizard. It used all three pools and finished with everyone conscious. Individual fights cost more HP and several more rounds, while the party spent one fewer kit over the expedition. The evidence supports the positional changes. Overall difficulty still needs player judgment.

The Debug checks were:

* `blobber_review_caves expedition 6 5 assets` drove all 17 ordinary fights including all three guardians. One campaign instance carried health, XP, inventory and resources from the Seep entrance through victory. The six optional fights are outside this comparison.
* `blobber_review_caves check D 5 NEW_DIR assets cmake-build-debug/assets/prepared --populated` passed for depths 4, 5 and 6, on normal generation and with `--fallback`. Ordinary and full routes verified XP, gates and return landings. These separate accounting checks can resolve deaths directly after contact. The scout and Vault positioning checks used normal enemy attacks and passed party turns, keeping the entire enemy group alive. With animated movement, the party opened the closed door and reached each stance before melee contact. Both archers fired from their original positions outside. Inside, one had to relocate. In each Vault layout, three different melee enemies attacked through both lanes from two distinct contact cells. The bull named a defender and rolled its guarded bite at disadvantage after Defend.
* Focused Debug CTest passed `cave_layout`, `brood_vault`, `damp_campaign`, `campaign_save_smoke` and `campaign_save_cli`. These cover the changed geometry, completion, kill credit and Continue paths. The Debug game and its prepared assets built successfully. The existing paired nest comparison also completed.

## Player review

1. Run `./cmake-build-debug/blobber --development --seed 5 --campaign-save output/cave-pressure-review.json`. Select **Start new game**, fill the party, then **Choose floor → 4. The Seep**. For the matched route use a paladin, Life cleric, wizard and ranger. Skip both optional encounters on each floor, take the camp supplies and guardian kits, and rest whenever you would in normal play.
2. Beyond the divided banks on floor five, face east and open the scouts' door from **(21,15)**. Holding there or stepping back to **(20,15)** exposes the party to both archers. To try cover, step forward through the doorway, then strafe right to **(22,16)** before melee contact. One archer loses its firing line and can move to find another. Clearing the scouts gives **937.5 XP each** and permits manual level-three advancement.
3. On floor six the mixed group near **(30,5)** has two small spiders, one adult web-spinner and the Bull Cave Newt. Watch the named bite target and try Defend. Beyond the goblin sentries, face east around **(40,5)** and hold between the two sides of the pillar. Watch for a third melee attacker while the first two are alive. [Seed-five encounter maps](screenshots/damp-caves-dc06/README.md).
4. Finish the Vault with the resources you brought. Record how many kits you used and whether the doorway, pillar and target choices changed what you did. A direct Galleries or Vault start is available for positional review and supplies a fresh party.

These are single-seed results for one spell-heavy policy. The runner uses basic ranger weapon attacks, decides recovery from HP and mana thresholds, and skips optional fights and town shopping. Firing lines change as enemies move, and a shot already in flight may land after the party reaches cover. Headless checks cover commands and outcomes. Interactive readability and overall difficulty await player review.

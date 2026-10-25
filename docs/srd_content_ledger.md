# SRD 5.1 source and adaptation ledger

Source: [official SRD 5.1 Creative Commons edition](https://media.dndbeyond.com/compendium-images/srd/5.1/SRD_CC_v5.1.pdf).
Page references below are printed page numbers. Only listed primitives/data are
adopted; this is not an import of the complete SRD. Attribution travels with the
catalog in `assets/rules/ATTRIBUTION.md`.

| Rule/content | Source | Slice 01 treatment |
| --- | --- | --- |
| Six abilities and modifiers | pp. 76–77 | Scores 1–30; round down, including negative modifiers. |
| Advantage/disadvantage and proficiency | pp. 76–77; class tables | One extra d20, no stacking, opposite conditions cancel; supported character levels 1–3 have +2 proficiency. |
| Ordinary saving throws | p. 80 | Compare modified roll to DC; no natural 1/20 exception. Death saves are separate and deferred. |
| Attack rolls | pp. 93–94 | AC comparison, natural 1 miss and natural 20 critical hit. Target legality and action economy deferred. |
| Damage and critical hits | p. 96 | Rolled damage, critical dice doubled, modifier once, zero minimum; physical damage types only. Mitigation/HP mutation deferred. |
| Goblin | p. 315 | STR 8, DEX 14, CON 10, INT 10, WIS 8, CHA 8; AC 15, HP 7; +4 scimitar/shortbow, 1d6+2 slashing/piercing. Fixed HP adopted. |
| Goblin appearance | Existing project artwork | Dagger art remains while melee mechanics use the scimitar baseline; visual difference, not a new dagger stat block. |
| Nimble Escape | p. 315 | Not implemented; explicit adaptation/defer decision belongs to slice 07. |

Project implementation limits (dice counts, JSON capacities, supported levels,
AC bounds, physical-only damage types) are engineering limits, not SRD rules.
Ranges, CR/XP, senses, skills, and other goblin features are not yet represented.
Race/class definitions, party choices, and starting equipment are recorded below.
Mana, shared maneuvers, cooldowns, and omitted actions remain proposed
adaptations in the roadmap. Slice 02 adds formation storage, not formation combat.


## Slice 02 adoption and adaptations

All source references below refer to the official SRD linked above; page numbers
are printed pages. Attribution remains included with copied executable assets.

| Content | Source | Implemented treatment |
| --- | --- | --- |
| Human, Hill Dwarf, Lightfoot Halfling, High Elf | pp. 3–6 | Ability adjustments, Dwarven Toughness, and weapon training. Other racial features are inactive. |
| Fighter, Cleric, Rogue, Wizard | pp. 15–16, 24, 39, 51–53 | Level-one Hit Dice/HP, weapon/armor training, and save proficiencies. Fighter Defense active. |
| Armor, shields, weapon statistics | pp. 62–66 | Nonmagical base AC, damage, weights, finesse, two-handed and versatile handling for supplied gear. |
| Ammunition, holy symbol, spellbook, focus | pp. 67–70 | Owned quantities and weights; no spending or casting behavior. |
| Carrying capacity | pp. 79–80 | Strength × 15 lb; no graded encumbrance. |
| Attack/damage ability and proficiency | pp. 93–96 | Deterministic previews; no attack execution. |

Names, standard-array assignments, relative formation, and compact equipment
packages are demo choices. Armor/shield proficiency is required at build validation
rather than implementing untrained-armor penalties. Equipment is limited to body,
main weapon, and off-hand shield; no dual wielding. Best finesse ability and free-hand
versatile grip are automatic. Thrown attacks, range, ammunition consumption,
loading, stealth/speed penalties, skills, tools, and languages are not simulated.
The [feature matrix](characters_design.md#content-and-compatibility) distinguishes
active passives, slice-10 decisions, and omissions. Mana has no provisional value.

## Slice 03 runtime integration

Runtime monster HP initializes from the existing catalog baseline (goblin: 7).
Resolved nonnegative damage now reduces HP; reaching zero makes the monster dead
and immediately nonblocking. No new SRD content or attack resolution was added.
The 100 ms white hit/death flash and subsequent hiding are project presentation
choices, not tabletop mechanics. Loot, XP awards and death animation clips remain
deferred to their roadmap slices.

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
Race/class definitions, party choices, and starting equipment belong to slice 02.
Mana, formation, shared maneuvers, cooldowns, and omitted actions remain proposed
adaptations in the roadmap; none are implemented by this foundation.

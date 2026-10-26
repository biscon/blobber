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
| Nimble Escape | p. 315 | Inactive in slice 07 by user decision; adaptation deferred to slice 08 alongside disengagement. |

Project implementation limits (dice counts, JSON capacities, supported levels,
AC bounds, physical-only damage types) are engineering limits, not SRD rules.
Ranges, CR/XP, senses, skills, and other goblin features are not yet represented.
Race/class definitions, party choices, and starting equipment are recorded below.
Mana, cooldowns, and omitted actions remain proposed
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


## Slice 05 combat adoption and adaptations

Source remains the official SRD 5.1 Creative Commons PDF linked above; attribution
continues to ship with the catalog. No new content schema or equipment data.

| Rule | Source (printed pages) | Treatment |
| --- | --- | --- |
| Initiative and groups | pp. 90–91 | One roll per character/monster-definition group, fixed across rounds; deterministic ties use Dexterity, party, then stable identity. |
| Attack and damage | pp. 93–97 | Existing equipped weapon derivation and goblin scimitar baseline now execute; ordinary hits, natural-roll exceptions and critical dice retained. |
| Dodge | p. 93 | Defend spends an action, granting attack disadvantage until next turn/unconsciousness. Dexterity-save and speed conditions are not simulated. |
| Zero HP, death saves, stabilization | pp. 97–98 | Dying/stable/dead states, natural 1/20, three outcomes, damage at zero, massive damage, healing reset and no resurrection. |
| First aid | p. 98 | Aid spends an action on DC 10 Wisdom; no Medicine proficiency because skills remain absent. |

Paired-slot protection on both sides and visibility-limited cell melee are project
adaptations. User clarification supersedes ordinary back-row step-forward attacks;
Rogue exceptions remain slice 10. Following user review, a front-row survivor shields only its own rear column.
An exposed melee goblin moves into its vacant front slot on its next attack turn;
this same-cell reform does not cost an additional action. Goblins target conscious exposed members uniformly.
No conscious party member ends the encounter in defeat without finishing death
saves. Victory automatically stabilizes surviving downed allies without restoring
HP. Natural 1d4-hour recovery is deferred with resting. No reach, surprise, shared
maneuvers, ranged approach, class features, XP, loot or resurrection is implied.

## Slice 07 AI adaptation

Directional six-cell sight, last-seen memory, guard/patrol/wander routines,
600/1200 ms movement and side/rear waiting are project adaptations, not SRD senses,
speed, stealth or action-economy implementations. No perception rolls, hearing,
shared alerts or surprise are active. Nimble Escape remains inactive by explicit
user choice; decide its adaptation in slice 08 with disengagement. Existing melee
rules, target selection and front-slot advancement remain authoritative.
See [AI design](goblin_ai_design.md) and [verification](slice07_verification.md).


## Slice 08 hybrid encounter adaptations

The catalog's existing shortbow attack now executes for goblins; Merrin readies
his carried bow in a dedicated ranged slot. Ammunition remains unspent until slice
11. Range is six cells by Euclidean distance with conservative wall/corner LOS,
no creature cover, and disadvantage with adjacent enemies. Ranged attacks ignore
formation protection. These range/formation policies are project adaptations.

NPC surprise uses awareness instead of skill checks and skips the NPC's first
action. The party has no surprise penalty after the approved front-focused review
correction. Initial unnoticed ranged attacks receive advantage. Local hit-or-miss alerts travel three walkable cardinal steps
without chaining. Existing group initiative survives partial-round disengagement/re-entry; arrivals
join at round boundaries. These are deterministic demo policies, not complete
SRD stealth, perception, cover or encounter rules.

Nimble Escape is represented as a typed catalog trait. At half HP or below, an
adjacent mixed-weapon goblin spends its whole action on one legal step away, then prefers ranged
attacks. The SRD bonus-action Hide/Disengage behavior is intentionally adapted;
there is no bonus-action engine or opportunity-attack simulation. Combat uses
steady facing with automatic turns after the adjacent front group clears. Turns
cost no action; offscreen enemies cannot attack. The front row always protects
its paired back row. Shared maneuvers and party retreat were removed by review.
These are project mechanics, not tabletop facing or flanking rules.

Gameplay advances in real time during approach and six seconds per finished
combat round. Decisions and presentation holds cannot recharge readiness or saves.
Presentation has its own clock; overlays/focus pauses stop both. Defend and
six-second readiness/death-save deadlines persist across phase changes. This clock
contract is available to later effects work; no slice 09 effects were implemented.

Slice 08 single-weapon review correction: melee-only NPCs close and attack even
when wounded. Ranged-only NPCs hold a legal shot, approach to obtain one, and try
to create distance when adjacent regardless of health or Nimble Escape. A blocked
escape falls back to the existing disadvantaged ranged attack. These are generic
project AI defaults; mixed-weapon goblin decisions remain unchanged. Two demo
archers share the goblin stats/art but list only the shortbow attack.

Three-cell melee-front adaptation: diagonal neighbors count as close contact with
conservative wall/corner LOS. Either side may melee across the three cells ahead;
paired enemy rows follow party heading. Close-range ranged disadvantage and
archer escape use this same neighborhood. Movement remains cardinal; this is a
project grid/formation rule, not additional tabletop weapon reach.

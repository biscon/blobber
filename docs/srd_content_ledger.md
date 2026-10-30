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
archers shared the goblin stats/art but listed only the shortbow attack at that checkpoint.

Slice 8.2 role loadouts (2026-10-26): production `goblin` now lists only the
existing scimitar-mechanics attack, with its historical dagger artwork.
`goblin.archer` retains only the existing shortbow attack and receives distinct
hood/bow/quiver artwork. HP, AC, abilities, traits, attack bonus and damage are
unchanged. These weapon-specific loadouts are project adaptations, not a claim
that the SRD goblin has only one weapon. Generic mixed-weapon engine behavior
remains covered by independent test catalogs. No ammunition or inventory rules
are introduced.

Three-cell melee-front adaptation: diagonal neighbors count as close contact with
conservative wall/corner LOS. Either side may melee across the three cells ahead;
paired enemy rows follow party heading. Close-range ranged disadvantage and
archer escape use this same neighborhood. Movement remains cardinal; this is a
project grid/formation rule, not additional tabletop weapon reach.


## Slice 09 effects and concentration

Source: the official SRD 5.1 CC PDF linked above, verified during planning.
These are effect primitives for later spells; casting/mana/target selection is not
implemented. Printed-page references:

| Content | Source | Adopted consequence |
| --- | --- | --- |
| Concentration | p. 102 | One per caster; Constitution save per positive damage source at impact, DC max(10, floor(damage/2)); incapacitation/death ends it. |
| Combining spell effects | p. 104 | Same-kind benefits do not stack; independently owned durations remain. Different kinds combine. |
| Bane | p. 120 | Initial Charisma save; attack/save minus 1d4; concentration up to one minute. |
| Bless | p. 122 | Attack/save plus 1d4; concentration up to one minute. |
| Hold Person | pp. 154–155 | Wisdom initial and repeat saves; paralysis; concentration up to one minute. |
| Mage Armor | p. 160 | Unarmored party base AC 13 + Dexterity, shield retained; eight hours, no concentration. |
| Shield of Faith | p. 179 | +2 AC, concentration up to ten minutes. Casting action adaptation is left to slice 10. |
| Paralysis | pp. 358–359 | Cannot act/move; automatic Strength/Dexterity save failure; incoming advantage and close-hit criticals. |
| Death saves | p. 98 | Bless/Bane modify the save total; natural 1/20 exceptions still use the original face. |

Approved adaptations: Hold Person retries every six gameplay seconds outside turns,
and at completed full-round boundaries during turns, rather than at the individual
target's turn end. Pauses and decision/playback time do not advance these deadlines.
Incapacitated front members no longer protect their paired rear; mobile allies
carry paralyzed party members. Close criticals use the accepted conservative
adjacent-contact geometry. Logical steps already committed finish visually.

The 64-instance pool, three-target storage, 1..1000 supplied DC and party-only caster
scope are engineering bounds, not SRD rules. Hold accepts one recipient; Bless/Bane
accept up to three; upcasting remains deferred. Mage Armor is party-only because
NPC armor currently has no equipment breakdown. Future spell commands must enforce
class, humanoid, range/visibility and resource rules before requesting application.
See [effects design](effects_design.md) for lifecycle, timestamp and capacity contracts.


## Slice 10 spells and class identity

Source remains the [official SRD 5.1 CC edition](https://media.dndbeyond.com/compendium-images/srd/5.1/SRD_CC_v5.1.pdf),
spell descriptions under the names below and the Fighter, Life Domain, Rogue and
Wizard class sections. Attribution remains with the executable catalog. Only the
listed level-one mechanics are adopted; this is not full tabletop spellcasting.

| Content | Adopted mechanics | Demo adaptation or omission |
| --- | --- | --- |
| Sacred Flame; Fire Bolt; Acid Splash | 1d8 radiant/Dex save; 1d10 fire/spell attack; 1d6 acid/Dex save | Six-cell visible-target range. Acid Splash allows one or two targets in the same cell. Cantrip scaling is deferred. |
| Spare the Dying; Cure Wounds | Stabilize a dying creature; 1d8 + casting modifier healing | Party portraits are touch reach. Dead allies cannot be restored; full-health healing rejects without cost. |
| Bless; Bane; Shield of Faith; Mage Armor | Existing slice 09 effects and durations; Bane uses a Charisma save | Up to three Bless/Bane recipients; other buffs one party recipient. Shield uses a full action. No material components. |
| Guiding Bolt; Inflict Wounds | 4d6 radiant ranged spell attack and advantage for the next attack; 3d10 necrotic melee spell attack | Guiding advantage expires after six gameplay seconds, starting at impact; next accepted attack roll consumes it. Inflict uses exposed front-row melee reach. Healing Word was excluded by user decision. |
| Magic Missile | Three automatic-hit force darts, each 1d4+1 | One shared damage roll per cast; distribute among up to three visible enemies. No Shield spell, counterspell or automatic retargeting. |
| Burning Hands; Thunderwave | 3d6 fire/Dex save; 2d8 thunder/Con save; half damage on successful save | Both affect the three clear-sight front cells, including protected/occluded occupants, without friendly fire. Failed Thunderwave survivors push one cardinal cell; occupied/walled destinations block without extra damage. |
| Fighter Second Wind | 1d10 + level self-healing | Full action; one charge until restart in this slice. Rest refresh is deferred. Defense remains active. |
| Life Domain | Heavy armor proficiency; Disciple of Life adds 2 + spell level to healing | Fixed Cleric profile; no domain-selection UI or further domain abilities. |
| Rogue Sneak Attack | Finesse/ranged eligibility, advantage or engaged ally, no final disadvantage; 1d6 extra damage, doubled on a critical | Rear-row melee does not change formation. An acting front ally engages the three-cell front. One accepted successful hit per six gameplay seconds, not once per individual tabletop turn. |
| Wizard Arcane Recovery | Limited recovery of spellcasting resources | Once per restart, restores two mana at level one in safe Exploration: no pending attack and no aware or adjacent living hostile. No short-rest transaction yet. |

Mana replaces spell slots: levels 1/2/3 have caps 4/6/14; cantrips cost zero and
first-level spells cost two. The fourteen curated spells are always accessible to
their class without preparation or spellbook management. Casting takes one action
or the shared six-second real-time cooldown. Components, bonus actions, rituals,
upcasting, second-level spells, advancement and new racial traits are omitted.
Existing racial numeric and training passives remain active. Levels 2/3 mana caps
are data support, not a delivered progression system.

Projectile spells use immediate release and 180–450 ms travel; Inflict Wounds
strikes at 200 ms; all other consequences occur at casting time. These timings,
conservative grid visibility, pause/full-round clocks, UI confirmation rules and
bounded capacities are project mechanics. See [spells design](spells_design.md)
and [verification](slice10_verification.md).


## Slice 11 inventory, ammunition and potions

Source: [official SRD 5.1 CC edition](https://media.dndbeyond.com/compendium-images/srd/5.1/SRD_CC_v5.1.pdf),
printed p. 64 (ammunition), pp. 68–69 (potion use and adventuring gear weight), pp. 79–80 (carrying capacity),
and p. 234 (Potion of Healing). Existing weapons/armor retain their earlier ledger entries.

| Content | Adopted behavior | Project adaptation or omission |
| --- | --- | --- |
| Potion of Healing | 2d4+2 healing; drinking or administering takes an action; weight 0.5 lb | Any living party ally is within reach. Full HP rejects without consumption. No resurrection or Life Domain spell-healing bonus. |
| Bow ammunition | One compatible arrow consumed per accepted attack | Consume at acceptance, including misses/interruption. NPC arrows remain abstract. Post-battle recovery is deferred. |
| Carrying capacity | Strength × 15 lb | Integer hundredths of pounds; reject overweight transactions atomically. No encumbrance tiers, lifting/dragging or racial size variations. |
| Mana potion | No SRD equivalent | Restores two mana, weighs 0.5 lb, uses one action/shared six-second cooldown. Full mana/noncasters reject. |

The 48-cell bags, four equipment positions, 208-stack bound, arrows-100/potions-20
stack limits, paused browsing, safe-only equipment/transfers, default-one split
confirmation and automatic transfer ordering are approved project rules. All four
characters start with two healing potions; Cleric and Wizard additionally start
with two mana potions. Rogue starts with twenty arrows. Loot, arrow recovery, rest,
progression, saves and dropping items are outside this slice.

## Slice 12 loot and recovery — implemented, awaiting review

Source: [official SRD 5.1 CC edition](https://media.dndbeyond.com/compendium-images/srd/5.1/SRD_CC_v5.1.pdf),
printed p. 62 (coin weight), p. 87 (short/long rests), and the previously recorded
class/equipment sections. See the [approved design](loot_recovery_design.md).

SRD 5.1 does not contain the DMG's individual-treasure/hoard-generation tables.
The 1d6 gold for current CR-1/4 goblins, guaranteed usable equipment, 15% healing /
5% mana potion outcomes, gold stack limit and fixed camp kits are project choices,
not a universal SRD gold-per-monster formula. Gold uses 50 coins per pound.
NPC stats/art/weapon roles stay unchanged, including the existing dagger-art /
scimitar-rules distinction and the archer's abstract shield/AC treatment.

Adopt one-hour short rests with selected class Hit Dice + Constitution healing,
and eight-hour long rests with full HP and half-maximum Hit Dice recovery (minimum
one). Second Wind refreshes on either rest; Arcane Recovery refreshes only on a
long rest while retaining its accepted manual safe-exploration activation. Mana
refill is the project's resource adaptation. Stable zero-HP allies may receive
rest benefits; dead allies never do. Omit the once-per-24-hours long-rest limit
and the SRD's one-HP-at-start requirement. Require existing safe Exploration plus
settled presentation/no dying ally. Skip gameplay time without AI movement.
Camp kits, authored sanctuaries, instantaneous transactions and cosmetic camp
playback are project adaptations. Implementation and checks are recorded in
[slice 12 verification](slice12_verification.md).

## Character-creation interlude (approved 2026-10-28; updated 2026-10-29)

The user accepted expanded spell selection and the visual creator flow. See
[slices 12.1–12.4](dnd5e_demo_design.md#character-creation-interlude-accepted-flow-and-shared-contracts)
for the roster, source requirements and adaptation questions. The following paragraphs record the original approved scope.
The 12.1 and 12.2 implementation records below supersede their planned status.

The planned additions are Ray of Frost, Shocking Grasp, Poison Spray, Sleep,
False Life, Grease, Color Spray, Guidance, Resistance and Sanctuary. The existing
fourteen spells remain. The user explicitly selected Color Spray instead of the
mockups' illustrative Ray of Sickness after source checking against the
[official SRD 5.1 CC edition](https://media.dndbeyond.com/compendium-images/srd/5.1/SRD_CC_v5.1.pdf).
Do not import Ray of Sickness from a different source by following the old image label.

Normal creation selection counts, Life Domain prepared grants and the High Elf
Intelligence-based cantrip are planned alongside per-character spell ownership.
Point buy uses the [2014 Basic Rules character-creation variant](https://www.dndbeyond.com/sources/dnd/basic-rules-2014/step-by-step-characters)
with existing racial bonuses. Mana, automatic one-shot buff consumption (if adopted),
Guidance's initiative consumer, discrete slow/prone/area behavior and action-cost
adaptations must be documented as project choices, not passed off as unchanged
tabletop rules. Skills and reactions remain outside this interlude.

## Slice 12.1 — approved adaptations, implemented and accepted

Use the [cached official SRD 5.1](references/README.md). The detailed
[expanded-spell design](expanded_spells_design.md) records the approved gameplay
adaptations before code: aimed 3×3 Sleep/one-cell Grease, friendly fire, automatic
next-turn knockdown recovery costing the action, two-action slowed NPC steps and
slower travel, caster-turn short expiry, automatic one-shot buffs, automatic best
temporary-HP amount with refreshed duration, bounded Sanctuary retargeting, and
High Elf magical-sleep immunity. Class-wide casting from this historical checkpoint
is replaced by explicit per-character grants in 12.2.
Reactions, other racial traits, progression and creation UI remain deferred.
Source locations: Color Spray p.125; False Life p.142; Grease p.150; Guidance p.151;
Poison Spray p.169; Ray of Frost p.174; Resistance p.175; Sanctuary p.176;
Shocking Grasp p.179; Sleep p.180; temporary HP pp.98–99; conditions pp.358–359.
Implementation and automated verification are recorded in the [handoff](slice121_verification.md).
User acceptance remains pending; these are project adaptations, not unchanged tabletop rules.

## Slice 12.2 — implemented, accepted and closed (2026-10-29)

[Design and exact selections](character_builds_design.md),
[verification/handoff](slice122_verification.md). The local SRD's High Elf cantrip,
Wizard spellbook/preparation, Cleric preparation and Life Domain grants now feed
explicit character-owned selections. Racial wizard cantrips use Intelligence,
including on Clerics and martial classes. Mana and the existing class feature/action
adaptations remain project rules; racial magic adds no martial mana pool.

Point buy adopts the previously approved 27-point, 8–15 variant with existing racial
bonuses. Requiring all 27 points and at least one ordinarily prepared spell are
creation-flow constraints. Curated starting gear is a project selection menu, not
an implementation of starting gold or every tabletop package. Two healing potions
per character, two mana potions for casters and three camp kits per fresh party are
approved demo supplies. Quarterstaff uses 4 lb, 1d6 bludgeoning and versatile 1d8;
existing hand/proficiency/weight validation applies. Skills, creator/menu screens,
rest-time spell changes and advancement remain later slices.


## Slice 12.4 — implemented, accepted and closed (2026-10-29)

The [creator and preparation design](character_creator_design.md) exposes the
already adopted 12.2 race/class, 27-point buy, curated gear and spell-grant rules.
It adds no spell, racial trait, skill, feat or progression rule. High Elf picks remain
Intelligence-based and separate from class cantrips; Life Domain grants remain
locked and outside the ordinary preparation allowance. Wizard preparation uses
that character's learned spellbook. Names/portraits and library schema are unchanged.

Long Rest can replace living Cleric/Wizard preparation using the same 1..allowance
constraint. The user explicitly approved a changed legal preparation as a sufficient
rest benefit even at full resources. Existing safety, eight-hour advancement and
one camp kit outside sanctuaries still apply. Staging/Cancel is free; only the checked
rest commit changes live lists. Short Rest never changes preparation. These are
project UI/recovery adaptations, not claims of unchanged tabletop preparation
procedures. Library blueprints are never modified by a live rest. The user accepted
slice 12.4 on 2026-10-29 and explicitly retained the current 27-point buy, 8–15 base
scores and fixed racial bonuses for now. No BG3-style assignable bonuses or higher
starting-score cap was adopted. Progression is recorded separately below.

## Slice 13 approved adaptations (implemented, accepted and closed, 2026-10-30)

See [progression design](progression_design.md) for the frozen feature/spell roster,
action economy, targeting and limitations. Sources: local SRD 5.1, character
advancement pp. 56–57; Cleric/Life pp. 15–17, Fighter/Champion pp. 24–25, Rogue/Thief
pp. 39–41, Wizard/Evocation pp. 51–55; Aid p. 114, Hold Person pp. 154–155, Lesser
Restoration p. 158, Mirror Image p. 165, Scorching Ray p. 176, Shatter p. 178,
Spiritual Weapon p. 182, Web p. 192. Page locations checked against the cached
official SRD text extraction. XP splitting in exact twelfths, 900 cap, deficit-preserving
advancement, Cunning Defend/Fast Hands potions, automatic Preserve Life allocation,
free limited Spiritual Weapon activation, grid areas/clocks and Web burning are
explicit project adaptations. No bonus-action/reaction framework or unsupported
features are claimed. Fixed subclass paths are Champion, Life, Thief and Evocation;
Turn Undead, skills/Expertise, locks/traps/climbing, spell copying/Evocation Savant,
upcasting and scenery destruction remain deferred. The eight spell implementations
and limits are documented in the design; [verification and review](slice13_verification.md)
record the delivered behavior. The user accepted slice 13 and milestone C on
2026-10-30, including existing Action Surge activation through class features.
No further rule adaptation was introduced by acceptance.

## PC03 placeholder guardian adaptation

`goblin.guardian.placeholder` reuses the existing melee goblin definition and art,
with 14 HP instead of 7. Other stats, attacks, loot and 50 XP remain unchanged.
This is an intentionally easy, uncalibrated project adaptation for progression
verification. It is not a new SRD stat block, monster level system or final depth
band. One ordinary archer escorts it; both designated deaths unlock the gate.

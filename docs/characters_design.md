# Characters and preset party (slice 02)

Implemented extension: [slice 05 melee combat](melee_combat_design.md). Slice 05 adds authoritative health/death-save operations and locks formation during combat. Starting builds, derived caches and catalog schema are unchanged. The remainder records this subsystem’s original slice boundary.

Status: implemented and accepted by the user (2026-10-25). Delivery status
belongs to the [roadmap](dnd5e_demo_design.md#slice-02-characters-and-preset-party).

## Responsibilities and ownership

`blobber_rules` is extended with immutable race/class/item/preset definitions and pure
character derivation. JSON remains in `blobber_rules_io`; `blobber_core` owns the
small party runtime. The application loads the catalog, constructs the party,
then prepares the world/images. The catalog outlives the party. Movement remains
the only owner of the party's shared world pose; formation is relative to facing.
No renderer, UI, monster, combat, inventory transaction, save, or progression work
is included. No production RNG is needed for fixed builds.

Catalog definitions are contiguous, immutable, and bounded: existing limits plus
16 races, 16 classes, 64 items, and an optional exactly-four-character preset.
Typed IDs are non-recycled indices local to their catalog; invalid lookups return
null. External references use stable strings. Race variants are complete records,
independent of classes; there is no racial class whitelist or inheritance graph.

Each build has base abilities, race/class IDs, level 1, a Defense selection, at
most 16 owned starting item entries, and body/main-hand/off-hand equipment indices.
An entry owns a positive uint16 quantity of one definition. Equipment borrows an
entry, never copies it; equipped entries must have quantity one and cannot occupy
two slots. Entries never recycle in this slice. This preparation bound does not
choose slice 11's inventory grid size. Items carry descriptions, integer hundredths
of pounds, and typed weapon/armor properties; totals use int64 arithmetic.

Party owns four fixed character records, initialized resources, and a permutation
of stable CharacterIds in front-left/front-right/back-left/back-right order.
CharacterIds are party-local; OwnedItemIds are character-local. Neither may be
used with another owner. No characters are recycled or removed. Read-only access
and checked formation swaps
are the only runtime interfaces. Rejected swaps leave state unchanged. Resource
spending, damage/death, healing, and rest operations belong to subsequent slices.
`Party::snapshot()` returns a borrowed const reference; retain that reference for
frame queries rather than copying its startup-owned strings. Party construction
from a temporary catalog is disabled. Moving/destroying a catalog invalidates its
borrowers. Names/builds and derived caches are copied once at party preparation.

## Derivation and validation

Pure derivation returns a small error enum plus fixed-size results, without heap
allocation or exceptions. Derive final abilities, modifiers, level proficiency,
six saves, initiative, HP maximum, AC, carrying capacity/weight, and previews for
owned weapons. Level-one HP uses maximum class Hit Die plus Constitution (minimum
one) and the racial HP bonus. The four class save proficiencies are SRD values.
Armor and shield use requires proficiency; untrained weapons remain usable but
receive no proficiency bonus. Finesse picks the better Strength/Dexterity modifier
(ties use Strength); ordinary ranged weapons use Dexterity. Preview damage adds
that modifier once. A versatile weapon uses its two-handed die only when equipped
in the main hand with the off hand empty; carried preview defaults to one hand.
A two-handed main weapon forbids an off-hand item. Off hand accepts shields only;
dual wielding is outside this slice. Unarmored AC is 10 + Dexterity, light armor
uses full Dexterity, medium caps its upper contribution at +2, heavy ignores it.
Shield adds +2; Defense adds +1 only with body armor. Negative Dexterity is never
clamped away for light/medium armor. Carried weapons do not affect AC or occupy
hands. All weight, including equipment, counts once against Strength × 15 lb.

Validation rejects invalid IDs/enums, invalid ability ranges, unsupported levels,
malformed gear, unsupported Defense selections, overweight builds, and invalid
formation. Bounds are checked before indexing or arithmetic. In-memory catalog
construction validates just as JSON loading does. No partially valid catalog or
party escapes failure. Definitions contain only implemented mechanical properties;
other traits are recorded below rather than advertised as active.

## Content and compatibility

Schema 2 adds `races`, `classes`, `items`, and `preset_party` to the existing root.
The preset may be null for rules-only catalogs. Its `characters` array stores
stable `id`, display `name`, `race`/`class` string references, six base `abilities`,
`level`, `defense`, `items` (`item` reference and `quantity`), and `equipment`
(`body`, `main_hand`, `off_hand`: null or zero-based owned-entry indices). Its
`formation` array names each character ID exactly once, in positional order.
Class `weapons` accepts supported weapon names or `simple`/`martial` groups;
racial training uses the same format. Groups cover the 16 currently represented
weapon identities, not an import of all SRD equipment. Class `saves` requires two
distinct ability names; armor training is a subset of light/medium/heavy/shield.
Race ability increases are 0..2 and racial HP bonus 0..1 for this content set;
class Hit Die supports d6/d8/d10/d12. These are explicit engineering bounds.
Version 1 retains its strict old shape and creates no character definitions/preset. Startup requires a preset and
reports its absence before map/image/window preparation. Unknown versions, fields,
keys, enums, unresolved references, and exceeded capacities fail with diagnostics.
The 1 MiB/depth-16 parser limits remain. Names are 1..64 printable ASCII bytes
with a nonspace character; item descriptions use the same rule with a 512-byte
limit. Stable IDs retain the foundation's 1..128 nonspace ASCII limit. Item fields
are selected by kind rather than accepting unused properties. Diagnostics can validate either version and
print a complete party summary when present. Shipped assets use schema 2.

| Position | Name / race / class | Final STR/DEX/CON/INT/WIS/CHA | Gear | HP / AC |
| --- | --- | --- | --- | --- |
| Front left | Alden / Human / Fighter | 16/14/15/9/13/11 | Longsword, chain mail, shield | 12 / 19 |
| Front right | Brunna / Hill Dwarf / Cleric | 13/10/16/8/16/12 | Mace, scale mail, shield, holy symbol | 12 / 16 |
| Back left | Merrin / Lightfoot Halfling / Rogue | 8/17/14/12/13/11 | Shortsword, leather, carried shortbow and 20 arrows | 10 / 14 |
| Back right | Lethiel / High Elf / Wizard | 8/16/13/16/12/10 | Dagger, spellbook, arcane focus | 7 / 13 |

All builds use a permutation of the standard array before racial adjustments.
Start at XP 0, full HP, and one available class Hit Die. No mana value/formula is
introduced before slice 10. Names, formation, compact loadouts, and quantities
are demo choices, not complete tabletop background/equipment packages.

| Feature group | Slice 02 treatment |
| --- | --- |
| Racial ability increases, Dwarven Toughness, racial weapon training | Active numeric/proficiency inputs |
| Class Hit Die, saving throws, weapon/armor proficiency | Active |
| Fighter Defense style | Active when armored; other styles unsupported |
| Second Wind, Sneak Attack, Expertise, spellcasting, domain features, Arcane Recovery | Deferred to slice 10 |
| Lucky, Brave, Fey Ancestry, Dwarven Resilience, High Elf cantrip, hiding traits | Inactive; adaptation/selection belongs to slice 10 |
| Languages, tools, skills (including Keen Senses), senses, Trance, racial movement/size | Not simulated; document omission for later feature review |
| Spellbooks, focuses, holy symbols | Owned weighted items; no spell behavior yet |
| Thrown attacks, range, ammunition spending, loading, stealth penalties, armor speed/Strength requirements | Not simulated; ranged approach/equipment integration belongs to slices 08/11 |

Armor/shield proficiency is a build-validation requirement in this demo, rather
than simulating the tabletop penalties for wearing untrained armor. Off-hand
weapons are not supported. Finesse automatically chooses the best modifier;
versatile grip uses the free off hand, with no separate grip-selection UI.

## Validation strategy

Use small test-owned catalogs, JSON fixtures, and temporary output directories.
Cover presets, independent race/class pairings, all derivation rules above,
resource initialization, formation identity and rejection, invalid handles,
capacity/weight boundaries, malformed loading, and atomic failures. Validate the
shipped/copied catalogs separately from routine tests. Extend the frame allocation
guard over queries and successful/rejected swaps after preparation. Run Debug and
Release configure/build/CTest, startup failure-order checks, and an available
desktop capture smoke check; record unavailable checks rather than claiming passes.
Stop at Ready for review with a durable handoff; do not begin slice 03.


## Delivered interfaces and checks

- `rules/character.hpp`: bounded definitions/builds, `CharacterStats`, and
  `deriveCharacter`, returning `CharacterError` and zeroed stats on failure.
- `rules/content.hpp`: immutable catalog lookups and optional `preset()`.
- `game/party.hpp`: validated `Party`, const snapshot/character/occupant access,
  and checked `swapFormation`. Swaps leave character addresses/resources stable.
- `assets/rules_character_loader.*`: preparation-only schema-2 parsing, using
  the foundation's shared strict JSON helpers. No JSON enters rules or game code.
- The rules-content `--validate FILE` diagnostic loads either schema and prints
  a complete party summary when a preset exists; absence is reported explicitly.

[Slice 02 verification](slice02_verification.md) records executed checks and review
steps. Later integration must introduce authoritative resource/equipment mutation
operations before exposing mutable UI commands; it must not mutate cached stats
or const snapshots. This slice has no mana or active spell list.

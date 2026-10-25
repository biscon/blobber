# Rules foundation (slice 01)

Status: implemented and accepted by the user (2026-10-25).
The user approved the slice 01 plan and chose editable JSON content.
See the [roadmap](dnd5e_demo_design.md) for delivery and acceptance status and
the [source ledger](srd_content_ledger.md) for adopted rules and adaptations.

Implemented extension: [slice 02 characters and party](characters_design.md)
adds catalog schema 2, race/class/item definitions, pure stat derivation, and a
startup party. This document preserves the slice 01 foundation contract; its
original version-1 JSON shape remains supported as rules-only content.

## Boundaries and ownership

`blobber_rules` contains dice, pure resolution, and immutable content values. It
has no JSON, graphics, world, or clock dependency. `blobber_rules_io` parses and
validates JSON using the existing nlohmann dependency. The application loads one
catalog before map/image preparation and owns it for the session. The slice 02
party now consumes character/equipment definitions; monster gameplay behavior is
not connected yet. Mutable actors, turn scheduling, death saves, mitigation, effects, and spatial
targeting remain in their designated slices.

Catalog vectors are contiguous, bounded, and immutable after construction.
MonsterId and AttackId are distinct indices, local to one catalog lifetime;
they never recycle. Default/out-of-range handles return null. Do not use handles
across catalogs or persist them: stable string IDs are the external identifiers.
Borrowed definition pointers last only as long as the owning catalog and must not
survive its move/destruction. String lookup is linear over at most 64 records,
used at preparation boundaries; runtime consumers retain typed indices.

## Rules interface and numerical contract

Six abilities use scores 1–30, floor((score - 10) / 2), and a level-1–3 proficiency
query (+2); unsupported levels fail. Rules calls return a value plus a small
error enum, without exceptions or heap allocation. Arithmetic uses int64 totals
with int32 modifiers/targets, preventing overflow for all accepted inputs.

Dice expressions allow 1–32 d4/d6/d8/d10/d12/d20/d100 plus a signed modifier.
Critical damage doubles the dice (up to 64), applies the modifier once, and
clamps final damage to zero. Results retain fixed-capacity rolled faces and a
total. Damage types initially cover bludgeoning, piercing, and slashing.

An explicitly seeded caller-owned `std::mt19937` supplies raw 32-bit words.
For a die with n sides, reject words below uint32(-n) % n, then return word % n
+ 1. This defines the sequence independently of standard-library distribution
implementations. Rejection sampling has expected constant cost; the dice count
and storage are bounded, but there is no artificial retry cap that biases rolls.
No global engine, clock seed, or RNG serialization is introduced. Saving RNG
state remains a later persistence concern.

D20 rolling consumes one die normally, two for advantage/disadvantage; any
presence of both cancels to one. Pure attack/save resolution accepts a validated
d20 result and retains it in the outcome. Attack natural 1 misses, natural 20
critically hits, and other totals >= AC hit. Ordinary saves use total >= DC
without natural-roll exceptions. Invalid requests fail before consuming RNG.
Pure resolvers reject inconsistent fabricated roll results.

## JSON and loading

The version-1 format has `schema_version: 1`, `attacks`, and `monsters`.
The shipped catalog now uses version 2, described in the character design.
Attack fields: `id`, `kind` (melee/ranged), `attack_bonus`, `damage` containing
`count`, `sides`, `modifier`, `type`. Monster fields: `id`, `abilities` containing
the six full lowercase ability names, `armor_class`, `hit_points`, and `attacks`
(string references). The format contains only implemented data, not placeholders
for features, ranges, equipment, or class rules.

Loading permits at most 1 MiB, 32 monsters, 64 attacks, eight distinct attack
references per monster, and IDs of 1–128 printable ASCII bytes without whitespace.
IDs are unique within their category; empty catalogs are legal. Each monster
requires 1–8 distinct attacks. AC is 1–30 and
HP is a positive int32. Numeric inputs must be integers within the public types.
Unknown fields, duplicate object keys, unsupported versions, nesting beyond 16,
unknown enums, invalid dice, and missing references fail with file/field context.
Parsing/building may allocate and throw; all storage is RAII-owned, and no partial
catalog escapes failure. The in-memory constructor also validates definitions.
No hot reload or mutation is provided.

The shipped content adopts the goblin's six abilities, AC 15, HP 7, and +4
scimitar/shortbow attacks (1d6+2 slashing/piercing). Existing NPC appearance IDs
and placements are unchanged and are not treated as monster handles.
Rules files and attribution are copied with executable assets.

## Validation and delivery

Headless rules tests cover golden RNG sequences, boundary values, deterministic
resolution, invalid requests preserving RNG state, critical damage, and enum
validation. Loader tests own isolated fixtures, including capacities, invalid
references, malformed files, and cleanup after failed loads. Extend the existing
allocation guard to valid rules calls and typed content access after preparation.
Run all Debug/Release CTest suites; separately validate shipped content and
application startup using valid/invalid copied assets, plus an available desktop
startup check. These checks do not assert third-party allocator behavior.

The `rules_content` test executable also accepts `--validate FILE` for explicitly
checking shipped/edited content; this diagnostic is separate from routine CTest
fixtures. [Verification and review instructions](slice01_verification.md) record
executed checks. Delivery stopped at Ready for review; user acceptance was
recorded on 2026-10-25. This slice does not make
goblins fight or alter rendering/movement.

# Slice 10: spells and class identity

Status: implemented, **accepted and closed** (2026-10-27). The user explicitly accepted
slice 10, including its spellcasting UI, native HUD and impact-alert corrections. See [verification and handoff](slice10_verification.md).

## Rules and boundaries

Deliver fourteen spells: Cleric Sacred Flame, Spare the Dying, Bless, Bane,
Shield of Faith, Cure Wounds, Guiding Bolt and Inflict Wounds; Wizard Fire Bolt,
Acid Splash, Mage Armor, Magic Missile, Burning Hands and Thunderwave. Healing
Word was explicitly replaced with Inflict Wounds. Hold Person remains a low-level
effect for slice 13, not a selectable spell. No advancement, inventory or resting.
Mana caps are 4/6/14 at levels 1/2/3, cantrips cost zero, first-level spells two.
All curated class spells are available without preparation management. Casting
uses one action or the existing shared six-gameplay-second character cooldown.

Fighter retains Defense and gains one Second Wind charge (action, 1d10 + level).
The Life Cleric gains heavy armor proficiency and +2 + spell level healing.
Rogue can melee from the rear without changing formation, respecting enemy
protection. Finesse/ranged hits gain 1d6 Sneak Attack with final advantage or an
acting front ally engaging the target cell, unless the final roll has disadvantage;
the accepted hit spends a six-second allowance, and criticals double its die.
Wizard has one safe Arcane Recovery charge restoring two mana at level one.
Recovery requires Exploration, no pending action and no aware/adjacent living
enemy. Charges persist until restart; rest refresh belongs to slice 12. Existing
racial numeric/training passives remain; other racial traits and skills are inactive.

Ranged hostile targets require six-cell range, conservative LOS, party view and
visible-body selection. Inflict Wounds uses ordinary exposed melee reach, without
a weapon requirement. Allies use portraits. Bless/Bane choose 1–3 distinct targets;
Acid Splash chooses 1–2 enemies in one cell. Magic Missile assigns three darts to
1–3 enemies; roll 1d4+1 once and multiply by assigned count per recipient.
Burning Hands/Thunderwave affect every enemy in the three clear-sight front cells,
including body-occluded/protected occupants. One damage roll, individual saves,
half rounded down on success. Party origin is excluded. Thunderwave pushes failed-
save survivors one cell along heading after damage, in actor-ID order, preserving
subslot or choosing first vacancy. Walls/full destinations block without damage.
Reuse 600 ms visual travel. Empty areas and invalid requests reject before costs/RNG.
Instant hostile effects provoke existing local alerts on resolution, even on saves.
Delayed spells and weapon attacks alert at impact (including misses), never merely
at acceptance. Missing/canceled targets do not alert; lethal hits still warn nearby
survivors. Normal sight detection remains independent. See [native HUD and impact
alert correction](native_hud_design.md).

## Ownership and execution

Immutable schema-4 catalog definitions use typed spell/feature identities, class
profiles and bounded spell lists (32 catalog spells, 16 per class). Versions 1–3
retain their existing behavior, with new capabilities absent. Party owns mana and
charges; combat owns action readiness and pending consequences. Checked commands
carry world/turn identity and up to three target assignments. No UI writes gameplay.

Fire Bolt, Guiding Bolt and Magic Missile release immediately, using existing
180–450 ms flights. Inflict Wounds strikes at 200 ms. Other spells and features
resolve at the cast timestamp. Delayed rolls are saved at acceptance; target
loss never retargets/refunds. Preserve pause, released-shot survival, cancellation,
chronological effect/impact ordering, damage/concentration and presentation holds.
Four fixed party payloads extend the existing one-action-per-attacker timeline;
12-recipient area scratch is separate from NPC pending storage. Guiding Bolt uses
one fixed marker per actor in the effect owner, refreshes to six gameplay seconds
on surviving hit, and is consumed by the next accepted attack roll (even a miss).
It does not stack, concentrate or affect saves/automatic hits. Expiry precedes ties.

Submission publishes its complete borrowed event batch, consumed exactly once
before any subsequent simulation operation. The direct returned outcome reports
acceptance. The 2,048-event bound covers 520 weapon release/impact notices, eight extra
spell-recipient impacts, the existing 512 effect allowance, 256 Guiding marker
endings and 64 synchronous spell/phase notices: a conservative 1,360 entries.
The synchronous area bound includes one cast, twelve saves, twelve damage results
and twelve moves; these operations do not automatically chain into more casts.
The 512 effect allowance retains slice 09's cleanup/concentration proof; only
four party casters are supported. A static assertion guards the aggregate bound.
Expanding caster populations or automatic casting must revisit it. Authoritative capacity rejection precedes
commitment; cosmetic replacement cannot discard gameplay. All runtime storage is
fixed/reused; content loading, preparation and diagnostics may allocate.

## UI and visuals

The implemented [casting UI revision](spellcasting_ui_design.md) owns Normal,
SpellPicker, Targeting and Resolving states. Dock slot 4 opens a compact paused
picker with seven scrollable icon/name/cost rows and a details column. Choosing
closes it without click-through. Pending selections remain separate from committed
combat state; Back restores the picker position, Cancel spends nothing, and focus
loss suspends input while preserving valid choices. Live changes remove invalid
recipients individually. Movement, caster and turn changes cancel stale casts.

Single targets use immediate body/portrait clicks; multi-target, fixed-area and
untargeted actions require confirmation in a shared casting dock. Portrait frames,
checkmarks and removable chips identify allies; manual dart allocation has small
count badges and pips. A shared authoritative query drives area validation,
resolution, floor tint and affected-enemy markers. The combat log stays visible.
Rogue normal Melee retains its rear exception. Mana, charges, concentration/status,
saves/healing and specific rejection reasons remain visible; concentration dismissal
is free. Seventeen painted spell/ability icons extend the existing atlas.

Prepared bolt/dart/pulse/healing glints retain projection, occlusion, source-free
startup and fixed presentation. Sixteen bounded feedback entries show a complete
area hit. Cosmetic overflow replaces oldest independently of simulation.

## Validation and delivery

Generated fixtures cover all spell/feature outcomes, class/resource checks,
invalid-operation RNG preservation, multi-target/areas/pushes, effect capacity,
Guiding Bolt/Sneak Attack boundaries, old content compatibility, exact impact
timing, interruptions, phase changes, restart and complete event consumption.
UI checks cover pause, live targeting, stale/held inputs, coordinates and captures.
Extend first/repeated-use allocation guards. Separate capacity/population stress
from routine tests. Run Debug/Release suites and focused ASan/UBSan. Provide
`blobber_spells_tests --trace`, `--spell-demo --seed 5` and
`--capture-spells DIRECTORY`. Use hidden application-owned captures and in-process
input only; no desktop automation. Record actual checks and limitations in
slice10_verification.md. User acceptance was recorded on 2026-10-27; slice 11 remains
Planned/unstarted.

## Implementation map and storage

- `rules/spells.*` supplies the typed fourteen-spell table. Catalog schema 4 binds
  localized names/descriptions and class spell lists to these fixed mechanics;
  malformed, duplicate, incompatible and over-capacity data rejects during load.
- `game/party.*` owns mutable mana and single-use charges. Life healing and heavy
  armor derive from the selected class profile; new capabilities default absent
  for old catalog schemas. No save format or rest reset has been introduced.
- `game/combat_spells.cpp` validates complete commands before cost, RNG or mutation,
  executes immediate consequences, and supplies delayed payloads to the common
  impact machinery. `combat_impacts.cpp` drains 260 main slots plus eight fixed
  extra-recipient slots. Four payloads each hold at most three saved outcomes.
  Extra shots retain their own identities/deadlines and extend their owner's hold.
- `game/effects.*` owns 256 actor-indexed Guiding deadlines using existing world
  identity; expiry participates in the same chronological drain as other effects.
  No new pool is needed. Invalid/dead actors cannot keep an active marker.
- `game/spell_commands.cpp` builds the bounded UI model: 20 book entries, three
  selected recipients, twelve area highlights and a 192-byte target summary.
  Names/descriptions borrow immutable catalog storage. `ui/spellbook.cpp` renders
  it; `ui/interaction.cpp` routes input without mutating simulation directly.
- `render/projectiles.cpp` reuses the prepared glint with spell-specific color and
  dart offsets. Sixteen immediate pulse slots and the existing bounded cosmetic
  player replace old cosmetics on overflow. They never decide damage or targets.
  Prepared raster memory remains 1,012,160 bytes; no new source art or startup
  decode path was added. Loading, capture export and test fixture construction
  remain outside the allocation-free frame contract.
- `app/spell_captures.cpp` owns diagnostic scenarios. Ordinary application code and
  existing capture helpers consume the full borrowed submission batch before the
  next simulation operation, exactly as update batches are consumed.

The fixed timeline adds a small bounded scan to existing deadline processing.
Areas scan at most 256 actor slots once, collect at most twelve occupied subslots,
and resolve in stable actor order. Multi-target sorting is over at most three
handles. This keeps the implementation contiguous and predictable without a
scheduler, per-projectile allocation, or new entity framework.

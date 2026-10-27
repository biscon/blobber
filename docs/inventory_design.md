# Inventory and equipment — slice 11

Status: implemented, **Ready for review** (2026-10-27); user acceptance pending.
The user approved the planning conversation with “Implement the plan.” See
[verification and handoff](slice11_verification.md). Slice 10 is accepted; slice 12
must not begin before this slice's user acceptance.

## Approved behavior

Four independent 8×6 bags plus body, main-hand, shield/off-hand and alternate
ranged equipment positions. Equipment leaves the bag; all weight counts once
against Strength × 15 lb. Preserve proficiency, Life heavy armor, Defense,
versatile grip and current shield/ranged incompatibility. Armor ends Mage Armor
immediately; removing it does not resurrect the effect.

Identical definitions stack: arrows 100, healing/mana potions 20, other items one.
Empty-cell drops move, unlike bag drops swap, matching drops merge what fits.
Explicit quantities are atomic. Split leaves at least one at source. Transfers
fill matching stacks then empty cells in row order, wholly or not at all.
Unequip needs space; replacement returns the old item to the vacated source cell.
No automatic removal of conflicting gear. All rejections conserve state and RNG.

Organization inside a bag is allowed during paused danger. Equipment/transfers
require stationary safe Exploration, no aware/adjacent living hostile or pending
attacks. Downed/dead characters retain their gear and can be managed when safe.

Healing potion: 2d4+2 HP, 0.5 lb. Mana potion: project adaptation, +2 mana,
0.5 lb. Everyone starts with two healing potions; cleric/wizard also have two mana
potions. Rogue retains 20 arrows. Conscious, able actors use their own potions on
any living ally; reject full resources/no mana capacity. Healing revives but never
resurrects; no Life bonus. Use costs one action or six-second character cooldown,
resolves immediately and uses existing feedback holds. Bow acceptance spends one
compatible arrow before rolls, without miss/interruption refunds. NPC ammo stays
abstract. Loot, arrow recovery, rest, progression, saves and dropped items deferred.

## Responsibilities and storage

Party is noncopyable/nonmovable so its handle identity and borrowed references
have one stable owner. Party owns a contiguous 208-stack inventory with four 48-cell bags and four
4-position equipment arrays. Typed handles carry owner identity, slot and
generation; movement preserves identity and consumption/complete merge invalidates
it. Exhausted generations retire their slot. Fixed scratch copies stage atomic
transactions and derived stats; no strings or growing containers enter this path.
GameSession gates safety and reconciles effects; Combat owns potion/action/ammo
acceptance and clocks. UI only submits requests and reads bounded snapshots.

Startup loadouts remain a separate immutable content representation; live equipment
queries must not read initial item indices. Pure derivation accepts current
loadouts. Catalog schema 5 adds typed ammo/consumables/stack limits; schemas 1–4
retain strict parsing and convert legacy quantities into legal runtime stacks,
rejecting capacity overflow. Definitions remain immutable, max 64 items.

Requests include expected inventory revision and checked handles. Results report
specific capacity, weight, ownership, compatibility, stale and safety failures.
All session calls invalidate the borrowed event batch; consume the whole batch
before the next simulation operation. Potion effects and costs commit together;
invalid requests spend no RNG. Existing impact outcomes and clocks remain intact.

## Presentation

Full-screen paused overlay: four character tabs, equipment silhouette left, 8×6
bag centrally and details/actions right. Open with I, inspector Inventory, or
backpack/5. Drag and applicable button actions share transactions. Drop onto another character's
tab transfers. Shift-drag to a valid target opens a split dialog on release,
default 1, with no mutation before confirmation. Escape/focus loss/invalidated
source cancels drag. Mouse down/release and held latches prevent click-through.
Keyboard navigation, contextual actions and quantity controls remain available;
move/merge/split use drag gestures (see the review correction below). Use exits browsing
into live ally targeting; combat actor is independent of the inspected character.

Generate fifteen painted item icons plus a neutral silhouette using built-in
imagegen. Preserve masters/prompts, existing portraits and the native UI render
path; prepare bounded padded atlas regions at startup. Dungeon banks unchanged.
Add --inventory-demo and --capture-inventory, with in-process scripted input only.

## Verification strategy

Test-owned catalogs/generated art, fixed RNG, isolated directories. Cover
conservation, ownership, stale/foreign/reused handles, full capacity, stack/weight
boundaries, atomic transfer/swap failures, equipment/stat/effect reconciliation,
potion revival/caps/restrictions, last-arrow spending and no refunds. UI checks
include drag/click parity, split default one, focus/held/release isolation, stale
dialogs, keyboard and aspect-fit mapping. Extend first/repeated allocation guards.
Run Debug/Release suites, focused ASan/UBSan, separate capacity/reuse stress, hidden
GPU lifecycle and native/reduced captures. Record unavailable checks honestly.
Finish Ready for review with a durable handoff, never self-accept.

## Implementation details

`Inventory` contains only fixed value storage (about 8.2 KiB on this build).
`Party::inventoryChange` stages a copy, applies structural operations, derives four
current loadouts, then commits the copy and stats. The bounded work is a few scans
of at most 208 locations plus four 52-entry derivations. Rejected staged partial
merges are discarded. `Inventory::apply` is a private staging primitive; only Party can mutate it.
`Party::previewInventoryChange` uses the same staging/derivation for valid split
destinations without committing. Session callers use `inventoryChange`.
Consumption only accepts bag locations. Stack generation and inventory revision
counters never wrap during mutations; exhausted slots are retired and revision
exhaustion rejects. No new per-frame containers or polymorphic dispatch were added.

`CharacterIdentity` separates immutable identity/build strings from startup gear.
`CharacterLoadout` supplies pure derivation with sparse current inventory. Runtime
weapon previews use positions 49 and 51, and effects/combat/inspectors query actual
equipped definitions. UI cells borrow immutable definitions and copy small derived
weapon previews. Armor changes reconcile effects before returning the borrowed
session event batch. UI consumes that batch before the next session operation.

`combat_items.cpp` validates the turn/world token, owner, stack generation,
inventory revision, acting state and recipient before committing costs and rolls.
Healing emits one item-labelled restoration event; mana emits one recovery event.
Both share existing readiness and the 1000 ms feedback hold. Ranged acceptance
consumes ammunition before attack RNG; delayed impact cancellation cannot refund it.

`inventory_commands.cpp` owns selection, drag, split dialog and pending item use;
`inventory_interaction.cpp` maps pointer/key edges to those operations. Focus loss
cancels drags; inventory revision changes cancel pending destinations. Direct stale
confirmation is also rejected authoritatively. Button/drag transfers merge in row
order; equipment replacement swaps back into the emptied source. Full inventory
browsing is paused; choosing Use closes it and resumes live recipient targeting.
The viewed character remains separate from the combat actor and formation slot.

The native HUD renderer draws the inventory after other overlays. The UI atlas is
2048×2048 RGBA (16 MiB), preserving old regions and adding fifteen 168-pixel item
regions and a 384×600 silhouette region, with two-pixel edge extrusion. It is
prepared/uploaded once. Master PNGs and generation prompts are retained under
`assets/ui/items/`; the dungeon prepared banks are unchanged. Measured description
lines use fixed scratch storage and four-line pages; wheel/Page Up/Page Down reach
all accepted 512 bytes. Weapon details display current attack/damage bonuses.

The capture runner uses real sessions and the production input router, with empty
placements for safe management, ordinary placements for danger, and a catalog clone
with 100 arrows and long text for the full-bag/page stress image. These diagnostic
preparation changes are never applied to normal gameplay. All windows are hidden;
no external window or OS input is controlled.

## Review correction: simpler inventory controls (2026-10-27)

Implemented from the user's screenshot review; Ready for review. This supersedes
slice 11's original Move/Split button and keyboard destination modes. Drag/drop
organizes and merges stacks; Shift-drag opens the quantity dialog. Keep only
contextual Equip/Unequip, Transfer and Use Potion actions. Gear-only items such as
holy symbols, spellbooks and arcane focuses expose Transfer, without suggesting
unimplemented abilities. Applicable actions temporarily blocked by safety, capacity,
training, turn, cooldown or recipient state are visibly disabled with a reason.
The controller builds a fixed action snapshot using the same transaction previews
and combat validation as submission; both clicks and shortcuts consult it.
This adds bounded preview work for at most three actions/four recipients per refresh,
with no retained transaction copies or frame-time allocations. Labels/reasons point
to static strings; character/item ownership and mutation stay with Party/GameSession.

Move the body slot onto the torso and ranged slot below the main hand, alongside
the silhouette. Leave head and feet unobstructed for future slots without adding
those slots now. Center inventory button captions using font measurements; use a
larger prepared font for split +/- controls. Align stack badges to the right with
consistent padding. Add an explicit always-visible drag/Shift-drag hint.
The 48-pixel bold control font is loaded once alongside the existing size-specific
atlases and destroyed with UiFonts. Shared layout functions compact visible action
rows identically for painting and hit testing.

I remains the inventory shortcut (already mapped, now regression-tested); M replaces
F1 for map toggling. Remove the obsolete M/S inventory command bindings. Update the
on-screen map hints and README. Replace the dock potion artwork with a separately
saved painted backpack and retain its generation prompt/master.
The dock tooltip names Inventory and shows its I/5 shortcuts.

Verify contextual actions and unavailable shortcuts, drag/split/merge conservation,
I opening, quantity-confirmation isolation, allocation guards, hidden GPU lifecycle,
and native/reduced review captures. No new D&D abilities or later-slice features.

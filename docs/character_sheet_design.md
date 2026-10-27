# Tabbed inventory and character sheet

Approved mockup: [Character v1](mockups/character-tab-v1.png). The user approved
implementation on 2026-10-27. Status: implemented, accepted and closed with slice 11 on 2026-10-27.
See [verification and screenshots](character_sheet_verification.md).

Replace the small character inspector with the existing paused fullscreen overlay.
Inventory and Character are clickable tabs in the title band; both share the
unchanged four-character selector. Inventory contents, positions and gestures stay
unchanged. I/backpack/5 opens Inventory; a portrait info button opens Character
for that portrait's identity. Switching tabs keeps the viewed character, selected
inventory item and cursor, without changing the active combat actor. Switching
cancels pending drag/transfer/quantity state without committing it. Closing consumes
the input frame. Character content never routes inventory gestures or shortcuts.
Character-only inspection preserves a pending spell as the old inspector did;
closing resumes targeting. Entering Inventory cancels targeting, as before.

## Ownership and presentation

Extend InventoryHud with a two-value page enum and a fixed CharacterSheet snapshot.
CommandController owns selection/navigation and prepares the sheet from Party,
Catalog and live Effects. Party remains the authority for identity, XP, resources,
equipment and derived statistics; Effects supplies effective AC and active effects.
No copying of Party, mutation from rendering, or additional gameplay clock is added.
Remove the obsolete inspected modal and inspector renderer/layout/input paths.
The existing preview-only controller also supports read-only sheet browsing.

Keep sheet preparation in a cohesive game source and rendering in a separate UI
source. Reuse the prepared portrait/item atlas and fonts; no new image generation
or frame-time asset work. Bounded arrays/static strings hold feature rows, saves,
proficiency text and effects. All capacities cover the supported six ability scores,
six feature rows, two equipped attacks, and sixteen weapon training bits; work is
bounded and allocation-free. Catalog pointers remain valid for the session lifetime.

The approved three-column sheet shows identity/formation, portrait, level, XP and
Hit Dice; HP, effective AC, initiative and proficiency; equipped melee/ranged attack
bonuses, damage dice/range/type and ammunition; class features and active effects;
ability scores/modifiers/saves with proficiency marks; armor/weapon training. Casters
also show mana, spell attack and save DC. Unarmed/empty ranged slots and dead/dying
characters must display truthfully. Damage summaries are ordinary equipped weapon
previews; conditional Sneak Attack, Bless/Bane and other effects remain separate.

The current supported level is one: show stored XP against the ordinary 300-XP
level-two threshold and clamp only the progress bar/remaining value. This is a
read-only presentation threshold, not XP awards or advancement. Reserve space in
the progression section for a future conditional level-up entry; no inactive fake
button or progression flow now. Future progression should supply thresholds and
route a level-up flow from this page, retaining the same character identity.

## Verification

Use existing test-owned catalogs. Check both entry routes and tab directions,
selection across formation changes, cancellation of pending inventory operations,
hidden inventory input isolation, modal pausing/close consumption, live equipment
and effect/resource updates, saves/proficiencies and empty slots. Extend first and
repeated allocation checks. Run affected CPU suites and hidden application-owned
captures of all four character sheets, Inventory after switching, changed gear,
and active effects at native/reduced/letterboxed sizes. No desktop automation.

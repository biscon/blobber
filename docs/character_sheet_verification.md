# Character sheet verification and handoff

Status: **Ready for review**, 2026-10-27. The user accepted the generated visual
mockup and authorized implementation. This remains part of slice 11; implementation
acceptance is pending and no later slice has started.

## Delivered

The approved [design](character_sheet_design.md) is implemented with shared
Inventory / Character navigation and the existing character selector. Portrait
info opens that character's sheet; I/5/backpack opens Inventory. Both tabs preserve
the viewed identity and inventory selection, while canceling unfinished inventory
operations. The old inspector modal, renderer and input paths were removed.
Inventory geometry and behavior below the title band are retained.

The sheet shows live identity, formation, portrait, level/XP, Hit Dice, HP/mana,
effective AC, initiative, proficiency, equipped attacks and ammunition, ability
scores/modifiers/saves, armor/weapon training, class features/uses and active effects.
All four classes use current data and existing prepared art/fonts. Empty weapon
slots show Not equipped. Character-only inspection preserves pending spells;
entering Inventory cancels targeting, matching its existing behavior.

XP is read-only: the existing stored XP is shown against the level-two threshold
of 300. XP awards, advancement, new racial mechanics/skills and a level-up flow
are not implemented. The progression area reserves room for the future entry.
Weapon previews are ordinary equipment-derived attacks; conditional damage and
effects are shown separately. Feature rows describe existing capabilities; they
are not activation controls.

## Checks

| Check | Result |
| --- | --- |
| Focused Debug | 6/6 passed: inventory, spells, hybrid, HUD, commands, frame allocations; 14.79 s |
| Focused Release | Same 6/6 passed; 1.83 s |
| Final drag-across-tab regression | Inventory suite passes again in both builds |
| Allocation guard | Zero tracked C++ allocations including repeated sheet preparation and tab switching |
| Hidden GPU smoke | Passed; existing prepared atlas/resources remain intact |
| Final Debug application build | Passed without compiler warnings |
| Hidden in-process capture run | 77 PNGs; native 1920x1080, reduced 1280x720, letterboxed 1024x768 |

Regression coverage includes portrait entry after formation swaps, both tab
directions, selected-item preservation, split/drag/transfer cancellation, hidden
inventory input isolation, gameplay pause and actor isolation, pending-spell
preservation, live effective AC/HP/mana, equipment and versatile damage changes,
empty equipment, class feature charge changes, race/class proficiency unions,
and existing inventory/combat behavior. Tests own their catalogs; captured real
content is separate diagnostic evidence. Initial testing corrected a test that
incorrectly treated Mage Armor as concentration; the sheet correctly does not.

Representative native/reduced captures were visually inspected, including actual
armor/spell effects and the return to Inventory. No mouse/keyboard injection or
window focus control was used. Interactive review remains unverified. Full CPU
and sanitizer suites were not repeated for this UI change; the affected suites
above and GPU integration were run.

## Review images

- [Rogue, native](screenshots/character-sheet/character-rogue.png),
  [reduced](screenshots/character-sheet/character-rogue-1280.png),
  [letterboxed](screenshots/character-sheet/character-rogue-4x3.png).
- [Fighter](screenshots/character-sheet/character-fighter-1280.png),
  [cleric](screenshots/character-sheet/character-cleric-1280.png),
  [wizard](screenshots/character-sheet/character-wizard-1280.png).
- [Shield removed: AC and versatile damage](screenshots/character-sheet/character-shield-removed-1280.png),
  [Mage Armor and spent mana](screenshots/character-sheet/character-active-effects-1280.png).
- [Inventory after switching](screenshots/character-sheet/inventory-from-character-1280.png).

Run `cmake-build-debug/blobber --inventory-demo --seed 5`, then click Character.
For the portrait entry, close the overlay and click a portrait's i control.
Reproduce all images with `--capture-inventory DIRECTORY --seed 5`.

After user acceptance, update the slice 11 checkpoint/tracker/execution record.
Do not infer slice acceptance from approval of the mockup or these checks.

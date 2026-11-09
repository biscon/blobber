# Early equipment review evidence

Status: implementation mechanically verified; **interactive user review pending**.
These captures do not imply user acceptance. Four selected images are retained.

| Evidence | Case |
| --- | --- |
| [Smithy weapons](smithy-weapons.png) | New greatsword with real attack/damage preview and explicit shield conflict. |
| [Magical equipment](magic-equipment.png) | All eight wearable categories equipped simultaneously, +1 dagger, selected cloak details; no attunement or item-count cap. |
| [Final chest](final-chest-active.png) | Depth III guardian chest opened, objective completed, HUD remains in Exploration. |
| [Artwork contact sheet](artwork-contact-sheet.png) | All 31 generated item icons, including six +1 variants. |

The superseded innkeeper armor capture was removed after relocating that stock to
the smithy. That configuration change was checked through the native shop loader;
its stock placement has not been recaptured.

Game captures use the production catalog, artwork, presenter and OpenGL integration
through `blobber_review_early_equipment`. Windows remain hidden, setup/input are
in-process, and no desktop mouse, keyboard or focus control is used. The isolated
fixture grants equipment and defeats enemies for presentation; the displayed
loadout is not a normal campaign's reward distribution or a balance measurement.
The actual campaign contains three wearable rewards and two distinct +1 weapons.
The final chest capture also passes active-campaign checkpointing and GL error
checks. The contact sheet is a review composition; original PNGs retain generated
transparency and have individual exact prompt files.

[Design, verification commands and limitations](../../early_equipment_design.md).
[Source artwork and prompts](../../../assets/ui/items/README.md).

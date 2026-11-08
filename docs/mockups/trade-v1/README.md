# Merchant trade — presentation exploration v1

2026-11-07. **Accepted with notes.** The user approved this generated presentation
reference, requiring uniform square slots, 350 ms item tooltips on both sides,
and comparison against the selected character's equipment in the matching slot.
Temporary innkeeper weapon and armor stock is approved for comparison testing.
Implementation is tracked in the [merchant design](../../merchant_design.md).

## Presentation

The user selected two item grids: merchant left, selected character right. Overlay
the lower world area without resizing the world viewport, keeping the innkeeper
and room visible above. Preserve the existing four party cards in the right
sidebar and highlight Alden as the current buyer/seller. His bag uses its existing
8×6 arrangement. Merchant grid pages are presentation only, not a committed stock
capacity. Match the native 1920×1080 presentation and existing dark green/gold UI.

Show finite ration-pack, healing-potion and mana-potion stock, plus one clearly
marked recently sold leather armor available to buy back. The shared selection
area shows item information, unit price, quantity, total and an explicit Buy or
Sell action. Include available character gold, carrying weight, restock time,
buyback expiry and a Back to conversation control. Values are illustrative,
not approved balance or final timing rules. The selected ration pack illustrates
the buying state; selecting an item in the character bag would show Sell instead.

## Later implementation requirements

Trade opens from a dialogue topic. Vendor assignment is data-driven, with a
reference to separately authored shop stock and prices. Use existing stable NPC
identities and campaign time; the exact data ownership and interfaces require a
subsequent implementation design. Stock is finite and refills after a configurable
number of game days. Sold items join vendor stock at a markup for buyback and
expire within a configurable period around one day, without accumulating forever.
The selected portrait determines whose inventory buys or sells. Potions are
temporary innkeeper offerings for testing; the intended later home is the
provisioner. No gameplay, dialogue YAML, item catalog or save data changes here.

## Artifact and verification record

- [Selected trade screen](trade-screen-v1.png)
- [Exact built-in ImageGen request](trade-screen-v1.prompt.json)
- [Innkeeper/world/sidebar reference](../../screenshots/dialogue/dialogue-topic-idle.png)
- [Inventory/artwork reference](../../screenshots/slice11/inventory-fighter.png)

Generated using the built-in imagegen tool, with the two runtime captures as
references. The selected PNG is retained at its native **1717×916** size; the
requested implementation target remains **1920×1080**. Generated slot proportions,
portrait details and typography are presentation references, not pixel-exact
runtime geometry. The engine's existing card layout and square bag slots remain
the implementation reference. The pictured ration icon is concept artwork only.

Visually inspected both eight-column/six-row grids, readable labels and sample
prices, finite quantities, marked buyback item, selected ration, quantity/total/
Buy controls, restock/expiry text, weight, conversation return and all four visible
party cards. Alden's highlight, bag heading and purchase button identify the same
character. The PNG passed Pillow's read-only format/integrity check; the request
passed JSON parsing and local Markdown links were checked. No gameplay build,
GPU test, desktop interaction, benchmark or gameplay change was made. Runtime
behavior, timing, resource handling and economy balance remain unverified.

Only the selected image and exact prompt are retained here as durable design
inputs. The owned disposable run is cleaned with the artifact helper's preview
and apply workflow after promotion, with no retained diagnostic exception. The
generator's original output is left in its managed location per imagegen policy.
Mockup approval is recorded above. Runtime appearance and interaction remain a
separate acceptance step.

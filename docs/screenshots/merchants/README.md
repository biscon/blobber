# Merchant trade runtime review

2026-11-07. **Ready for user review; runtime acceptance pending.** The user accepted
the [mockup](../../mockups/trade-v1/README.md), with uniform slots and 350 ms item
tooltips comparing the selected character's current equipment. These are native
1920×1080 hidden OpenGL application captures of the implemented interface, refreshed
after the interaction review. The user accepts comparison presentation; gesture
and revised presentation review remain pending.

| Capture | Review case |
| --- | --- |
| [Buying supplies](trade-buy.png) | Square grids, 50%-opacity label backgrounds, close X, personal gold and no B hint. |
| [Weapon comparison](trade-weapon-comparison.png) | Brunna's hovered longsword beside her equipped mace, including attack, damage and training differences. |
| [Armor comparison](trade-armor-comparison.png) | Merrin's chain mail beside equipped leather, including AC and a red missing-training restriction. |
| [Selling and bag comparison](trade-sell-comparison.png) | Alden's bag dagger beside equipped longsword, with an explicit Sell action. |
| [Buyback](trade-buyback.png) | Sold dagger reappears as an ordinary purchase at 3 gold after being sold for 2, without a B badge. |
| [Dragging to buy](trade-drag.png) | Item preview and destination highlight during a normal buyback drag into the player grid. |

The review runner creates its own campaign and grants isolated example inventory;
no user save is opened or modified. It exercises a 120-gold ration purchase, sale,
buyback pricing and drag purchase, selected characters, frozen game clock, mouse-X
return and checkpointed stock. All world textures/alpha residency are zero after view teardown. Tooltip
panels stay inside the world/trade region and leave all portraits visible. Hover
boundaries, empty equipment slots, focus/press cancellation, paging and rejection
paths, double-clicks, both drag directions, chosen quantities and successive Sell
clicks are covered by owned CPU fixtures. See [design and verification](../../merchant_design.md).

No OS input injection, focus changes or desktop automation was used. Runtime
interactive feel, economy tuning and user acceptance remain pending. Six selected
captures are retained; disposable output and logs are removed using the owned
agent-run cleanup workflow.

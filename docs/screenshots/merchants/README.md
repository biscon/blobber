# Merchant trade runtime review

2026-11-07. **Ready for user review; runtime acceptance pending.** The user accepted
the [mockup](../../mockups/trade-v1/README.md), with uniform slots and 350 ms item
tooltips comparing the selected character's current equipment. These are native
1920×1080 hidden OpenGL application captures of the implemented interface.

| Capture | Review case |
| --- | --- |
| [Buying supplies](trade-buy.png) | Square 8×6 grids, finite prices/quantities, personal gold, weight, selected trader and purchase controls. |
| [Weapon comparison](trade-weapon-comparison.png) | Brunna's hovered longsword beside her equipped mace, including attack, damage and training differences. |
| [Armor comparison](trade-armor-comparison.png) | Merrin's hovered chain mail beside equipped leather, including AC and missing heavy-armor training. |
| [Selling and bag comparison](trade-sell-comparison.png) | Alden's bag dagger beside equipped longsword, with an explicit Sell action. |
| [Buyback](trade-buyback.png) | Sold dagger reappears with B marker at 3 gold after being sold for 2. |
| [Conversation return](trade-return.png) | Escape returns to the innkeeper's topic list and preserves conversation framing. |

The review runner creates its own campaign and grants isolated example inventory;
no user save is opened or modified. It exercises a 120-gold ration purchase, sale,
buyback pricing, selected characters, frozen game clock, return and checkpointed
stock. All world textures/alpha residency are zero after view teardown. Tooltip
panels stay inside the world/trade region and leave all portraits visible. Hover
boundaries, empty equipment slots, focus/press cancellation, paging and rejection
paths are covered by owned CPU fixtures. See [design and verification](../../merchant_design.md).

No OS input injection, focus changes or desktop automation was used. Runtime
interactive feel, economy tuning and user acceptance remain pending. Six selected
captures are retained; disposable output and logs are removed using the owned
agent-run cleanup workflow.

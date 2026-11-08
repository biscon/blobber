# Merchant trade and party purse runtime review

2026-11-07. **Ready for user review; runtime acceptance pending.** The user accepted
the [mockup](../../mockups/trade-v1/README.md), uniform slots, delayed item tooltips
and the comparison presentation. These six native 1920×1080 hidden OpenGL captures
refresh the evidence for the shared, weightless party purse. This cleanup does
not imply acceptance of the revised gestures or purse presentation.

| Capture | Review case |
| --- | --- |
| [Inventory purse](inventory-purse.png) | Shared 879 gold above the bag, right-aligned to the grid border; no coin stack. |
| [Inventory large balance](inventory-large-purse.png) | Largest uint64 balance fits above the grid without colliding with headings. |
| [Buying supplies](trade-buy.png) | Shared 1000 gold right-aligned to the player grid, lowered onto the subtitle row; no inventory currency. |
| [Trade large balance](trade-large-purse.png) | Largest uint64 balance fits without overlapping the helper text or close X. |
| [Weapon comparison](trade-weapon-comparison.png) | Switching to Brunna retains the shared 880 gold after a 120-gold purchase; current mace comparison and red training warning remain. |
| [Armor comparison](trade-armor-comparison.png) | Switching to Merrin retains 880 gold; equipped leather comparison and red missing-training restriction remain. |

The review runner creates its own campaign and grants isolated example inventory.
It buys a ration for 120 gold (1000→880), sells a dagger for 2 (880→882), verifies
3-gold buyback pricing, buys it back by dragging (882→879), switches characters,
returns through the mouse X and opens inventory. The two maximum-balance captures
change only a copy of the presentation snapshot, not the campaign balance.
Gameplay time stays frozen and the party does not move. All world texture/alpha
residency is zero after view teardown. No user save is used by this capture runner.

Owned CPU fixtures cover double-clicks, both drag directions, buyback, quantities,
successive Sell clicks, failure conservation, request replay, full bags, purse
overflow, weightless pickup and 350 ms comparison hover. The separate one-time
user-save edit was validated through the headless loader; see
[design and verification](../../merchant_design.md#party-purse-verification--2026-11-07).

No OS input injection, focus changes or desktop automation was used. Interactive
feel, economy tuning and user acceptance remain pending. Superseded screenshots
were replaced; exactly six images are retained. Disposable captures, logs and
save-validation copies are cleaned using the owned agent-run workflow.

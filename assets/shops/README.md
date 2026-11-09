# Shop inventories and prices

Each `.yaml`/`.yml` file defines one immutable shop template. Edit the file and
restart the game; no C++ changes or live reload are needed. The dialogue host's
file-level `shop` value refers to this `id`. Two residents can share a template
while retaining independent stock and sold items.

```yaml
version: 1
id: sanctuary_inn
restock_days: 3
buyback_hours: 24
buyback_markup_percent: 25
prices:
  healing_potion: {buy: 50, sell: 12}
stock:
  - {item: healing_potion, quantity: 3}
```

`buy` is what the character pays per item; `sell` is what the merchant pays them.
Every stocked item needs a price. Other priced items can be sold even if the shop
does not ordinarily stock them. Unpriced items and currency cannot be sold.
Prices are positive integer gold: buy at most 100,000, sell strictly below buy.
Use item IDs from `assets/rules/catalog.json`; unknown items and duplicate entries
reject with the filename. `camp_kit` is the existing rest supply, shown as
**Ration pack** in trade; its mechanical identity, weight, use and save fingerprint
are preserved. The UI reuses its existing prepared artwork.

Stock is finite. First visit sets the next delivery; each `restock_days` interval
refills stocked quantities to their authored targets. Missed deliveries do not
accumulate. Restock takes 1–365 elapsed game days, not real-world days; resting
advances the same campaign clock. Prices can change between sessions without
resetting saved stock. New stocked items remain empty until the next delivery in
an already visited shop. Removing an item from stock hides its remaining stock.

Sold goods appear in the ordinary merchant grid at the sale price plus the markup,
rounded up. Their tooltip identifies them as Buyback and shows the expiry. Buy
them with the regular button, double-click or drag into the character's grid;
there is no B shortcut or separate buyback control. Their individual prices and
expiry deadlines are saved. A sale of
10 gold at 25% markup costs 13 gold to buy back. The markup is 1–100%; held time
is 1–24 game hours. Restocking does not extend it. Buybacks are separate from
ordinary stock and disappear at their deadline. At 48 outstanding buyback stacks,
further sales reject without losing goods; wait for expiry or buy something back.

Gold comes from the shared, weightless party purse; goods use the selected
character's bag. Equipped items must be unequipped before sale. Purchases go into
that character's bag, respecting stack, space and carrying weight. Purchases do
not equip items automatically. Merchant funds are unlimited; item stock is finite.

Single click selects; double click buys/sells the selected quantity, default one.
Drag from either grid to the opposite grid to trade that quantity. Same-side and
outside drops cancel. After selling, a remaining source stack stays selected;
otherwise the next sellable item in forward bag order is selected at quantity one,
skipping unpriced items and stopping at the end without wrapping. This permits
successive Sell clicks through adjacent items. Rejected sales do not advance.
Escape, the top-right X and the mouse Back control return to dialogue.

Limits: 16 shop files, 1 MiB combined YAML, 48 stocked definitions per shop,
64 item prices, quantity 1–65,535 per stocked item and 16 visited vendors per run.
All runtime storage is bounded; shop parsing and save serialization happen outside
the frame path. Current innkeeper potions are temporary provisioner stock.
Duncan Ironballs at the smithy stocks all current weapons, body armor, shields
and arrows. The inn retains its former equipment prices for selling/buyback;
its temporary longsword and chain-mail retail stock has moved to Duncan.

The early equipment expansion adds the nine new body armors to Duncan's smithy,
alongside its existing armor, shields, weapons and ammunition. The inn stocks
camp provisions. Duncan also stocks rapier, battleaxe, warhammer, greatsword,
maul, longbow, light crossbow and bolts. New equipment sells for 25% of its ordinary purchase price (rounded down,
minimum one), except bolts use the existing arrow economy, 2/1 per unit. Magic
weapons have price entries at Duncan's, and magic wearables at the emporium, for
100-gold sales and ordinary temporary buyback. No merchant stocks or restocks
magical equipment; their 400-gold reference prices do not create retail stock.

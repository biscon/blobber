# Merchants and trading

Status: implemented and mechanically verified; runtime user review pending,
2026-11-07. The user accepted the
[two-grid mockup](mockups/trade-v1/README.md), with uniform square slots and item
tooltips after 350 ms on both sides. Equippable-item tooltips must also show the
selected character's item currently equipped in that slot, including its stats.
The approval authorizes implementation and temporary innkeeper weapon/armor stock.

## Ownership and authoring

Dialogue YAML supplies an optional stable `shop` ID and a `kind: trade` topic;
shop definitions live separately under `assets/shops/`. No general NPC registry
is needed for this concrete feature. Loading validates all dialogue shop references.
The immutable shop catalog owns prices and finite restock targets; it lives beside
the rules/dialogue catalogs for gameplay. No text parsing or file I/O during play.
Shop YAML includes `restock_days`, `buyback_hours`, `buyback_markup_percent`,
per-item buy/sell prices and stocked quantities. Currency cannot be merchandise.
Unpriced items cannot be sold. Buyback costs the sale proceeds plus the configured
markup, rounded up; ordinary retail prices must exceed selling prices.

CampaignRun owns a bounded merchant ledger keyed by stable resident and shop IDs,
independent of ephemeral actors and location sessions. Standalone gameplay owns
a corresponding ledger. At most 16 visited vendors, 48 authored stock entries per
shop and 48 buyback stacks per vendor. Counts/prices are bounded at load time;
fixed runtime arrays keep updates, inspection and transactions allocation-free.
Full buyback storage rejects a sale explicitly rather than deleting an item.
Display stock/buyback pages contain 48 cells; bought-out authored stock remains
visible with zero quantity. Buyback entries reuse slots only after expiry or
purchase; requests validate resident/shop identity and ledger/inventory revisions.

## Transactions, time and saves

Gold remains physical inventory currency owned by the selected character. Buying
and selling only affect that character's bag; equipped items must first be
unequipped using the existing inventory screen. The merchant has no finite purse.
Party stages an inventory copy, removes goods/payment, adds goods/proceeds, checks
bag and weight constraints, then commits together with vendor state. Explicit
quantities are atomic: insufficient funds, capacity, weight, stock or stale
requests change neither side. Prices and arithmetic use checked bounded values;
currency is never taken from other party members automatically.

Restock and expiry use existing elapsed campaign GameTime. First visit starts
the restock deadline; each due interval refills authored quantities to their
targets, without accumulating missed deliveries. Large time jumps use arithmetic
rather than per-day loops. Each sale expires after the shop's buyback duration;
restocking never extends those deadlines. Refresh at entry and transaction time,
with bounded work; conversations and trade freeze gameplay time.

Checkpointing serializes only visited vendors. Save schema advances to 13,
simulation remains 17; accept existing schema 12/simulation 17 by initializing an
empty ledger. Item catalog and NPC fingerprints remain unchanged, avoiding a new
campaign requirement. Validate restored vendor capacity, identities, stock counts,
buyback item/quantity/price/deadlines and revision before publication. Shop price
edits apply to ordinary trades; buyback prices already recorded stay fixed.

## Input and presentation

Trade is a subview of the live conversation: keep its world camera, resident Idle
and ambient audio, hide speech/topic/overlay cues and return to the same conversation
with Escape/Back. Keep the world render target and four party cards unchanged.
Trading blocks world input, inventory hotkeys, saving/travel and other modals.
Use existing physical press/release capture and context revisions to prevent
click-through; focus loss cancels presses and pending hover.

Both grids use the same 8×6 square-cell geometry. Merchant left, selected bag
right; character cards and 1–4 change the buyer/seller, resetting selection,
quantity and hover. Cells select an item without buying/selling; explicit quantity
and Buy/Sell controls commit. Show price, stock, available personal gold, weight,
timers, rejection feedback and a clear buyback marker. Reuse existing prepared item
art, including the supply artwork for the existing `camp_kit` ration/rest supply.
Do not rename catalog items/fingerprints merely to label the shop's ration packs.

Hover uses the conversation presentation clock, not paused game time. A stable
occupied cell opens its tooltip at 350 ms; moving cells, paging, inventory/vendor
revision, selected character, focus loss or exit resets it. Clamp tooltip panels
to the world/trade region so portraits remain visible. Display item name,
description, weight, price and applicable damage/attack/training or armor AC.
For weapons compare main hand or ranged as appropriate; armor compares body;
shields compare off hand. Show the equipped item's corresponding information or
explicitly Empty. Use selected-character abilities/proficiencies and current
shield/versatile context; communicate equip restrictions instead of presenting
an unconditional upgrade. Descriptions wrap using bounded local buffers.

Initial innkeeper stock: ration supplies (`camp_kit`) 4 at 120 gold, healing
potions 3 at 50, mana potions 3 at 65, longsword 1 at 30 and chain mail 1 at 100.
Initial restock is 3 game days, buyback lasts 24 hours with a 25% markup. Provide
explicit sell prices for current transferable noncurrency items. These editable
starting values are tuning defaults, not proven balance. Potions move to the
provisioner and weapon/armor to the smithy when those NPCs exist.

## Verification

Owned small CPU fixtures cover parser/reference failures, finite stock, selected
character funds/ownership, quantities, multi-stack payment, full bags/weight,
atomic rejection, stale handles/revisions, buyback markup/capacity/exact expiry,
restock/time jumps, independent vendors, save roundtrip/legacy initialization and
warmed allocation guards. UI checks cover 349/350 ms dwell, both sides, equipped/
empty comparisons and character changes, focus/exit/page reset, uniform slots,
physical release isolation and conversation return. Run only affected Debug CPU
suites, one extended save integration checkpoint, and hidden application-owned
captures of buying, selling and weapon/armor comparisons. No OS input/focus control.
Keep at most six selected screenshots, record final results/limits here and clean
the owned artifact run. Interactive runtime acceptance remains user-owned.

## Implementation and verification — 2026-11-07

Implemented the authored shop catalog/validation, per-resident campaign ledger,
atomic bag/currency exchange, dialogue subview and native two-grid UI above.
Requests include the inventory owner's identity as well as both revisions, so
old purchases cannot replay into a different inventory owner after restore.
Prepared item artwork is reused; no new icon or rendering resource is needed.
`camp_kit` retains its catalog identity and is labelled Ration pack only in trade.
The source catalog and NPC fingerprints are unchanged. Current schema-12 saves
load with an empty ledger; new saves write schema 13/simulation 17.

One Debug configuration, at most two build workers and one test process, using
`nice -n 15 ionice -c 3`. About 21 GiB available RAM and 452 GiB workspace disk
were checked before work. Gameplay and `blobber_headless` are rebuilt; compiler
output has no warnings. The normal initial game build refreshed packaged assets.
After the final comparison-only metadata correction, the game was relinked using
its CMake-generated linker arguments and current verified libraries, reusing
prepared artwork rather than repeating asset preparation.

- `ctest --test-dir cmake-build-debug -R '^(trade|inventory|dialogue|hud|buttons|commands)$' --output-on-failure -j 1`:
  all six suites pass in 0.20 s. Owned trade fixtures cover purchase/sale/buyback,
  per-character funds and ownership, multiple gold stacks, atomic quantity/stock/
  bag/weight failures, exact expiry/restock and long time jumps, independent
  residents, bounded storage and malformed content. Existing inventory/dialogue/
  input/HUD callers retain their focused regression coverage.
- The final `ctest --test-dir cmake-build-debug -R '^trade$' --output-on-failure -j 1`
  passes in 0.01 s after adding owner-identity checks, metadata parity and boundary
  cases. Includes 349/350 ms dwell, merchant and player sides, current and empty
  equipment comparisons, training/shield context, portrait changes, focus/page/
  revision reset, physical release and context cancellation, square geometry,
  conversation return and C++ allocation guards for purchase/refresh/hover. The
  unrelated passing suites were not repeated after these trade-only corrections.
- `ctest --test-dir cmake-build-debug -R '^campaign_save$' --output-on-failure -j 1`:
  the extended persistence integration suite passes once in 100.54 s. It verifies
  canonical codec/restore with merchant stock, buyback prices and deadlines,
  schema-12 empty-ledger initialization without changing party/time/source bytes,
  corrupt merchant quantity rejection, and existing travel/floor/lifetime paths.
  No second Debug/Release/save/stress matrix was run.
- `blobber_review_trade cmake-build-debug/assets NEW_OUTPUT`: final hidden review
  passes on NVIDIA RTX 3060 / OpenGL 3.3. Six native 1920×1080 captures are
  [retained and visually inspected](screenshots/merchants/README.md). The runner
  grants isolated review inventory, buys a ration for 120 gold (1000→880), sells
  a dagger for 2 (880→882), verifies a 3-gold buyback, switches selected characters
  and returns to dialogue. Game time stays zero and pose stays fixed throughout;
  released world texture and alpha residency are zero. Final ability metadata
  does not change the captured pixels and has its focused CPU parity assertion.
- `git diff --check`, prompt JSON, selected PNG dimensions and the authored local
  Markdown links pass. No user save was read or changed, no desktop input/focus
  automation was used, and no benchmark or capacity stress workload was run.

The first hidden launch could not open X11 inside the sandbox; the authorized
hidden retry succeeded with local display access. Rendering jobs bounded
`OMP_NUM_THREADS=1`, `OPENBLAS_NUM_THREADS=1` and `LP_NUM_THREADS=2`. Interactive
feel, listening and economy balance remain user-owned review work; generated
fixture tests make no claim about graphics-driver or third-party C allocations.
Buyback is deliberately limited to 48 outstanding stacks per vendor and equipped
goods require unequipping before sale, as documented for authors/players.

Exactly six selected screenshots plus the accepted source mockup/prompt remain
as durable design/review inputs. The owned run's raw captures, logs and resolved
failure output are cleaned with the artifact helper's preview/apply workflow;
there is no retained diagnostic exception. Mockup approval is preserved and does
not imply runtime acceptance.

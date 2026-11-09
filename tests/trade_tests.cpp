#include "assets/shops.hpp"
#include "game/commands.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/inventory_fixtures.hpp"
#include "ui/button_targets.hpp"
#include "ui/interaction.hpp"
#include "ui/trade_layout.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace blobber;
namespace {
constexpr auto host = dialogueKey("fixture.host");
bool hasBuyback(const MerchantState& state) {
    return std::any_of(state.buyback.begin(), state.buyback.end(), [](const auto& entry) { return entry.quantity; });
}
rules::Catalog fixtureCatalog(bool atCapacity = false) {
    auto f = inventoryFixture();
    if (atCapacity)
        f.items[7].weight = 22900;
    rules::ItemDefinition gold;
    gold.id = "gold";
    gold.name = "Fixture gold";
    gold.description = "Test-owned currency";
    gold.kind = rules::ItemKind::Currency;
    gold.weight = 2;
    f.items.push_back(gold); // 12
    rules::ItemDefinition gear;
    gear.id = "fixture.gear";
    gear.name = "Fixture gear";
    gear.description = "Test-owned zero-weight fixture";
    f.items.push_back(gear); // 13
    return inventoryCatalog(f);
}
ShopCatalog shops() {
    ShopDefinition shop;
    shop.key = "fixture.shop";
    shop.id = dialogueKey(shop.key);
    shop.restockDays = 2;
    for (auto& p : shop.prices)
        p = {100, 10};
    shop.prices[10] = {50, 12};
    shop.stock = {{{10}, 3}, {{0}, 1}, {{7}, 4}};
    return {{shop}};
}
void acquire(Party& p, unsigned item, unsigned quantity, unsigned character = 0) {
    check(bool(p.acquire({{std::uint16_t(character)},
                          {std::uint16_t(item)},
                          std::uint16_t(quantity),
                          p.inventory().revision()})),
          "Fixture acquisition");
}
TradeRequest request(const Party& party, const MerchantLedger& ledger, const ShopCatalog& catalog,
                     unsigned offer = 0, unsigned quantity = 1, unsigned character = 0) {
    const auto* state = ledger.find(host);
    TradeRequest r;
    r.resident = host;
    r.shop = catalog.shops[0].id;
    r.merchantRevision = state->revision;
    r.inventoryRevision = party.inventory().revision();
    r.inventoryIdentity = party.inventory().identity();
    r.character = {std::uint16_t(character)};
    r.offer = offer;
    r.quantity = quantity;
    return r;
}
TradeRequest sale(const Party& party, const MerchantLedger& ledger, const ShopCatalog& catalog,
                  ItemHandle handle, unsigned quantity = 1) {
    auto r = request(party, ledger, catalog);
    r.side = TradeSide::Player;
    r.item = handle;
    r.quantity = quantity;
    return r;
}
unsigned quantity(const Party& p, unsigned item, unsigned character = 0) {
    unsigned out = 0;
    for (std::uint16_t i = 0; i < bagCells; ++i)
        if (const auto* s = p.inventory().stack(p.inventory().at({{std::uint16_t(character)}, i}));
            s && s->item.index == item)
            out += s->quantity;
    return out;
}
void purse() {
    const auto catalog = fixtureCatalog();
    auto preset = *catalog.preset();
    preset.characters[0].items[preset.characters[0].itemCount++] = {{12}, 65535};
    preset.characters[1].items[preset.characters[1].itemCount++] = {{12}, 1500};
    Party seeded(catalog, preset);
    check(seeded.gold() == 67035 && !findInventoryItem(seeded, 0, 12) && !findInventoryItem(seeded, 1, 12),
          "Starting currency is pooled without live stacks");
    check(rules::deriveCharacter(catalog, preset.characters[0]).value.carriedWeight ==
              seeded.character({0})->stats.carriedWeight,
          "Authored previews also exclude currency weight");
    const auto weight = seeded.character({0})->stats.carriedWeight;
    const AcquisitionRequest credit{{0}, {12}, 100, seeded.inventory().revision()};
    watching = true;
    allocations = 0;
    const auto preview = seeded.acquire(credit, true);
    const auto committed = seeded.acquire(credit);
    watching = false;
    check(preview && committed && allocations == 0 && seeded.gold() == 67135 &&
              seeded.character({0})->stats.carriedWeight == weight,
          "Currency preview/commit are allocation-free and weightless");
    check(seeded.acquire(credit).error == InventoryError::Stale && seeded.gold() == 67135,
          "Currency credit advances the transaction revision to reject replay");
    Party full(catalog);
    while (full.acquire({{0}, {13}, 1, full.inventory().revision()})) {
    }
    acquire(full, 12, 123);
    check(full.gold() == 123 && !findInventoryItem(full, 0, 12), "Full bags do not obstruct the purse");
    auto fullShop = shops();
    MerchantLedger fullLedger;
    fullLedger.visit(host, fullShop.shops[0], {});
    const auto partial = findInventoryItem(full, 0, 10);
    check(bool(trade(full, fullLedger, fullShop, sale(full, fullLedger, fullShop, partial), {})) &&
              full.gold() == 135 && full.inventory().stack(partial)->quantity == 1,
          "Partial sales into a full bag credit the purse without needing a proceeds slot");
    const auto heavyCatalog = fixtureCatalog(true);
    Party heavy(heavyCatalog);
    check(heavy.character({0})->stats.carriedWeight == heavy.character({0})->stats.carryingCapacity,
          "Currency boundary fixture starts at carrying capacity");
    acquire(heavy, 12, 65535);
    check(heavy.gold() == 65535 &&
              heavy.character({0})->stats.carriedWeight == heavy.character({0})->stats.carryingCapacity,
          "Currency never consumes carrying capacity");
    auto saved = seeded.checkpoint();
    saved.gold = UINT64_MAX;
    Party rich(catalog, saved);
    const auto revision = rich.inventory().revision();
    check(rich.acquire({{0}, {12}, 1, revision}, true).error == InventoryError::GoldOverflow &&
              rich.acquire({{0}, {12}, 1, revision}).error == InventoryError::GoldOverflow &&
              rich.gold() == UINT64_MAX && rich.inventory().revision() == revision,
          "Overflow preview and commit conserve the purse and revision");
    auto content = shops();
    MerchantLedger ledger;
    ledger.visit(host, content.shops[0], {});
    const auto sold = findInventoryItem(rich, 0, 10);
    check(trade(rich, ledger, content, sale(rich, ledger, content, sold), {}).error ==
                  TradeError::GoldOverflow &&
              rich.inventory().stack(sold)->quantity == 2 && ledger.find(host)->revision == 1 &&
              rich.gold() == UINT64_MAX,
          "Sale overflow rolls back goods, purse and merchant state");
    Party poor(catalog);
    check(trade(poor, ledger, content, request(poor, ledger, content), {}).error == TradeError::Funds &&
              poor.gold() == 0 && ledger.find(host)->stock[10] == 3,
          "Insufficient shared funds preserve merchant and party state");
    saved.inventory.stacks[207] = {{12}, 1, 1};
    saved.inventory.locations[0][47] = 207;
    rejects([&] { Party invalid(catalog, saved); }, "Live saves reject currency stacks");
}
void transactions() {
    auto catalog = fixtureCatalog();
    Party p(catalog);
    acquire(p, 12, 1500);
    auto content = shops();
    MerchantLedger ledger;
    ledger.visit(host, content.shops[0], {});
    const auto initialGold = p.gold(), initialRevision = p.inventory().revision();
    auto r = request(p, ledger, content, 0, 2);
    auto foreign = r;
    ++foreign.inventoryIdentity;
    check(trade(p, ledger, content, foreign, {}).error == TradeError::Stale,
          "A purchase cannot replay against a different inventory owner");
    const auto quote = trade(p, ledger, content, r, {}, true);
    check(bool(quote) && quote.total == 100 && p.gold() == initialGold &&
              p.inventory().revision() == initialRevision && ledger.find(host)->stock[10] == 3,
          "Preview conserves both owners");
    watching = true;
    allocations = 0;
    const auto committed = trade(p, ledger, content, r, {});
    watching = false;
    check(bool(committed) && allocations == 0, "Atomic purchase does not allocate C++ memory");
    check(quantity(p, 10) == 4 && p.gold() == 1400 && ledger.find(host)->stock[10] == 1,
          "Atomic multi-quantity purchase decrements finite stock and shared currency");
    check(trade(p, ledger, content, r, {}).error == TradeError::Stale, "Replayed revisions reject");
    r = request(p, ledger, content, 0, 2);
    const auto revision = p.inventory().revision();
    check(trade(p, ledger, content, r, {}).error == TradeError::Quantity &&
              p.inventory().revision() == revision && p.gold() == 1400,
          "Excess stock request conserves payment and inventory");
    r = request(p, ledger, content, 0, 1, 1);
    check(bool(trade(p, ledger, content, r, {})) && p.gold() == 1350 && quantity(p, 10, 1) == 3,
          "Another character spends shared funds and receives the purchase");
    check(trade(p, ledger, content, request(p, ledger, content), {}).error == TradeError::SoldOut,
          "Zero stock is not unlimited");
    auto sell = sale(p, ledger, content, findInventoryItem(p, 0, 10));
    const auto beforeSale = p.gold();
    check(bool(trade(p, ledger, content, sell, {})) && p.gold() == beforeSale + 12,
          "Sale credits the shared party purse");
    check(ledger.find(host)->buyback[0].price == 15 && ledger.find(host)->buyback[0].quantity == 1,
          "Sold goods appear in marked-up buyback stock");
    r = request(p, ledger, content, shopStockCapacity);
    check(bool(trade(p, ledger, content, r, {})) && p.gold() == beforeSale - 3 &&
              ledger.find(host)->buyback[0].quantity == 0,
          "Buyback restores the item at a markup and releases its slot");
    acquire(p, 0, 1);
    sell = sale(p, ledger, content, findInventoryItem(p, 0, 0));
    // findInventoryItem visits bag before equipment, so the unequipped sword is sold.
    check(bool(trade(p, ledger, content, sell, {})) && ledger.find(host)->buyback[0].price == 13,
          "Fractional buyback markup rounds upward");
    sell = sale(p, ledger, content, p.inventory().at({{0}, 49}));
    check(trade(p, ledger, content, sell, {}).error == TradeError::Stale,
          "Equipped items cannot be sold from a bag transaction");
    check(!findInventoryItem(p, 0, 12), "The purse never creates a sellable currency stack");
    sell = sale(p, ledger, content, findInventoryItem(p, 1, 10));
    check(trade(p, ledger, content, sell, {}).error == TradeError::Stale,
          "Foreign-character item handles reject");
    content.shops[0].prices[10].sell = 0;
    sell = sale(p, ledger, content, findInventoryItem(p, 0, 10));
    check(trade(p, ledger, content, sell, {}).error == TradeError::NotForSale,
          "Unpriced goods are rejected explicitly");
}
void capacityAndTime() {
    const auto catalog = fixtureCatalog();
    auto content = shops();
    Party p(catalog);
    acquire(p, 12, 1500);
    MerchantLedger ledger;
    ledger.visit(host, content.shops[0], {});
    const auto before = p.inventory().revision();
    check(trade(p, ledger, content, request(p, ledger, content, 2, 4), {}).error == TradeError::Overweight &&
              p.inventory().revision() == before && ledger.find(host)->stock[7] == 4,
          "Overweight rejection restores payment, inventory and merchant stock");
    while (p.acquire({{0}, {13}, 1, p.inventory().revision()})) {
    }
    const auto gold = p.gold();
    check(trade(p, ledger, content, request(p, ledger, content, 1), {}).error == TradeError::Full &&
              p.gold() == gold && ledger.find(host)->stock[0] == 1,
          "Full-bag purchase rejection is atomic");
    Party exact(catalog);
    acquire(exact, 12, 50);
    while (exact.acquire({{0}, {13}, 1, exact.inventory().revision()})) {
    }
    check(bool(trade(exact, ledger, content, request(exact, ledger, content), {})) && exact.gold() == 0,
          "Exact payment spends the purse while goods merge into an existing stack");
    Party seller(catalog);
    acquire(seller, 9, 100);
    for (unsigned i = 0; i < buybackCapacity; ++i) {
        const auto r = sale(seller, ledger, content, findInventoryItem(seller, 0, 9));
        check(bool(trade(seller, ledger, content, r, {})), "Bounded buyback storage accepts available slot");
    }
    auto r = sale(seller, ledger, content, findInventoryItem(seller, 0, 9));
    const auto sellerGold = seller.gold();
    check(trade(seller, ledger, content, r, {}).error == TradeError::BuybackFull &&
              seller.gold() == sellerGold,
          "Full buyback storage rejects without losing goods");
    auto* state = ledger.visit(host, content.shops[0], {gameDuration::day - 1});
    check(state->buyback[0].quantity == 1, "Buyback remains available within a trading session");
    state = ledger.visit(host, content.shops[0], {gameDuration::day});
    check(state->buyback[0].quantity == 1 && state->stock[10] == 2,
          "Legacy expiry metadata no longer removes session buyback or restocks ordinary goods");
    ledger.visit(host, content.shops[0], {2 * gameDuration::day - 1});
    check(state->stock[10] == 2, "Restock waits until deadline");
    ledger.visit(host, content.shops[0], {2 * gameDuration::day});
    check(state->stock[10] == 3 && state->restockMs == 4 * gameDuration::day,
          "Due restock refills to target, rather than adding unlimited stock");
    ledger.visit(host, content.shops[0], {100 * gameDuration::day});
    check(state->stock[10] == 3 && state->restockMs == 102 * gameDuration::day,
          "Large time jump computes the next interval without iterative deliveries");
    const auto* other = ledger.visit(dialogueKey("fixture.other"), content.shops[0], {});
    check(other != state && other->stock[10] == 3, "Shared shop template has independent per-resident stock");
    auto* independent = ledger.visit(other->resident, content.shops[0], {});
    independent->buyback[0] = {{10}, 1, 15, gameDuration::day};
    const auto stale = request(seller, ledger, content, shopStockCapacity);
    const auto retail = state->stock;
    const auto deadline = state->restockMs;
    const auto revision = state->revision;
    const auto goldBeforeClear = seller.gold();
    allocations = 0;
    watching = true;
    const bool cleared = ledger.clearBuyback(host);
    watching = false;
    check(cleared && allocations == 0 && !hasBuyback(*state) && state->revision == revision + 1 &&
              state->stock == retail && state->restockMs == deadline && seller.gold() == goldBeforeClear &&
              hasBuyback(*independent),
          "Session clear is allocation-free, vendor-local and preserves gold, retail stock and restock time");
    check(trade(seller, ledger, content, stale, {100 * gameDuration::day}).error == TradeError::Stale,
          "Clearing buyback invalidates an outstanding purchase request");
    check(!ledger.clearBuyback(host) && state->revision == revision + 1 &&
              !ledger.clearBuyback(dialogueKey("missing")),
          "Repeated and unknown-vendor clears are harmless");
    auto saved = ledger.checkpoint();
    MerchantLedger restored;
    restored.restore(saved, catalog);
    check(restored.find(host)->restockMs == state->restockMs, "Ledger restores its saved deadlines");
    saved[0].buyback[0] = {{10}, 21, 15, gameDuration::day};
    rejects([&] { restored.restore(saved, catalog); }, "Oversized buyback stacks reject on restore");
    saved = ledger.checkpoint();
    saved.push_back(saved[0]);
    rejects([&] { restored.restore(saved, catalog); }, "Duplicate saved resident ownership rejects");
    saved = ledger.checkpoint();
    saved[0].buyback[0] = {{10}, 1, 15, UINT64_MAX};
    rejects([&] { restored.restore(saved, catalog); }, "Unbounded saved buyback deadline rejects");
    for (unsigned i = 2; i < merchantCapacity; ++i)
        check(ledger.visit(dialogueKey("fixture.capacity") + i, content.shops[0], {}) != nullptr,
              "Available visited-vendor slot accepts a new resident");
    check(!ledger.visit(dialogueKey("fixture.excess"), content.shops[0], {}) &&
              ledger.checkpoint().size() == merchantCapacity,
          "Visited-vendor capacity rejects without unbounded storage growth");
    state->buyback[0] = {{10}, 1, 15, gameDuration::day};
    state->revision = UINT64_MAX - 1;
    check(ledger.clearBuyback(host) && !hasBuyback(*state) && state->revision == UINT64_MAX - 1 &&
              trade(seller, ledger, content, request(seller, ledger, content), {}).error == TradeError::Stale,
          "Exhausted revision still clears goods, never wraps and cannot resume trading");
}
void comparisons() {
    const auto catalog = fixtureCatalog();
    Party p(catalog);
    auto c = compareItem(p, {0}, *catalog.item({0}));
    check(c.equippable && c.equipped == p.equipped({0}, EquipmentSlot::MainHand) &&
              c.offeredWeapon.damage.sides == 8 && c.equippedWeapon.damage.sides == 8,
          "Melee comparison uses actual selected character main hand and shield context");
    c = compareItem(p, {2}, *catalog.item({3}));
    check(c.equipped == p.equipped({2}, EquipmentSlot::Ranged),
          "Ranged comparison uses alternate ranged slot");
    auto restricted = compareItem(p, {0}, *catalog.item({3}));
    check(restricted.restriction[0] && restricted.offeredWeapon.available &&
              restricted.offeredWeapon.damage.sides == 6 && restricted.offeredWeapon.attackBonus == 4,
          "Shield conflict retains a real hypothetical bow preview with restriction");
    check(c.offeredWeapon.ability == rules::Ability::Dexterity,
          "Ranged comparison records its derived attack ability");
    c = compareItem(p, {3}, *catalog.item({7}));
    check(c.equippable && !c.equipped && c.restriction[0] && c.offeredArmorClass == 16,
          "Empty armor slot and missing training remain explicit");
    c = compareItem(p, {0}, *catalog.item({7}));
    check(c.equipped && c.offeredArmorClass == p.character({0})->stats.armorClass,
          "Armor comparison includes shield and Defense with actual current equipment");
    check(!compareItem(p, {0}, *catalog.item({10})).equippable,
          "Consumables have no invented equipment comparison");
}
using Json = nlohmann::json;
struct Directory {
    std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("blobber-trade-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Directory() { std::filesystem::create_directory(path); }
    ~Directory() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
    void write(const Json& source) { std::ofstream(path / "shop.yaml") << source.dump(); }
};
void parser() {
    const auto catalog = fixtureCatalog();
    Directory directory;
    Json source = {{"version", 1},
                   {"id", "fixture.shop"},
                   {"restock_days", 2},
                   {"buyback_hours", 24},
                   {"buyback_markup_percent", 25},
                   {"prices", {{"fixture.healing", {{"buy", 50}, {"sell", 12}}}}},
                   {"stock", Json::array({{{"item", "fixture.healing"}, {"quantity", 3}}})}};
    directory.write(source);
    check(loadShops(directory.path, catalog).shops[0].stock[0].quantity == 3,
          "Independent shop YAML resolves catalog item IDs and finite stock");
    for (unsigned fault = 0; fault < 10; ++fault) {
        auto bad = source;
        if (fault == 0)
            bad["stock"][0]["item"] = "missing";
        if (fault == 1)
            bad["restock_days"] = 0;
        if (fault == 2)
            bad["stock"][0]["quantity"] = 0;
        if (fault == 3)
            bad["prices"]["fixture.healing"]["sell"] = 50;
        if (fault == 4)
            bad["buyback_hours"] = 25;
        if (fault == 5)
            bad["stock"].push_back(bad["stock"][0]);
        if (fault == 6)
            bad["prices"]["gold"] = {{"buy", 2}, {"sell", 1}};
        if (fault == 7)
            bad["restock_days"] = 1.5;
        if (fault == 8)
            bad["restok_days"] = 2;
        if (fault == 9)
            bad["buyback_markup_percent"] = 0;
        directory.write(bad);
        rejects([&] { loadShops(directory.path, catalog); }, "Malformed shop data rejects");
    }
    std::ofstream(directory.path / "shop.yaml") << "version: 1\nversion: 1\n";
    rejects([&] { loadShops(directory.path, catalog); }, "Duplicate YAML field rejects");
    DialogueCatalog dialogue;
    dialogue.characters.emplace_back();
    dialogue.characters[0].shop = dialogueKey("missing");
    rejects([&] { validateShops(dialogue, shops()); },
            "Unresolved dialogue shop references fail at preparation");
}
float measure(const char* text, void*) {
    return float(std::char_traits<char>::length(text) * 4);
}
void gestures(CommandController& controls, GameSession& game) {
    auto point = [](TradeSide side, unsigned cell) {
        const auto r = tradeLayout::cell(side, cell);
        return ArtPoint{float(r.x + 5), float(r.y + 5)};
    };
    const auto physical = [&](ArtPoint p, bool press, bool held, bool release, bool focused = true,
                              bool accept = true) {
        UiInput in;
        in.pointerTracked = true;
        in.pointer = p;
        in.pointerHeld = held;
        in.pointerReleased = release;
        in.focused = focused;
        in.pressed[unsigned(UiKey::Click)] = press;
        // Match production's pre-routing and routeHudInput's second, idempotent pass.
        in = routeHudButtons(controls, in, accept);
        routeHudInput(controls, in, {}, accept);
    };
    const auto click = [&](TradeSide side, unsigned cell) {
        const auto p = point(side, cell);
        physical(p, true, true, false);
        check(controls.snapshot().trade.gesture.down, "Grid press captures a potential drag");
        physical(p, false, false, true);
    };
    const auto key = [&](UiKey k) {
        UiInput in;
        in.pressed[unsigned(k)] = true;
        routeHudInput(controls, in);
    };
    const auto cellOf = [&](unsigned item, bool buyback = false) {
        const auto& cells = buyback ? controls.snapshot().trade.merchant : controls.snapshot().trade.player;
        for (unsigned i = 0; i < 48; ++i)
            if (cells[i].definition &&
                cells[i].definition->id == game.party().catalog().item({std::uint16_t(item)})->id &&
                (!buyback || cells[i].buyback))
                return i;
        throw std::runtime_error("Gesture fixture item not found");
    };
    const auto drag = [&](TradeSide side, unsigned cell, ArtPoint destination) {
        physical(point(side, cell), true, true, false);
        const auto revision = game.party().inventory().revision();
        physical(destination, false, true, false);
        check(controls.snapshot().trade.gesture.dragging && game.party().inventory().revision() == revision,
              "Dragging does not move goods or payment before release");
        physical(destination, false, false, true);
    };
    auto gold = game.party().gold();
    click(TradeSide::Merchant, 0);
    check(game.party().gold() == gold, "First completed grid click only selects");
    click(TradeSide::Merchant, 0);
    check(game.party().gold() == gold - 50, "Physical double-click buys one item");
    // The last stocked potion is now gone; a third click cannot purchase a successor.
    UiInput logical;
    logical.pointer = point(TradeSide::Merchant, 0);
    logical.pressed[unsigned(UiKey::Click)] = true;
    routeHudInput(controls, logical);
    check(game.party().gold() == gold - 50, "A third click never repeats the completed trade");
    click(TradeSide::Merchant, 1);
    controls.tickDialogue(.401, true);
    click(TradeSide::Merchant, 1);
    check(game.party().gold() == gold - 50, "Clicks outside the double-click interval only select");
    key(UiKey::Back);
    check(controls.snapshot().trade.open, "B/Back has no binding in trade");
    gold = game.party().gold();
    drag(TradeSide::Merchant, 1, point(TradeSide::Player, 0));
    check(game.party().gold() == gold - 100 && quantity(game.party(), 0) == 1,
          "Dropping merchant stock on an occupied player cell buys through normal placement");
    auto sword = cellOf(0);
    gold = game.party().gold();
    click(TradeSide::Player, sword);
    click(TradeSide::Player, sword);
    check(game.party().gold() == gold + 10 && quantity(game.party(), 0) == 0,
          "Double-clicking a bag item sells it");
    auto buyback = cellOf(0, true);
    gold = game.party().gold();
    drag(TradeSide::Merchant, buyback, point(TradeSide::Player, 47));
    check(game.party().gold() == gold - 13 && quantity(game.party(), 0) == 1,
          "Buyback uses normal drag purchase at its stored markup");
    sword = cellOf(0);
    gold = game.party().gold();
    drag(TradeSide::Player, sword, point(TradeSide::Merchant, 0));
    check(game.party().gold() == gold + 10 && quantity(game.party(), 0) == 0,
          "Dropping bag goods on occupied merchant stock sells them");
    buyback = cellOf(0, true);
    click(TradeSide::Merchant, buyback);
    click(TradeSide::Merchant, buyback);
    check(game.party().gold() == gold - 3 && quantity(game.party(), 0) == 1,
          "Buyback also uses normal double-click purchase");
    const auto potion = cellOf(10);
    click(TradeSide::Player, potion);
    key(UiKey::Right);
    check(controls.snapshot().trade.quantity == 2, "Selected quick-trade quantity can be adjusted");
    const auto potions = quantity(game.party(), 10);
    click(TradeSide::Player, potion);
    click(TradeSide::Player, potion);
    check(quantity(game.party(), 10) == potions - 2 && controls.snapshot().trade.selected == potion &&
              controls.snapshot().trade.quantity == 1,
          "Double-click honors chosen quantity and retains a partially sold stack at quantity one");
    gold = game.party().gold();
    key(UiKey::Confirm);
    check(game.party().gold() == gold + 12 && controls.snapshot().trade.selected == potion,
          "Repeated Sell can continue a remaining stack");
    acquire(game.party(), 10, 4);
    controls.refresh({}, false, false, true);
    click(TradeSide::Player, potion);
    key(UiKey::Right);
    const auto beforeDrag = quantity(game.party(), 10);
    gold = game.party().gold();
    drag(TradeSide::Player, potion, point(TradeSide::Merchant, 0));
    check(quantity(game.party(), 10) == beforeDrag - 2 && game.party().gold() == gold + 24 &&
              controls.snapshot().trade.quantity == 1,
          "Drag captures the chosen quantity and commits it atomically");
    const ArtPoint sell{float(tradeLayout::accept.x + 4), float(tradeLayout::accept.y + 4)};
    acquire(game.party(), 12, unsigned(1000 - game.party().gold()));
    acquire(game.party(), 13, 5);
    controls.refresh({}, false, false, true);
    const auto first = cellOf(13);
    click(TradeSide::Player, first);
    for (unsigned i = 0; i < 5; ++i) {
        check(controls.snapshot().trade.selected == first + i,
              "Next adjacent sellable item is auto-selected");
        gold = game.party().gold();
        physical(sell, true, true, false);
        physical(sell, false, true, false);
        check(game.party().gold() == gold, "Held Sell button never repeats");
        physical(sell, false, false, true);
        check(game.party().gold() == gold + 10, "Each fresh Sell click sells one successive item");
        physical(sell, false, false, true);
        check(game.party().gold() == gold + 10, "Replayed release cannot sell the successor");
    }
    check(quantity(game.party(), 13) == 0 && controls.snapshot().trade.selected == 48 &&
              game.party().gold() == 1050 && quantity(game.party(), 12) == 0,
          "Sequential sales credit the purse without creating stacks or wrapping to earlier items");
    gold = game.party().gold();
    drag(TradeSide::Merchant, 2, point(TradeSide::Merchant, 0));
    drag(TradeSide::Merchant, 2, {505, 340});
    check(game.party().gold() == gold, "Own-side and outside drops cancel without a trade");
    physical(point(TradeSide::Merchant, 2), true, true, false);
    physical(point(TradeSide::Player, 0), false, true, false, false);
    physical(point(TradeSide::Player, 0), false, false, true);
    check(game.party().gold() == gold && !controls.snapshot().trade.gesture.down,
          "Focus loss cancels an in-progress drag");
    physical(point(TradeSide::Merchant, 2), true, true, false);
    key(UiKey::Two);
    physical(point(TradeSide::Player, 0), false, false, true);
    check(game.party().gold() == gold, "Changing character cancels the captured source before drop");
    key(UiKey::One);
    physical(point(TradeSide::Merchant, 2), true, true, false);
    acquire(game.party(), 13, 1);
    controls.refresh({}, false, false, true);
    physical(point(TradeSide::Player, 0), false, false, true);
    check(game.party().gold() == gold, "Inventory revision changes cancel captured drops");
    physical(point(TradeSide::Merchant, 2), true, true, false);
    physical(point(TradeSide::Player, 0), false, false, true, true, false);
    check(game.party().gold() == gold && !controls.snapshot().trade.gesture.down,
          "Rejected physical input cannot commit an earlier drag on release");
    physical(point(TradeSide::Merchant, 2), true, true, false);
    UiInput pointerLost;
    pointerLost.pointerTracked = true;
    pointerLost.pointerHeld = true;
    routeHudInput(controls, pointerLost);
    physical(point(TradeSide::Player, 0), false, false, true);
    check(game.party().gold() == gold && !controls.snapshot().trade.gesture.down,
          "Losing the mapped pointer cancels a captured drop");
    click(TradeSide::Merchant, 2);
    UiInput blur;
    blur.focused = false;
    routeHudInput(controls, blur);
    click(TradeSide::Merchant, 2);
    check(game.party().gold() == gold,
          "Focus loss resets double-click history instead of completing a purchase after refocus");
}
void ui() {
    const auto catalog = fixtureCatalog();
    const Map map(
        {"#########", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#", "#########"});
    const Player pose{{4, 5}, Heading::North};
    NpcInstance npc{{0}, {4, 3}, {1, 1}, Heading::South};
    npc.role = NpcRole::Resident;
    npc.residentId = host;
    const NpcWorld world(map, pose.cell, {"goblin"}, {npc});
    GameSession game(map, catalog, world, 5, pose, false);
    acquire(game.party(), 12, 500);
    DialogueCatalog dialogue;
    DialogueCharacter character;
    character.id = host;
    character.shop = shops().shops[0].id;
    character.name = "Fixture merchant";
    DialogueParticipant participant;
    participant.id = host;
    participant.name = "Fixture merchant";
    character.participants.push_back(participant);
    DialogueBeat beat;
    beat.text = "Test greeting";
    character.greeting.beats.push_back(beat);
    character.introduction.beats.push_back(beat);
    DialogueTopic topic;
    topic.id = dialogueKey("trade");
    topic.label = "Trade";
    topic.trade = true;
    character.topics.push_back(topic);
    dialogue.characters.push_back(character);
    prepareDialogueLayout(dialogue, measure, nullptr);
    auto content = shops();
    MerchantLedger ledger;
    auto* legacy = ledger.visit(host, content.shops[0], {});
    legacy->buyback[0] = {{10}, 1, 15, gameDuration::day};
    ledger.restore(ledger.checkpoint(), catalog);
    DialogueHistory history;
    CommandController controls(game);
    controls.bindDialogue(&dialogue, &history);
    controls.bindTrade(&content, &ledger);
    TargetSnapshot residents;
    residents.world = game.actors().snapshot().world;
    residents.revision = game.actors().snapshot().revision;
    residents.ids[0] = game.actors().id(0);
    residents.count = 1;
    controls.residentVisibility(residents);
    controls.refresh({}, false, false, true);
    check(controls.startDialogue(), "Fixture resident admits dialogue");
    controls.tickDialogue(.7, true);
    UiInput input;
    input.pressed[unsigned(UiKey::One)] = true;
    routeHudInput(controls, input);
    check(controls.snapshot().trade.open && controls.snapshot().dialogue.open &&
              !controls.snapshot().modal(false) && !controls.dialoguePlayer().beat(),
          "Trade topic opens a paused subview and clears speech cues without pausing ambience");
    check(!hasBuyback(*ledger.find(host)) && ledger.find(host)->stock[10] == 3,
          "Opening trade discards loaded legacy buyback while preserving ordinary stock");
    const auto clock = game.gameTime().elapsedMs;
    game.update(1, false);
    check(game.gameTime().elapsedMs == clock && game.saveSafety() && game.travelSafety(),
          "Trade keeps gameplay paused and save/travel blocked");
    auto hover = [&](TradeSide side, unsigned cell) {
        UiInput in;
        const auto rect = tradeLayout::cell(side, cell);
        in.pointer = ArtPoint{float(rect.x + 4), float(rect.y + 4)};
        routeHudInput(controls, in);
    };
    hover(TradeSide::Merchant, 1);
    controls.tickDialogue(.349, true);
    check(!controls.snapshot().trade.tooltipVisible, "Tooltip hidden at 349 ms");
    controls.tickDialogue(.001, true);
    check(controls.snapshot().trade.tooltipVisible && controls.snapshot().trade.comparison.equipped ==
                                                          game.party().equipped({0}, EquipmentSlot::MainHand),
          "Tooltip opens at 350 ms with selected member's current weapon");
    UiInput other;
    other.pressed[unsigned(UiKey::Four)] = true;
    routeHudInput(controls, other);
    check(!controls.snapshot().trade.tooltipVisible && controls.snapshot().trade.character.index == 3,
          "Portrait hotkey changes trader and resets tooltip");
    hover(TradeSide::Merchant, 2);
    controls.tickDialogue(.35, true);
    check(controls.snapshot().trade.tooltipVisible && !controls.snapshot().trade.comparison.equipped &&
              controls.snapshot().trade.comparison.restriction[0],
          "Armor tooltip shows empty equipment and missing training for the new character");
    hover(TradeSide::Player, 0);
    controls.tickDialogue(.35, true);
    check(controls.snapshot().trade.tooltipVisible &&
              controls.snapshot().trade.hoverSide == TradeSide::Player,
          "Player bag items use the same delayed tooltip");
    UiInput blur;
    blur.focused = false;
    routeHudInput(controls, blur);
    check(!controls.snapshot().trade.tooltipVisible, "Focus loss clears tooltip immediately");
    controls.tickDialogue(1, false);
    hover(TradeSide::Player, 0);
    check(!controls.snapshot().trade.tooltipVisible, "Regained focus starts a fresh dwell");
    for (const auto side : {TradeSide::Merchant, TradeSide::Player})
        for (unsigned i = 0; i < 48; ++i) {
            const auto rect = tradeLayout::cell(side, i);
            check(rect.width == 26 && rect.height == 26,
                  "Every inventory slot has identical square geometry");
        }
    check(!hudButtonTargets(controls.snapshot()).overflow, "Both grids fit bounded button storage");
    other = {};
    other.pressed[unsigned(UiKey::One)] = true;
    routeHudInput(controls, other);
    auto rect = tradeLayout::cell(TradeSide::Merchant, 0);
    UiInput click;
    click.pointer = ArtPoint{float(rect.x + 4), float(rect.y + 4)};
    click.pressed[unsigned(UiKey::Click)] = true;
    routeHudInput(controls, click);
    check(controls.snapshot().trade.selected == 0 && game.party().gold() == 500,
          "Selecting a cell never commits a purchase");
    UiInput confirm;
    confirm.pressed[unsigned(UiKey::Confirm)] = true;
    routeHudInput(controls, confirm);
    check(game.party().gold() == 450 && !controls.snapshot().trade.tooltipVisible,
          "Explicit confirmation commits a purchase and resets revision-bound hover");
    routeHudInput(controls, click);
    UiInput press;
    press.pointerTracked = true;
    press.pointerHeld = true;
    press.pointer = ArtPoint{float(tradeLayout::accept.x + 3), float(tradeLayout::accept.y + 3)};
    press.pressed[unsigned(UiKey::Click)] = true;
    routeHudInput(controls, press);
    check(game.party().gold() == 450, "Physical buy press waits for release");
    UiInput release;
    release.pointerTracked = true;
    release.pointerReleased = true;
    release.pointer = press.pointer;
    routeHudInput(controls, release);
    check(game.party().gold() == 400, "Matching physical release commits exactly one purchase");
    routeHudInput(controls, click);
    routeHudInput(controls, press);
    other = {};
    other.pressed[unsigned(UiKey::Two)] = true;
    routeHudInput(controls, other);
    routeHudInput(controls, release);
    check(game.party().gold() == 400, "Character change cancels an armed purchase before its release");
    other = {};
    other.pressed[unsigned(UiKey::One)] = true;
    routeHudInput(controls, other);
    watching = true;
    allocations = 0;
    for (unsigned i = 0; i < 10; ++i) {
        hover(TradeSide::Merchant, 1);
        controls.tickDialogue(.05, true);
        controls.refresh({}, false, false, true);
    }
    watching = false;
    check(allocations == 0, "Warmed trade refresh, hover and equipment comparison allocate no C++ memory");
    gestures(controls, game);
    auto* state = ledger.visit(host, content.shops[0], {});
    for (unsigned i = 0; i < state->buyback.size(); ++i)
        state->buyback[i] = {{10}, 1, 15, gameDuration::day};
    ++state->revision;
    controls.refresh({}, false, false, true);
    UiInput selectMana;
    for (unsigned i = 0; i < 48; ++i)
        if (controls.snapshot().trade.player[i].definition &&
            controls.snapshot().trade.player[i].definition->consumable == rules::Consumable::Mana) {
            const auto rect = tradeLayout::cell(TradeSide::Player, i);
            selectMana.pointer = ArtPoint{float(rect.x + 4), float(rect.y + 4)};
        }
    selectMana.pressed[unsigned(UiKey::Click)] = true;
    routeHudInput(controls, selectMana);
    UiInput more;
    more.pressed[unsigned(UiKey::Right)] = true;
    routeHudInput(controls, more);
    const auto selected = controls.snapshot().trade.selected;
    const auto oldRevision = game.party().inventory().revision();
    UiInput sell;
    sell.pressed[unsigned(UiKey::Confirm)] = true;
    routeHudInput(controls, sell);
    check(controls.snapshot().trade.selected == selected && controls.snapshot().trade.quantity == 2 &&
              game.party().inventory().revision() == oldRevision,
          "Rejected sale preserves selection and quantity instead of advancing");
    UiInput gridPress;
    gridPress.pointerTracked = true;
    gridPress.pointerHeld = true;
    gridPress.pressed[unsigned(UiKey::Click)] = true;
    const auto armorCell = tradeLayout::cell(TradeSide::Merchant, 2);
    gridPress.pointer = ArtPoint{float(armorCell.x + 4), float(armorCell.y + 4)};
    routeHudInput(controls, gridPress);
    UiInput turnPage;
    turnPage.pressed[unsigned(UiKey::NextPage)] = true;
    routeHudInput(controls, turnPage);
    gridPress.pressed.fill(false);
    gridPress.pointerHeld = false;
    gridPress.pointerReleased = true;
    gridPress.pointer = ArtPoint{270, 140};
    routeHudInput(controls, gridPress);
    check(game.party().inventory().revision() == oldRevision && !controls.snapshot().trade.gesture.down,
          "Page changes cancel a source capture before its release");
    turnPage.pressed.fill(false);
    turnPage.pressed[unsigned(UiKey::PreviousPage)] = true;
    routeHudInput(controls, turnPage);
    hover(TradeSide::Merchant, 1);
    controls.tickDialogue(.35, true);
    check(controls.snapshot().trade.tooltipVisible && controls.snapshot().trade.pages == 2,
          "Bounded combined stock and buyback use merchant pages");
    UiInput page;
    page.pressed[unsigned(UiKey::NextPage)] = true;
    routeHudInput(controls, page);
    check(controls.snapshot().trade.page == 1 && !controls.snapshot().trade.tooltipVisible,
          "Changing merchant page resets the hover identity and dwell");
    UiInput close;
    close.pointerTracked = true;
    close.pointerHeld = true;
    close.pointer = ArtPoint{float(tradeLayout::close.x + 3), float(tradeLayout::close.y + 3)};
    close.pressed[unsigned(UiKey::Click)] = true;
    routeHudInput(controls, close);
    check(controls.snapshot().trade.open, "Close X waits for physical release");
    close.pressed.fill(false);
    close.pointerHeld = false;
    close.pointerReleased = true;
    const auto stockBeforeClose = ledger.find(host)->stock;
    const auto goldBeforeClose = game.party().gold();
    routeHudInput(controls, close);
    check(!controls.snapshot().trade.open && controls.snapshot().dialogue.open,
          "Close X returns to dialogue without click-through");
    check(!hasBuyback(*ledger.find(host)) && ledger.find(host)->stock == stockBeforeClose &&
              game.party().gold() == goldBeforeClose,
          "Closing X discards sold goods while preserving purchased retail stock and sale proceeds");
    check(controls.openTrade(), "Trade can reopen after mouse close");
    UiInput back;
    back.pressed[unsigned(UiKey::Cancel)] = true;
    routeHudInput(controls, back);
    check(!controls.snapshot().trade.open && controls.snapshot().dialogue.open && game.inConversation(),
          "Escape returns to conversation without leaking input or ending it");
    controls.finishDialogue();
    controls.openInventory();
    check(controls.snapshot().inventory.gold == game.party().gold(),
          "Inventory and trade expose the same party purse");
    controls.inventoryCharacter({1});
    check(controls.snapshot().inventory.gold == game.party().gold(),
          "Inventory character selection preserves the shared balance");
    controls.closeOverlay();
    check(!controls.snapshot().trade.open && !game.inConversation(),
          "Conversation teardown closes trade safely");

    const auto reopen = [&](CommandController& controller) {
        if (!game.inConversation()) {
            residents.revision = game.actors().snapshot().revision;
            controller.residentVisibility(residents);
            controller.refresh({}, false, false, true);
            check(controller.startDialogue(), "Reenter conversation for buyback lifetime check");
            controller.tickDialogue(.7, true);
        }
        check(controller.openTrade(), "Open another isolated trading session");
        check(!hasBuyback(*ledger.find(host)), "Reopening never resurrects buyback from a closed session");
    };
    const auto sellOne = [&](CommandController& controller) {
        acquire(game.party(), 13, 1);
        const auto item = findInventoryItem(game.party(), 0, 13);
        check(bool(trade(game.party(), ledger, content, sale(game.party(), ledger, content, item), {})),
              "Sell an owned item for session lifetime regression");
        controller.refresh({}, false, false, true);
        check(hasBuyback(*ledger.find(host)), "Fresh sale is available for same-session buyback");
    };
    for (unsigned exit = 0; exit < 6; ++exit) {
        reopen(controls);
        sellOne(controls);
        check(controls.openTrade() && hasBuyback(*ledger.find(host)),
              "Reentrant open preserves the current trading session");
        UiInput selectOther;
        selectOther.pressed[unsigned(UiKey::Two)] = true;
        routeHudInput(controls, selectOther);
        check(hasBuyback(*ledger.find(host)), "Character changes preserve this session's sold goods");
        const auto gold = game.party().gold();
        const auto stock = ledger.find(host)->stock;
        allocations = 0;
        watching = true;
        if (exit == 0)
            routeHudInput(controls, back); // Escape.
        else if (exit == 1) {
            auto mouseBack = close;
            mouseBack.pointer = ArtPoint{float(tradeLayout::back.x + 3), float(tradeLayout::back.y + 3)};
            mouseBack.pointerHeld = true;
            mouseBack.pointerReleased = false;
            mouseBack.pressed[unsigned(UiKey::Click)] = true;
            routeHudInput(controls, mouseBack);
            mouseBack.pressed.fill(false);
            mouseBack.pointerHeld = false;
            mouseBack.pointerReleased = true;
            routeHudInput(controls, mouseBack);
        } else if (exit == 2)
            controls.closeOverlay();
        else if (exit == 3)
            controls.finishDialogue();
        else if (exit == 4)
            controls.bindTrade(&content, &ledger);
        else
            controls.bindDialogue(&dialogue, &history);
        watching = false;
        check(allocations == 0 && !controls.snapshot().trade.open && !hasBuyback(*ledger.find(host)) &&
                  ledger.find(host)->stock == stock && game.party().gold() == gold,
              "Every trade exit clears buyback without allocation or changing gold/retail stock");
        controls.finishDialogue();
    }
    {
        CommandController transient(game);
        transient.bindDialogue(&dialogue, &history);
        transient.bindTrade(&content, &ledger);
        reopen(transient);
        sellOne(transient);
    }
    check(!hasBuyback(*ledger.find(host)) && !game.inConversation(),
          "Controller destruction closes the conversation and discards session buyback");
}
} // namespace
int main() try {
    parser();
    purse();
    transactions();
    capacityAndTime();
    comparisons();
    ui();
    std::cout << "trade tests passed\n";
    return 0;
} catch (const std::exception& error) {
    watching = false;
    std::cerr << error.what() << '\n';
    return 1;
}

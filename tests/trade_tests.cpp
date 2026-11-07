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
rules::Catalog fixtureCatalog() {
    auto f = inventoryFixture();
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
void transactions() {
    auto catalog = fixtureCatalog();
    Party p(catalog);
    acquire(p, 12, 1500);
    auto content = shops();
    MerchantLedger ledger;
    ledger.visit(host, content.shops[0], {});
    const auto initialGold = characterGold(p, {0}), initialRevision = p.inventory().revision();
    auto r = request(p, ledger, content, 0, 2);
    auto foreign = r;
    ++foreign.inventoryIdentity;
    check(trade(p, ledger, content, foreign, {}).error == TradeError::Stale,
          "A purchase cannot replay against a different inventory owner");
    const auto quote = trade(p, ledger, content, r, {}, true);
    check(bool(quote) && quote.total == 100 && characterGold(p, {0}) == initialGold &&
              p.inventory().revision() == initialRevision && ledger.find(host)->stock[10] == 3,
          "Preview conserves both owners");
    watching = true;
    allocations = 0;
    const auto committed = trade(p, ledger, content, r, {});
    watching = false;
    check(bool(committed) && allocations == 0, "Atomic purchase does not allocate C++ memory");
    check(quantity(p, 10) == 4 && characterGold(p, {0}) == 1400 && ledger.find(host)->stock[10] == 1,
          "Atomic multi-quantity purchase decrements finite stock and personal currency");
    check(trade(p, ledger, content, r, {}).error == TradeError::Stale, "Replayed revisions reject");
    r = request(p, ledger, content, 0, 2);
    const auto revision = p.inventory().revision();
    check(trade(p, ledger, content, r, {}).error == TradeError::Quantity &&
              p.inventory().revision() == revision && characterGold(p, {0}) == 1400,
          "Excess stock request conserves payment and inventory");
    r = request(p, ledger, content, 0, 1, 1);
    check(trade(p, ledger, content, r, {}).error == TradeError::Funds && characterGold(p, {0}) == 1400,
          "Purchasing never borrows another member's gold");
    r = request(p, ledger, content, 0);
    check(bool(trade(p, ledger, content, r, {})), "Last stocked item can be purchased");
    check(trade(p, ledger, content, request(p, ledger, content), {}).error == TradeError::SoldOut,
          "Zero stock is not unlimited");
    auto sell = sale(p, ledger, content, findInventoryItem(p, 0, 10));
    const auto beforeSale = characterGold(p, {0});
    check(bool(trade(p, ledger, content, sell, {})) && characterGold(p, {0}) == beforeSale + 12,
          "Sale pays exactly the selected character");
    check(ledger.find(host)->buyback[0].price == 15 && ledger.find(host)->buyback[0].quantity == 1,
          "Sold goods appear in marked-up buyback stock");
    r = request(p, ledger, content, shopStockCapacity);
    check(bool(trade(p, ledger, content, r, {})) && characterGold(p, {0}) == beforeSale - 3 &&
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
    sell = sale(p, ledger, content, findInventoryItem(p, 0, 12));
    check(trade(p, ledger, content, sell, {}).error == TradeError::NotForSale,
          "Currency cannot be sold as merchandise");
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
    const auto gold = characterGold(p, {0});
    check(trade(p, ledger, content, request(p, ledger, content, 1), {}).error == TradeError::Full &&
              characterGold(p, {0}) == gold && ledger.find(host)->stock[0] == 1,
          "Full-bag purchase rejection is atomic");
    Party exact(catalog);
    acquire(exact, 12, 50);
    while (exact.acquire({{0}, {13}, 1, exact.inventory().revision()})) {
    }
    check(bool(trade(exact, ledger, content, request(exact, ledger, content), {})) &&
              characterGold(exact, {0}) == 0,
          "Payment can free its last currency slot before goods are inserted");
    Party seller(catalog);
    acquire(seller, 9, 100);
    for (unsigned i = 0; i < buybackCapacity; ++i) {
        const auto r = sale(seller, ledger, content, findInventoryItem(seller, 0, 9));
        check(bool(trade(seller, ledger, content, r, {})), "Bounded buyback storage accepts available slot");
    }
    auto r = sale(seller, ledger, content, findInventoryItem(seller, 0, 9));
    const auto sellerGold = characterGold(seller, {0});
    check(trade(seller, ledger, content, r, {}).error == TradeError::BuybackFull &&
              characterGold(seller, {0}) == sellerGold,
          "Full buyback storage rejects without losing goods");
    auto* state = ledger.visit(host, content.shops[0], {gameDuration::day - 1});
    check(state->buyback[0].quantity == 1, "Buyback remains until exact expiry");
    state = ledger.visit(host, content.shops[0], {gameDuration::day});
    check(state->buyback[0].quantity == 0 && state->stock[10] == 2,
          "Expiry clears sold items without prematurely restocking ordinary stock");
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
    check(controls.snapshot().trade.selected == 0 && characterGold(game.party(), {0}) == 500,
          "Selecting a cell never commits a purchase");
    UiInput confirm;
    confirm.pressed[unsigned(UiKey::Confirm)] = true;
    routeHudInput(controls, confirm);
    check(characterGold(game.party(), {0}) == 450 && !controls.snapshot().trade.tooltipVisible,
          "Explicit confirmation commits a purchase and resets revision-bound hover");
    routeHudInput(controls, click);
    UiInput press;
    press.pointerTracked = true;
    press.pointerHeld = true;
    press.pointer = ArtPoint{float(tradeLayout::accept.x + 3), float(tradeLayout::accept.y + 3)};
    press.pressed[unsigned(UiKey::Click)] = true;
    routeHudInput(controls, press);
    check(characterGold(game.party(), {0}) == 450, "Physical buy press waits for release");
    UiInput release;
    release.pointerTracked = true;
    release.pointerReleased = true;
    release.pointer = press.pointer;
    routeHudInput(controls, release);
    check(characterGold(game.party(), {0}) == 400, "Matching physical release commits exactly one purchase");
    routeHudInput(controls, click);
    routeHudInput(controls, press);
    other = {};
    other.pressed[unsigned(UiKey::Two)] = true;
    routeHudInput(controls, other);
    routeHudInput(controls, release);
    check(characterGold(game.party(), {0}) == 400 && characterGold(game.party(), {1}) == 0,
          "Character change cancels an armed purchase before its release");
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
    auto* state = ledger.visit(host, content.shops[0], {});
    for (unsigned i = 0; i < state->buyback.size(); ++i)
        state->buyback[i] = {{10}, 1, 15, gameDuration::day};
    ++state->revision;
    controls.refresh({}, false, false, true);
    hover(TradeSide::Merchant, 1);
    controls.tickDialogue(.35, true);
    check(controls.snapshot().trade.tooltipVisible && controls.snapshot().trade.pages == 2,
          "Bounded combined stock and buyback use merchant pages");
    UiInput page;
    page.pressed[unsigned(UiKey::NextPage)] = true;
    routeHudInput(controls, page);
    check(controls.snapshot().trade.page == 1 && !controls.snapshot().trade.tooltipVisible,
          "Changing merchant page resets the hover identity and dwell");
    UiInput back;
    back.pressed[unsigned(UiKey::Cancel)] = true;
    routeHudInput(controls, back);
    check(!controls.snapshot().trade.open && controls.snapshot().dialogue.open && game.inConversation(),
          "Escape returns to conversation without leaking input or ending it");
    controls.finishDialogue();
    check(!controls.snapshot().trade.open && !game.inConversation(),
          "Conversation teardown closes trade safely");
}
} // namespace
int main() try {
    parser();
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

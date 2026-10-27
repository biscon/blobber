#include "game/commands.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "support/inventory_fixtures.hpp"
#include "ui/interaction.hpp"
#include "ui/inventory_layout.hpp"
#include <chrono>
#include <cstring>
#include <iostream>
#include <string_view>
using namespace blobber;
namespace {
std::array<unsigned, 64> quantities(const Party& p) {
    std::array<unsigned, 64> totals{};
    std::array<bool, inventoryCapacity> seen{};
    for (std::uint16_t c = 0; c < 4; ++c) {
        std::int64_t weight = 0;
        for (std::uint16_t i = 0; i < 52; ++i) {
            const auto h = p.inventory().at({{c}, i});
            if (!h)
                continue;
            const auto* s = p.inventory().stack(h);
            check(s && !seen[h.slot], "Every placed handle owns exactly one live stack");
            seen[h.slot] = true;
            totals[s->item.index] += s->quantity;
            weight += std::int64_t(p.catalog().item(s->item)->weight) * s->quantity;
            check(s->quantity <= rules::stackLimit(*p.catalog().item(s->item)), "Stack limit holds");
        }
        check(weight == p.character({c})->stats.carriedWeight, "All equipment and bags counted exactly once");
    }
    return totals;
}
void transactions(int cycles = 100) {
    const auto cat = inventoryCatalog();
    Party p(cat), foreign(cat);
    const auto total = quantities(p);
    auto arrow = findInventoryItem(p, 2, 9);
    auto split = inventoryRequest(p, arrow, 2, 47, InventoryOp::Split, 1);
    check(bool(p.inventoryChange(split)), "Split succeeds");
    const auto single = p.inventory().at({{2}, 47});
    check(p.inventory().stack(single)->quantity == 1 && p.inventory().stack(arrow)->quantity == 19,
          "Split quantities");
    check(p.inventoryChange(split).error == InventoryError::Stale, "Old revision rejected");
    auto merge = inventoryRequest(p, single, 2, p.inventory().locate(arrow).cell);
    check(bool(p.inventoryChange(merge)) && !p.inventory().stack(single),
          "Merge invalidates exhausted handle");
    check(!foreign.inventory().stack(arrow), "Foreign identity rejected");
    for (int i = 0; i < cycles; ++i) {
        check(bool(p.inventoryChange(inventoryRequest(p, arrow, 2, 47, InventoryOp::Split, 1))),
              "Repeated split");
        const auto fresh = p.inventory().at({{2}, 47});
        check(fresh != single, "Generation prevents stale reuse");
        check(bool(p.inventoryChange(inventoryRequest(p, fresh, 2, p.inventory().locate(arrow).cell))),
              "Repeated merge");
    }
    check(bool(p.inventoryChange(inventoryRequest(p, arrow, 0, UINT16_MAX, InventoryOp::Transfer))),
          "Transfer to fighter");
    check(p.inventory().locate(arrow).character.index == 0, "Whole transfer preserves handle");
    const auto rev = p.inventory().revision();
    check(p.inventoryChange(inventoryRequest(p, arrow, 0, 47, InventoryOp::Split, 20)).error ==
              InventoryError::InvalidQuantity,
          "Split must leave one behind");
    check(p.inventory().revision() == rev && quantities(p) == total,
          "Rejected changes and organization conserve quantities");
    const auto shield = p.inventory().at({{0}, 50});
    const auto hp = p.character({0})->hitPoints;
    check(bool(p.inventoryChange(inventoryRequest(p, shield, 0, 46))), "Unequip shield");
    check(p.character({0})->stats.armorClass == 17 && p.character({0})->stats.weapons[49].damage.sides == 10,
          "Defense and versatile recalculate");
    check(p.character({0})->hitPoints == hp, "Equipment never restores health");
    check(bool(p.inventoryChange(inventoryRequest(p, shield, 0, 50))), "Equip shield again");
    const auto wizardPotion = findInventoryItem(p, 3, 10);
    check(p.inventoryChange(inventoryRequest(p, wizardPotion, 3, 48)).error == InventoryError::Equipment,
          "Potion cannot occupy armor slot");
    check(quantities(p) == total, "Gear failures conserve items");
    check(p.swapFormation(FormationSlot::FrontLeft, FormationSlot::BackRight) &&
              p.inventory().locate(shield).character.index == 0,
          "Formation changes do not change item ownership");
}
void fullAndWeight() {
    auto f = inventoryFixture();
    rules::ItemDefinition token;
    token.id = "fixture.token";
    token.name = "Token";
    token.description = "Zero-weight fixture";
    f.items.push_back(token);
    auto& c = f.preset.characters[0];
    c.items[c.itemCount++] = {{12}, 46}; // two potion stacks + 46 tokens
    const auto cat = inventoryCatalog(f);
    Party p(cat);
    const auto all = quantities(p);
    check(bool(p.inventory().at({{0}, 47})), "Legacy quantities normalize into full bag");
    const auto body = p.inventory().at({{0}, 48});
    const auto otherArmor = findInventoryItem(p, 1, 6);
    check(p.inventoryChange(inventoryRequest(p, otherArmor, 0, UINT16_MAX, InventoryOp::Transfer)).error ==
              InventoryError::Full,
          "Full bag rejects transfer");
    check(quantities(p) == all, "Full bag failure is atomic");
    auto potion = findInventoryItem(p, 1, 10);
    check(bool(p.inventoryChange(inventoryRequest(p, potion, 0, UINT16_MAX, InventoryOp::Transfer))),
          "Full bag accepts merge-only transfer");
    const auto tokenHandle = p.inventory().at({{0}, 47});
    check(p.inventoryChange(inventoryRequest(p, body, 0, 47)).error == InventoryError::Equipment,
          "Unequip cannot swap incompatible item into armor");
    check(p.inventory().at({{0}, 47}) == tokenHandle, "Rejected swap keeps both owners");
    // A replacement can occupy the source cell even when every bag cell is full.
    auto g = inventoryFixture();
    auto& fighter = g.preset.characters[0];
    g.items.push_back(token);
    fighter.items[fighter.itemCount++] = {{12}, 45};
    fighter.items[fighter.itemCount++] = {{5}, 1};
    const auto gearCat = inventoryCatalog(g);
    Party gear(gearCat);
    const auto leather = findInventoryItem(gear, 0, 5);
    const auto loc = gear.inventory().locate(leather);
    check(bool(gear.inventoryChange(inventoryRequest(gear, leather, 0, 48))),
          "Full-bag armor replacement succeeds");
    check(gear.inventory().stack(gear.inventory().at(loc))->item.index == 7,
          "Old armor returns to vacated cell");
    auto w = inventoryFixture();
    w.items[7].weight = 22900; // Fighter 240 lb; rogue only 120 lb.
    const auto weightCat = inventoryCatalog(w);
    Party weighted(weightCat);
    check(weighted.character({0})->stats.carriedWeight == weighted.character({0})->stats.carryingCapacity,
          "Exact carrying capacity accepted");
    const auto before = quantities(weighted);
    const auto heavy = weighted.inventory().at({{0}, 48});
    check(weighted.inventoryChange(inventoryRequest(weighted, heavy, 2, UINT16_MAX, InventoryOp::Transfer))
                  .error == InventoryError::Overweight,
          "Weight limit rejects transfer after staging");
    check(quantities(weighted) == before && weighted.inventory().at({{0}, 48}) == heavy,
          "Weight failure preserves equipment and source");
    auto overflow = f;
    overflow.preset.characters[0].items[5].quantity = 47;
    const auto overCat = inventoryCatalog(overflow);
    rejects([&] { Party tooFull(overCat); }, "Legacy over-capacity startup rejects");
}
PartyCommand potionCommand(GameSession& s, unsigned who, unsigned item, unsigned ally) {
    PartyCommand c;
    c.action = PartyAction::Item;
    c.character = {static_cast<std::uint16_t>(who)};
    c.ally = rules::CharacterId{static_cast<std::uint16_t>(ally)};
    c.item = findInventoryItem(s.party(), who, item);
    c.inventoryRevision = s.party().inventory().revision();
    c.world = s.combat().snapshot().world;
    c.turn = s.combat().snapshot().turn;
    return c;
}
void sessionIntegration() {
    const auto map = hybridRoom();
    const auto cat = inventoryCatalog();
    const Player spawn{{6, 8}, Heading::North};
    const NpcWorld empty(map, spawn.cell, {}, {});
    GameSession s(map, cat, empty, 5, spawn, false);
    const auto armor = s.party().inventory().at({{1}, 48});
    auto r = inventoryRequest(s.party(), armor, 1, 47);
    check(bool(s.inventoryChange(r)), "Safe session unequip");
    EffectRequest e;
    e.world = s.actors().snapshot().world;
    e.caster = {3};
    e.kind = EffectKind::MageArmor;
    e.targets[0] = rules::CharacterId{1};
    e.count = 1;
    check(bool(s.applyEffect(e)), "Mage Armor applies to unarmored cleric");
    check(bool(s.inventoryChange(inventoryRequest(s.party(), armor, 1, 48))), "Reequip reconciles effects");
    check(!s.effects().has(rules::CharacterId{1}, EffectKind::MageArmor) && s.events().count >= 1,
          "Armor ends effect immediately with event");
    s.party().damage({1}, s.party().character({1})->hitPoints);
    auto c = potionCommand(s, 0, 10, 1);
    const auto original = quantities(s.party());
    const auto out = s.submit(c);
    check(out.reason == CommandReason::Resolved && out.event == CombatEvent::Healing && out.damage >= 4 &&
              out.damage <= 10,
          "Potion heals using 2d4+2");
    check(s.party().character({1})->life == rules::Life::Conscious, "Healing revives downed ally");
    check(s.events().count == 1 && s.events().values[0].item.index == 10,
          "Exactly one item restoration event");
    check(quantities(s.party())[10] + 1 == original[10], "Exactly one potion consumed");
    check(s.submit(c).reason == CommandReason::StaleTurn, "Duplicate potion command rejected");
    check(s.submit(potionCommand(s, 0, 10, 0)).reason == CommandReason::Cooldown,
          "Cooldown shared across uses");
    const auto clock = s.timeMs();
    s.update(60, true);
    check(s.timeMs() == clock, "Browsing pause does not recharge");
    s.party().spendMana({3}, 3);
    const auto mana = s.submit(potionCommand(s, 1, 11, 3));
    check(mana.damage == 2 && s.party().character({3})->mana == 3,
          "Ally mana restoration capped independently of caster");
    check(s.submit(potionCommand(s, 2, 11, 0)).reason == CommandReason::FullMana,
          "Noncasters reject mana potions");
    s.party().damage({2}, 1000);
    check(s.submit(potionCommand(s, 3, 10, 2)).reason == CommandReason::DeadAlly, "No resurrection");
    // Safe gate observes threats and pending attacks, even before impact causes alert.
    NpcWorld places(map, spawn.cell, {"goblin"}, {{{0}, {6, 4}, {0, 0}, Heading::North}});
    GameSession danger(map, cat, places, 9, spawn, false);
    const auto arrows = findInventoryItem(danger.party(), 2, 9);
    danger.party().consumeItem(arrows, 19);
    auto shot = turnCommand(danger, PartyAction::Ranged);
    shot.character = {2};
    shot.target = danger.actors().id(0);
    const int hp = danger.actors().actor(*shot.target)->hitPoints;
    check(danger.submit(shot).reason == CommandReason::Resolved, "Last arrow accepted");
    check(!danger.party().inventory().stack(arrows) && danger.actors().actor(*shot.target)->hitPoints == hp,
          "Arrow spent at acceptance before impact");
    const auto sword = danger.party().inventory().at({{0}, 49});
    check(danger.inventoryChange(inventoryRequest(danger.party(), sword, 0, 47)).error ==
              InventoryError::Unsafe,
          "Pending shot locks equipment");
    const auto heal = findInventoryItem(danger.party(), 0, 10);
    check(bool(danger.inventoryChange(inventoryRequest(danger.party(), heal, 0, 46))),
          "Bag organization allowed during flight");
    danger.actors().remove(*shot.target);
    danger.update(6, false);
    check(danger.party().inventory().count({2}, rules::Ammunition::Arrow, cat) == 0,
          "Target removal never refunds ammunition");
}
void partialMerge() {
    auto f = inventoryFixture();
    for (auto& item : f.preset.characters[2].items)
        if (item.item.index == 9)
            item.quantity = 195;
    const auto cat = inventoryCatalog(f);
    Party p(cat);
    const auto total = quantities(p);
    const auto a = findInventoryItem(p, 2, 9);
    const auto first = p.inventory().locate(a).cell;
    const auto b = p.inventory().at({{2}, static_cast<std::uint16_t>(first + 1)});
    check(p.inventory().stack(a)->quantity == 100 && p.inventory().stack(b)->quantity == 95,
          "Legacy large quantity partitions at stack limit");
    check(p.inventoryChange(inventoryRequest(p, a, 2, first + 1, InventoryOp::Move, 6)).error ==
                  InventoryError::Full &&
              quantities(p) == total,
          "Explicit merge beyond remaining capacity is atomic");
    check(bool(p.inventoryChange(inventoryRequest(p, a, 2, first + 1))) &&
              p.inventory().stack(a)->quantity == 95 && p.inventory().stack(b)->quantity == 100,
          "Default merge moves only what fits");
    check(quantities(p) == total, "Partial merge conserves all quantities");
}
void rejectionBoundaries() {
    auto f = inventoryFixture();
    rules::ItemDefinition token;
    token.id = "fixture.token";
    token.name = "Token";
    token.description = "Fixture";
    f.items.push_back(token);
    // All four bags are full; fighter has just one potion of merge capacity.
    for (unsigned c = 0; c < 4; ++c) {
        auto& b = f.preset.characters[c];
        if (!c)
            for (unsigned k = 0; k < b.itemCount; ++k)
                if (b.items[k].item.index == 10)
                    b.items[k].quantity = 19;
        unsigned bag = 0;
        for (unsigned k = 0; k < b.itemCount; ++k)
            if (k != b.equipment.body.index && k != b.equipment.mainHand.index &&
                k != b.equipment.offHand.index && k != b.equipment.ranged.index)
                ++bag;
        b.items[b.itemCount++] = {{12}, static_cast<std::uint16_t>(48 - bag)};
    }
    const auto fullCat = inventoryCatalog(f);
    Party full(fullCat);
    const auto total = quantities(full);
    for (unsigned c = 0; c < 4; ++c)
        check(bool(full.inventory().at({{static_cast<std::uint16_t>(c)}, 47})),
              "All four bags reach capacity");
    const auto healing = findInventoryItem(full, 1, 10);
    const auto revision = full.inventory().revision();
    check(full.inventoryChange(inventoryRequest(full, healing, 0, UINT16_MAX, InventoryOp::Transfer)).error ==
              InventoryError::Full,
          "Partial merge then exhausted destination rejects atomically");
    check(full.inventory().revision() == revision && quantities(full) == total,
          "Failed partial merge preserves all four inventories");
    check(
        bool(full.inventoryChange(inventoryRequest(full, healing, 0, UINT16_MAX, InventoryOp::Transfer, 1))),
        "Exact partial transfer fills final stack space");
    check(full.inventory().stack(healing)->quantity == 1, "Explicit quantity leaves remainder in owner bag");
    const auto map = hybridRoom();
    const auto cat = inventoryCatalog();
    const Player spawn{{6, 8}, Heading::North};
    const NpcWorld empty(map, spawn.cell, {}, {});
    GameSession rejected(map, cat, empty, 31, spawn, false), control(map, cat, empty, 31, spawn, false);
    const auto before = quantities(rejected.party());
    check(rejected.submit(potionCommand(rejected, 0, 10, 0)).reason == CommandReason::FullHealth,
          "Full health rejects without consuming");
    check(rejected.submit(potionCommand(rejected, 0, 11, 0)).reason == CommandReason::FullMana,
          "No mana capacity rejects without consuming");
    auto foreign = potionCommand(rejected, 0, 10, 1);
    foreign.item = findInventoryItem(control.party(), 0, 10);
    check(rejected.submit(foreign).reason == CommandReason::InvalidItem, "Foreign potion handle rejects");
    auto stale = potionCommand(rejected, 0, 10, 1);
    --stale.inventoryRevision;
    check(rejected.submit(stale).reason == CommandReason::StaleInventory, "Stale potion revision rejects");
    check(quantities(rejected.party()) == before, "All rejected potion costs unchanged");
    rejected.party().damage({1}, 10);
    control.party().damage({1}, 10);
    const auto a = rejected.submit(potionCommand(rejected, 0, 10, 1));
    const auto b = control.submit(potionCommand(control, 0, 10, 1));
    check(a.damage == b.damage &&
              rejected.party().character({1})->hitPoints == control.party().character({1})->hitPoints,
          "Rejected uses preserve deterministic RNG");
    rejected.update(6, false);
    rejected.party().damage({0}, 1);
    const auto cap = rejected.submit(potionCommand(rejected, 0, 10, 0));
    check(cap.reason == CommandReason::Resolved && cap.damage == 1, "Healing clamps to missing HP");
    rejected.party().spendMana({3}, 1);
    const auto mana = rejected.submit(potionCommand(rejected, 1, 11, 3));
    check(mana.reason == CommandReason::Resolved && mana.damage == 1 &&
              rejected.party().character({3})->mana == 4,
          "Mana clamps to capacity");
    // Real turn action, independently of which character is inspected.
    NpcWorld nearby(map, spawn.cell, {"goblin"}, {{{0}, {6, 7}, {0, 0}, Heading::South}});
    GameSession turns(map, cat, nearby, 5, spawn, false);
    turns.update(0, false);
    for (int step = 0; step < 50 && !turns.combat().snapshot().awaitingAction; ++step)
        turns.update(.5, false);
    check(turns.combat().snapshot().awaitingAction, "Potion fixture reaches player turn");
    const auto actor = *turns.combat().snapshot().activeCharacter;
    turns.party().damage(actor, 1);
    const auto turn = turns.combat().snapshot().turn;
    const auto used = turns.submit(potionCommand(turns, actor.index, 10, actor.index));
    check(used.reason == CommandReason::Resolved && !turns.combat().snapshot().awaitingAction &&
              turns.combat().snapshot().resolvingAction,
          "Potion spends turn and starts feedback hold");
    check(turns.submit(potionCommand(turns, actor.index, 10, actor.index)).reason ==
              CommandReason::NotYourTurn,
          "Resolving action cannot be repeated");
    turns.update(.999, false);
    check(turns.combat().snapshot().turn == turn, "Potion feedback holds turn");
    turns.update(.001, false);
    check(!turns.combat().snapshot().resolvingAction, "Potion feedback hands off exactly once");
}
void staleAndKeyboard() {
    const auto map = hybridRoom();
    const auto cat = inventoryCatalog();
    const Player spawn{{6, 8}, Heading::North};
    const NpcWorld empty(map, spawn.cell, {}, {});
    GameSession s(map, cat, empty, 5, spawn, false);
    CommandController ui(s);
    ui.refresh({}, false, false, true);
    ui.openInventory(rules::CharacterId{2});
    check(ui.snapshot().inventory.cells[51].weapon.damage.modifier == 3 &&
              ui.snapshot().inventory.cells[51].weapon.attackBonus == 5,
          "Inventory exposes derived weapon bonuses");
    const auto h = findInventoryItem(s.party(), 2, 9);
    const auto source = s.party().inventory().locate(h).cell;
    ui.inventorySelect(source);
    ui.inventoryDrop({{2}, 47}, true);
    check(bool(s.inventoryChange(inventoryRequest(s.party(), h, 2, 46, InventoryOp::Split, 1))),
          "External transaction changes pending revision");
    const auto rev = s.party().inventory().revision();
    ui.inventoryConfirm();
    check(s.party().inventory().revision() == rev && !s.party().inventory().at({{2}, 47}),
          "Stale dialog never applies quantity");
    ui.refresh({}, false, false, true);
    ui.inventorySelect(source);
    UiInput tab;
    tab.pressed[static_cast<std::size_t>(UiKey::Tab)] = true;
    routeHudInput(ui, tab, {});
    check(s.party().inventory().revision() == rev && ui.snapshot().inventory.cursor == 48,
          "Tab navigates equipment without committing a move");
    ui.inventoryCursor(45);
    UiInput enter;
    enter.pressed[static_cast<std::size_t>(UiKey::Confirm)] = true;
    routeHudInput(ui, enter, {});
    check(s.party().inventory().revision() == rev && s.party().inventory().at({{2}, source}) == h,
          "Keyboard navigation never implicitly moves an item");
    UiInputEdges edges;
    std::array<bool, uiKeyCount> down{};
    down[static_cast<std::size_t>(UiKey::Click)] = true;
    const auto first = edges.sample(down, true, ArtPoint{180, 85});
    check(first.has(UiKey::Click) && first.pointerHeld, "Fresh pointer edge begins drag");
    edges.sample(down, false);
    const auto refocus = edges.sample(down, true);
    check(!refocus.has(UiKey::Click) && !refocus.pointerHeld, "Held pointer blocked across focus loss");
}
void contextualActions() {
    auto f = inventoryFixture();
    for (const auto* id : {"fixture.book", "fixture.symbol", "fixture.focus"}) {
        rules::ItemDefinition item;
        item.id = id;
        item.name = id;
        item.description = "Test-owned gear with no active use";
        const auto index = static_cast<std::uint16_t>(f.items.size());
        f.items.push_back(item);
        auto& wizard = f.preset.characters[3];
        wizard.items[wizard.itemCount++] = {{index}, 1};
    }
    const auto cat = inventoryCatalog(f);
    const auto map = hybridRoom();
    const Player spawn{{6, 8}, Heading::North};
    const NpcWorld empty(map, spawn.cell, {}, {});
    GameSession s(map, cat, empty, 5, spawn, false);
    CommandController ui(s);
    ui.refresh({}, false, false, true);
    UiInput hover;
    const auto dock = hudLayout::action(4);
    hover.pointer = ArtPoint{float(dock.x + dock.width / 2), float(dock.y + dock.height / 2)};
    routeHudInput(ui, hover, {});
    check(std::strstr(ui.snapshot().hover.lines[0].data(), "Inventory") &&
              std::strstr(ui.snapshot().hover.lines[3].data(), "press I"),
          "Backpack tooltip describes inventory and its shortcut");
    UiInput open;
    open.pressed[static_cast<std::size_t>(UiKey::Inventory)] = true;
    routeHudInput(ui, open, {});
    check(ui.snapshot().inventory.open, "I opens inventory through the production router");
    ui.inventoryCharacter({3});
    const auto original = quantities(s.party());
    const auto revision = s.party().inventory().revision();
    for (unsigned item = 12; item < 15; ++item) {
        ui.inventorySelect(s.party().inventory().locate(findInventoryItem(s.party(), 3, item)).cell);
        const auto& actions = ui.snapshot().inventory.actions;
        check(!actions[0].visible && actions[1].visible && actions[1].enabled && !actions[2].visible,
              "Book, holy symbol and focus show only Transfer");
        UiInput use;
        use.pressed[static_cast<std::size_t>(UiKey::UseItem)] = true;
        routeHudInput(ui, use, {});
        ui.inventoryAction(InventoryAction::Equip);
        check(!ui.snapshot().inventory.itemTargeting &&
                  ui.snapshot().inventory.mode == InventoryMode::Browse &&
                  s.party().inventory().revision() == revision,
              "Hidden use/equip shortcuts cannot start an action");
        UiInput obsolete;
        obsolete.pointer = ArtPoint{500, 315};
        obsolete.pressed[static_cast<std::size_t>(UiKey::Click)] = true;
        routeHudInput(ui, obsolete, {});
        check(ui.snapshot().inventory.mode == InventoryMode::Browse,
              "Removed action rows have no invisible click targets");
    }
    ui.inventorySelect(s.party().inventory().locate(findInventoryItem(s.party(), 3, 10)).cell);
    check(!ui.snapshot().inventory.actions[0].visible && ui.snapshot().inventory.actions[2].visible &&
              !ui.snapshot().inventory.actions[2].enabled,
          "Potion use is disabled when nobody needs healing");
    ui.inventoryAction(InventoryAction::Use);
    check(!ui.snapshot().inventory.itemTargeting && quantities(s.party()) == original,
          "Disabled use does not leave inventory or consume items");
    s.party().damage({1}, 5);
    ui.refresh({}, false, false, true);
    check(ui.snapshot().inventory.actions[2].enabled, "Potion use enables for an eligible ally");
    ui.inventoryAction(InventoryAction::Use);
    check(ui.snapshot().inventory.itemTargeting, "Applicable enabled use enters targeting");
    ui.cancel();
    ui.openInventory(rules::CharacterId{0});
    ui.inventorySelect(50);
    check(ui.snapshot().inventory.actions[0].enabled &&
              std::string_view(ui.snapshot().inventory.actions[0].label) == "E  UNEQUIP",
          "Equipment offers explicit unequip");
    ui.inventoryAction(InventoryAction::Equip);
    check(ui.snapshot().inventory.actions[0].enabled &&
              std::string_view(ui.snapshot().inventory.actions[0].label) == "E  EQUIP",
          "Removed equipment offers equip");
    ui.inventoryCharacter({2});
    ui.inventorySelect(s.party().inventory().locate(findInventoryItem(s.party(), 2, 9)).cell);
    check(!ui.snapshot().inventory.actions[0].visible && !ui.snapshot().inventory.actions[2].visible,
          "Arrows have neither equip nor use action");
    const NpcWorld nearby(map, spawn.cell, {"goblin"}, {{{0}, {6, 7}, {0, 0}, Heading::South}});
    GameSession danger(map, cat, nearby, 5, spawn, false);
    CommandController locked(danger);
    locked.record(danger.update(0, false));
    locked.refresh({}, false, false, true);
    locked.openInventory(rules::CharacterId{0});
    locked.inventorySelect(50);
    check(locked.snapshot().inventory.actions[0].visible && !locked.snapshot().inventory.actions[0].enabled &&
              !locked.snapshot().inventory.actions[1].enabled,
          "Unsafe equipment and transfers remain visibly disabled");
}
void inputBehavior() {
    const auto map = hybridRoom();
    const auto cat = inventoryCatalog();
    const Player spawn{{6, 8}, Heading::North};
    const NpcWorld empty(map, spawn.cell, {}, {});
    GameSession s(map, cat, empty, 5, spawn, false);
    CommandController ui(s);
    ui.refresh({}, false, false, true);
    ui.openInventory(rules::CharacterId{2});
    const auto arrow = findInventoryItem(s.party(), 2, 9);
    const auto cell = s.party().inventory().locate(arrow).cell;
    const auto a = inventoryLayout::cell(cell), b = inventoryLayout::cell(47);
    auto pointer = [&](UiRect r, bool press, bool held, bool release, bool shift) {
        UiInput in;
        in.pointer = ArtPoint{float(r.x + 10), float(r.y + 10)};
        in.pointerHeld = held;
        in.pointerReleased = release;
        in.shift = shift;
        in.pressed[static_cast<std::size_t>(UiKey::Click)] = press;
        routeHudInput(ui, in);
    };
    pointer(a, true, true, false, true);
    pointer(b, false, true, false, true);
    pointer(b, false, false, true, true);
    check(ui.snapshot().inventory.mode == InventoryMode::Quantity && ui.snapshot().inventory.quantity == 1,
          "Shift drag opens default-one dialog");
    check(!s.party().inventory().at({{2}, 47}), "Drag did not mutate before confirmation");
    ui.inventoryConfirm();
    check(s.party().inventory().stack(s.party().inventory().at({{2}, 47}))->quantity == 1,
          "Dialog commits exactly selected quantity");
    const auto rev = s.party().inventory().revision();
    pointer(a, true, true, false, false);
    pointer(b, false, true, false, false);
    ui.refresh({}, false, false, false);
    pointer(b, false, false, true, false);
    check(s.party().inventory().revision() == rev && !ui.snapshot().inventory.dragging,
          "Blur cancels drag without release commit");
    ui.refresh({}, false, false, true);
    ui.inventorySelect(cell);
    ui.inventoryDrop({{2}, 46}, true);
    check(ui.snapshot().inventory.mode == InventoryMode::Quantity, "Pending split can be canceled");
    ui.inventoryCancel();
    check(s.party().inventory().revision() == rev, "Canceled split spends nothing");
    ui.inventoryAction(InventoryAction::Transfer);
    ui.inventoryCharacter({0});
    check(s.party().inventory().locate(arrow).character.index == 0,
          "Character tab transfer uses authoritative transaction");
    check(!ui.snapshot().inventory.selected,
          "Transferred item is no longer selected in the former owner bag");
    ui.inventoryCharacter({0});
    ui.inventorySelect(s.party().inventory().locate(findInventoryItem(s.party(), 0, 10)).cell);
    s.party().damage({1}, 5);
    ui.inventoryAction(InventoryAction::Use);
    check(!ui.snapshot().modal() && ui.snapshot().inventory.itemTargeting, "Use resumes live targeting");
    ui.inventoryAlly(1, true);
    check(!ui.snapshot().inventory.itemTargeting && s.party().character({1})->hitPoints > 7,
          "Portrait use commits and closes targeting");
}
} // namespace
int main(int argc, char** argv) {
    try {
        const bool stress = argc > 1 && std::string_view(argv[1]) == "--stress";
        const auto start = std::chrono::steady_clock::now();
        transactions(stress ? 100000 : 100);
        fullAndWeight();
        sessionIntegration();
        partialMerge();
        rejectionBoundaries();
        staleAndKeyboard();
        inputBehavior();
        contextualActions();
        std::cout << "Inventory transactions, costs, effects and input passed ("
                  << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                                           start)
                         .count()
                  << " ms)\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

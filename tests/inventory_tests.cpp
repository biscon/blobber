#include "game/commands.hpp"
#include "game/trade.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "support/early_equipment_fixtures.hpp"
#include "support/inventory_fixtures.hpp"
#include "ui/interaction.hpp"
#include "ui/button_targets.hpp"
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
        for (std::uint16_t i = 0; i < inventoryCells; ++i) {
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
    e.caster = rules::CharacterId{3};
    e.kind = EffectKind::MageArmor;
    e.targets[0] = rules::CharacterId{1};
    e.count = 1;
    check(bool(s.applyEffect(e)), "Mage Armor applies to unarmored cleric");
    check(bool(s.inventoryChange(inventoryRequest(s.party(), armor, 1, 48))), "Reequip reconciles effects");
    check(!s.effects().has(rules::CharacterId{1}, EffectKind::MageArmor) && s.events().count >= 1,
          "Armor ends effect immediately with event");
    s.party().damage({1}, s.party().character({1})->hitPoints);
    check(bool(s.party().stabilize({1})) && s.party().character({1})->life == rules::Life::Stable,
          "stable unconscious ally is a legal administered potion recipient");
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
    check(s.party().inventory().revision() == rev &&
              ui.snapshot().inventory.cursor == unsigned(EquipmentSlot::Head),
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
void characterSheet() {
    auto f = inventoryFixture();
    f.classes[0].features = static_cast<rules::ClassFeatures>(rules::ClassFeature::SecondWind);
    f.classes[2].features = static_cast<rules::ClassFeatures>(rules::ClassFeature::SneakAttack);
    const auto cat = inventoryCatalog(f);
    const auto map = hybridRoom();
    const Player spawn{{6, 8}, Heading::North};
    const NpcWorld empty(map, spawn.cell, {}, {});
    GameSession s(map, cat, empty, 5, spawn, false);
    check(s.party().swapFormation(FormationSlot::FrontLeft, FormationSlot::BackLeft),
          "Fixture swaps formation");
    CommandController ui(s);
    auto refresh = [&] { ui.refresh({}, false, false, true); };
    auto click = [&](UiRect r) {
        UiInput in;
        in.pointer = ArtPoint{float(r.x + r.width / 2), float(r.y + r.height / 2)};
        in.pressed[static_cast<unsigned>(UiKey::Click)] = true;
        routeHudInput(ui, in, {});
    };
    refresh();
    const auto actor = ui.snapshot().selection.character;
    click(hudLayout::detail(0));
    const auto& i = ui.snapshot().inventory;
    check(i.open && i.page == CharacterPage::Character && i.character.index == 2 &&
              ui.snapshot().selection.character.index == actor.index,
          "Portrait info opens Character by formation identity without changing actor");
    const auto clock = s.timeMs();
    s.update(1, ui.snapshot().modal());
    check(s.timeMs() == clock, "Character sheet pauses gameplay");
    check(i.sheet.modifiers[0] == -1 && i.sheet.modifiers[1] == 3 && i.sheet.saves[1] == 5 &&
              i.sheet.saves[3] == 3 && i.sheet.proficientSaves[1] && !i.sheet.proficientSaves[0],
          "Character abilities, saves and training are derived from the selected character");
    check(i.sheet.hitDice == 1 && i.sheet.hitDie == 8 && i.sheet.experience == 0 &&
              i.sheet.nextLevelExperience == 300 && i.sheet.arrows == 20 &&
              std::strstr(i.sheet.weapons.data(), "Shortswords") && i.sheet.featureCount == 1,
          "Sheet exposes resources, XP preview, training and class features");
    click(inventoryLayout::inventoryPage);
    const auto arrows = findInventoryItem(s.party(), 2, 9);
    const auto source = s.party().inventory().locate(arrows).cell;
    ui.inventorySelect(source);
    const auto revision = s.party().inventory().revision();
    ui.inventoryDrop({{2}, 47}, true);
    check(i.mode == InventoryMode::Quantity, "Fixture starts a pending split");
    click(inventoryLayout::characterPage);
    check(i.mode == InventoryMode::Browse && !i.source && i.selected == arrows && i.cursor == source,
          "Changing page cancels pending split but preserves selection and cursor");
    UiInput hidden;
    hidden.pressed[static_cast<unsigned>(UiKey::UseItem)] = true;
    hidden.pressed[static_cast<unsigned>(UiKey::Equip)] = true;
    hidden.pressed[static_cast<unsigned>(UiKey::Confirm)] = true;
    routeHudInput(ui, hidden, {});
    click(inventoryLayout::cell(47));
    check(s.party().inventory().revision() == revision && !i.itemTargeting && i.selected == arrows,
          "Character page consumes inventory shortcuts and hidden grid clicks");
    click(inventoryLayout::inventoryPage);
    check(i.page == CharacterPage::Inventory && i.character.index == 2 && i.selected == arrows,
          "Both directions retain character and inventory selection");
    const auto from = inventoryLayout::cell(source);
    ui.inventoryPointer(from.x + 4, from.y + 4, true, true, false, false);
    ui.inventoryPointer(from.x + 30, from.y + 30, false, true, false, false);
    check(i.dragging, "Fixture begins dragging an item");
    click(inventoryLayout::characterPage);
    UiInput release;
    release.pointerReleased = true;
    release.pointer =
        ArtPoint{float(inventoryLayout::cell(47).x + 4), float(inventoryLayout::cell(47).y + 4)};
    routeHudInput(ui, release, {});
    check(!i.dragging && !i.pointerDown && !i.source && s.party().inventory().revision() == revision,
          "Tab change cancels drag and its later release cannot move inventory");
    click(inventoryLayout::inventoryPage);
    ui.inventoryAction(InventoryAction::Transfer);
    click(inventoryLayout::characterPage);
    click(inventoryLayout::tab(1));
    check(i.character.index == 1 && s.party().inventory().revision() == revision,
          "Switching from transfer then selecting a character never transfers the item");
    click(inventoryLayout::tab(3));
    check(std::strstr(i.sheet.weapons.data(), "Daggers") && std::strstr(i.sheet.weapons.data(), "Shortbows"),
          "Training combines race and class");
    EffectRequest effect;
    effect.world = s.actors().snapshot().world;
    effect.caster = rules::CharacterId{3};
    effect.kind = EffectKind::MageArmor;
    effect.targets[0] = rules::CharacterId{3};
    effect.count = 1;
    check(bool(s.applyEffect(effect)), "Sheet effect fixture applies");
    s.party().damage({3}, 2);
    s.party().spendMana({3}, 1);
    refresh();
    check(i.armorClass == s.effects().armorClass(rules::CharacterId{3}) &&
              i.armorClass > s.party().character({3})->stats.armorClass &&
              std::strstr(i.sheet.effects.data(), "Mage Armor") &&
              !std::strstr(i.sheet.effects.data(), "Concentrating") &&
              ui.snapshot().cards[3].mana == s.party().character({3})->mana &&
              ui.snapshot().cards[3].hp == s.party().character({3})->hitPoints,
          "Sheet displays live effective AC, effects, HP and mana");
    const auto weapon = s.party().inventory().at({{3}, 49});
    check(bool(s.inventoryChange(inventoryRequest(s.party(), weapon, 3, 47))), "Remove weapon fixture");
    refresh();
    check(!i.cells[49].definition && !i.cells[49].weapon.available && !i.cells[51].definition,
          "Empty weapon slots expose no stale attack preview");
    ui.inventoryCharacter({0});
    check(i.sheet.featureCount == 2 && std::strcmp(i.sheet.features[1].value.data(), "1 / 1 use") == 0,
          "Fighter sheet includes Defense and available Second Wind");
    s.party().useFeature({0}, rules::Feature::SecondWind);
    const auto shield = s.party().inventory().at({{0}, 50});
    const int oldAC = i.armorClass;
    check(bool(s.inventoryChange(inventoryRequest(s.party(), shield, 0, 47))), "Remove shield fixture");
    refresh();
    check(i.armorClass == oldAC - 2 && i.cells[49].weapon.damage.sides == 10 &&
              std::strcmp(i.sheet.features[1].value.data(), "0 / 1 use") == 0,
          "Gear and feature changes update AC, versatile damage and charges");
    click(inventoryLayout::close);
    check(!ui.snapshot().modal(), "Fullscreen close consumes its click");
    UiInput open;
    open.pressed[static_cast<unsigned>(UiKey::Inventory)] = true;
    routeHudInput(ui, open, {});
    check(i.open && i.page == CharacterPage::Inventory && i.character.index == actor.index,
          "I entry resets to Inventory for the gameplay selection");
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
        check(!actions[0].visible && actions[1].visible && actions[1].enabled && !actions[2].visible &&
                  actions[3].visible && actions[3].enabled,
              "Book, holy symbol and focus show Transfer and Drop");
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
void inventoryToggle() {
    const auto map = hybridRoom();
    const auto cat = inventoryCatalog();
    const Player spawn{{6, 8}, Heading::North};
    const NpcWorld empty(map, spawn.cell, {}, {});
    GameSession session(map, cat, empty, 5, spawn, false);
    CommandController ui(session);
    ui.refresh({}, false, false, true);
    UiInputEdges edges;
    std::array<bool, uiKeyCount> down{};
    const auto sample = [&] { routeHudInput(ui, edges.sample(down, true)); };
    const auto press = [&] {
        down[unsigned(UiKey::Inventory)] = false;
        sample();
        down[unsigned(UiKey::Inventory)] = true;
        sample();
    };
    const auto& inventory = ui.snapshot().inventory;
    press();
    check(inventory.open && inventory.page == CharacterPage::Inventory, "I opens inventory");
    sample();
    check(inventory.open, "Holding I does not immediately close inventory");
    press();
    check(!inventory.open && !ui.snapshot().modal(), "Next I press closes inventory and releases pause");
    sample();
    check(!inventory.open, "Held closing key does not reopen inventory");
    press();
    check(inventory.open, "A fresh I press can reopen inventory");
    UiInput click;
    click.pressed[unsigned(UiKey::Click)] = true;
    const auto tab = inventoryLayout::inventoryPage;
    click.pointer = ArtPoint{float(tab.x + 3), float(tab.y + 3)};
    routeHudInput(ui, click);
    check(inventory.open, "Clicking the Inventory tab does not toggle the overlay closed");
    ui.inventoryPage(CharacterPage::Character);
    press();
    check(!inventory.open, "I also closes the shared Character page");

    ui.openInventory(rules::CharacterId{2});
    const auto arrow = findInventoryItem(session.party(), 2, 9);
    const auto cell = session.party().inventory().locate(arrow).cell;
    const auto revision = session.party().inventory().revision();
    ui.inventorySelect(cell);
    ui.inventoryDrop({{2}, 47}, true);
    check(inventory.mode == InventoryMode::Quantity, "Toggle fixture has an unconfirmed split");
    press();
    check(!inventory.open && inventory.mode == InventoryMode::Browse &&
              session.party().inventory().revision() == revision &&
              !session.party().inventory().at({{2}, 47}),
          "I closes a pending split without committing it");
    ui.openInventory(rules::CharacterId{2});
    const auto from = inventoryLayout::cell(cell), to = inventoryLayout::cell(47);
    ui.inventoryPointer(float(from.x + 4), float(from.y + 4), true, true, false, false);
    ui.inventoryPointer(float(to.x + 4), float(to.y + 4), false, true, false, false);
    check(inventory.dragging, "Toggle fixture has an active drag");
    press();
    UiInput release;
    release.pointer = ArtPoint{float(to.x + 4), float(to.y + 4)};
    release.pointerReleased = true;
    routeHudInput(ui, release);
    check(!inventory.open && !inventory.dragging && !inventory.pointerDown &&
              session.party().inventory().revision() == revision &&
              !session.party().inventory().at({{2}, 47}),
          "I cancels the drag and its later release cannot move an item");
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
void wearableEquipment() {
    const auto cat = inventoryCatalog(wearableFixture());
    Party p(cat);
    const int ac = p.character({0})->stats.armorClass;
    const auto initialWeight = p.character({0})->stats.carriedWeight;
    constexpr EquipmentSlot slots[]{EquipmentSlot::Head,  EquipmentSlot::Hands,  EquipmentSlot::Feet,
                                    EquipmentSlot::Belt,  EquipmentSlot::Cloak,  EquipmentSlot::Ring1,
                                    EquipmentSlot::Ring2, EquipmentSlot::Amulet, EquipmentSlot::Wrists};
    for (std::uint16_t who = 0; who < rules::partySize; ++who) {
        for (unsigned n = 0; n < std::size(slots); ++n) {
            check(bool(p.acquire({{who}, {std::uint16_t(12 + n)}, 1, p.inventory().revision()})),
                  "Acquire test-owned wearable");
            const auto item = findInventoryItem(p, who, 12 + n);
            check(bool(p.inventoryChange(inventoryRequest(p, item, who, unsigned(slots[n])))),
                  "Every new equipment position accepts its own wearing location");
            check(p.equipped({who}, slots[n]) == cat.item({std::uint16_t(12 + n)}),
                  "Equipped queries expose every new position");
        }
        unsigned empty = 0;
        for (std::uint16_t cell = 0; cell < bagCells; ++cell)
            empty += !p.inventory().at({{who}, cell});
        check(bool(p.acquire({{who}, {22}, std::uint16_t(empty), p.inventory().revision()})),
              "New equipment does not reduce 48-cell bag capacity");
    }
    check(p.character({0})->stats.carriedWeight == initialWeight + 9 * 25 &&
              p.character({0})->stats.armorClass == ac,
          "Wearables count weight exactly once without inventing AC bonuses");
    const auto comparison = compareItem(p, {3}, *cat.item({12}));
    check(comparison.equippable && comparison.restriction[0] == 0 &&
              comparison.offeredArmorClass == p.character({3})->stats.armorClass &&
              !p.equipped({0}, static_cast<EquipmentSlot>(0)),
          "Wearable comparisons invent no shield bonus/training requirement; invalid equipment is empty");
    unsigned live = 0;
    for (const auto& stack : p.inventory().checkpoint().stacks)
        live += stack.quantity != 0;
    check(live > 208 && live <= inventoryCapacity, "Expanded pool supports all four full bags and new gear");
    const auto before = quantities(p);
    const auto head = p.inventory().at({{0}, unsigned(EquipmentSlot::Head)});
    check(p.inventoryChange(inventoryRequest(p, head, 1, UINT16_MAX, InventoryOp::Transfer)).error ==
              InventoryError::Full,
          "Transfer from a new equipment position rejects a full recipient bag atomically");
    for (const auto slot : {EquipmentSlot::Feet, EquipmentSlot::MainHand, EquipmentSlot::Body})
        check(p.inventoryChange(inventoryRequest(p, head, 0, unsigned(slot))).error ==
                  InventoryError::Equipment,
              "Wrong wearing location rejects atomically, including swaps");
    const auto potion = findInventoryItem(p, 0, 10);
    for (const auto slot : slots)
        check(p.inventoryChange(inventoryRequest(p, potion, 0, unsigned(slot))).error ==
                  InventoryError::Equipment,
              "Existing consumables cannot occupy any new slot");
    check(quantities(p) == before, "Rejected wearable swaps preserve all ownership and derived weight");
    auto saved = p.checkpoint();
    Party restored(cat, saved);
    check(!restored.inventory().stack(head) && quantities(restored) == before &&
              restored.equipped({0}, EquipmentSlot::Wrists)->wearSlot == rules::WearSlot::Wrists,
          "Checkpoint restores last equipment position and issues fresh handle identity");
    std::swap(saved.inventory.locations[0][unsigned(EquipmentSlot::Head)],
              saved.inventory.locations[0][unsigned(EquipmentSlot::Feet)]);
    bool rejected = false;
    try {
        Party invalid(cat, saved);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "Saved equipment with mismatched wear locations is rejected");
    check(bool(p.releaseItem(p.inventory().at({{0}, 47}), p.inventory().revision())), "Vacate bag cell");
    check(bool(p.acquire({{0}, {21}, 1, p.inventory().revision()})),
          "Acquire replacement ring into full bag");
    const auto ring = findInventoryItem(p, 0, 21);
    const auto source = p.inventory().locate(ring);
    check(bool(p.inventoryChange(inventoryRequest(p, ring, 0, unsigned(EquipmentSlot::Ring2)))) &&
              p.inventory().stack(p.inventory().at(source))->item.index == 18,
          "Full-bag replacement returns the second ring to the vacated source cell");
    const auto revision = p.inventory().revision();
    check(p.inventoryChange(inventoryRequest(p, ring, 0, inventoryCells)).error ==
                  InventoryError::InvalidDestination &&
              p.inventory().revision() == revision,
          "One-past-last equipment location rejects without a mutation");
}
void paperdollGeometry() {
    using namespace inventoryLayout;
    check(equipmentFrame(1).x == equipmentFrame(9).x && equipmentFrame(3).x == equipmentFrame(10).x,
          "Main Hand and Ranged align vertically with their respective ring columns");
    const auto head = equipmentFrame(int(unsigned(EquipmentSlot::Head) - bagCells));
    const auto feet = equipmentFrame(int(unsigned(EquipmentSlot::Feet) - bagCells));
    check(head.x + head.width / 2 == silhouette.x + silhouette.width / 2 &&
              feet.x + feet.width / 2 == head.x + head.width / 2 && head.y + head.height <= silhouette.y &&
              feet.y >= silhouette.y + silhouette.height,
          "Head is centered above the doll and Feet is centered below it");
    check(equipment(int(unsigned(EquipmentSlot::Ring1) - bagCells)).y ==
              equipment(int(unsigned(EquipmentSlot::Ring2) - bagCells)).y,
          "Ring positions share a horizontal baseline");
    const auto orientation = [](ArtPoint a, ArtPoint b, ArtPoint c) {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    };
    const auto on = [](ArtPoint a, ArtPoint b, ArtPoint c) {
        return c.x >= std::min(a.x, b.x) && c.x <= std::max(a.x, b.x) && c.y >= std::min(a.y, b.y) &&
               c.y <= std::max(a.y, b.y);
    };
    const auto intersects = [&](ArtPoint a, ArtPoint b, ArtPoint c, ArtPoint d) {
        const auto abC = orientation(a, b, c), abD = orientation(a, b, d);
        const auto cdA = orientation(c, d, a), cdB = orientation(c, d, b);
        return (abC * abD < 0 && cdA * cdB < 0) || (abC == 0 && on(a, b, c)) || (abD == 0 && on(a, b, d)) ||
               (cdA == 0 && on(c, d, a)) || (cdB == 0 && on(c, d, b));
    };
    for (unsigned n = 0; n < equipmentSlots; ++n) {
        const auto frame = equipmentFrame(int(n)), label = equipmentLabel(int(n)), icon = equipment(int(n));
        check(label.x > frame.x && label.y > frame.y && label.x + label.width < frame.x + frame.width &&
                  label.y + label.height <= icon.y && icon.y + icon.height < frame.y + frame.height,
              "Each caption and item square belongs inside one contiguous card");
        for (unsigned m = 0; m < n; ++m) {
            const auto other = equipmentFrame(int(m));
            check(frame.x + frame.width < other.x || other.x + other.width < frame.x ||
                      frame.y + frame.height < other.y || other.y + other.height < frame.y,
                  "Grouped equipment cards have visible separation");
        }
    }
    for (unsigned n = 0; n < equipmentLeaders.size(); ++n) {
        const auto& a = equipmentLeaders[n];
        const std::array<ArtPoint, 3> path{a.start, a.elbow, a.target};
        for (unsigned m = 0; m < n; ++m) {
            const auto& b = equipmentLeaders[m];
            if (a.slot == b.slot)
                continue; // Feet intentionally shares a fork to both shoes.
            const std::array<ArtPoint, 3> other{b.start, b.elbow, b.target};
            for (unsigned i = 0; i < 2; ++i)
                for (unsigned j = 0; j < 2; ++j)
                    check(!intersects(path[i], path[i + 1], other[j], other[j + 1]),
                          "Independent paperdoll connectors never cross or touch");
        }
        for (unsigned slot = 0; slot < equipmentSlots; ++slot) {
            if (int(slot) == a.slot)
                continue;
            const auto r = equipmentFrame(int(slot));
            const std::array<ArtPoint, 5> edges{{{float(r.x), float(r.y)},
                                                 {float(r.x + r.width), float(r.y)},
                                                 {float(r.x + r.width), float(r.y + r.height)},
                                                 {float(r.x), float(r.y + r.height)},
                                                 {float(r.x), float(r.y)}}};
            for (unsigned i = 0; i < 2; ++i) {
                check(!r.contains(path[i]), "Connector never enters another slot card");
                for (unsigned j = 0; j < 4; ++j)
                    check(!intersects(path[i], path[i + 1], edges[j], edges[j + 1]),
                          "Connector never crosses another slot caption/card border");
            }
        }
    }
}
void wearableInput() {
    const auto cat = inventoryCatalog(wearableFixture());
    const auto map = hybridRoom();
    const Player spawn{{6, 8}, Heading::North};
    const NpcWorld empty(map, spawn.cell, {}, {});
    GameSession s(map, cat, empty, 5, spawn, false);
    CommandController ui(s);
    ui.refresh({}, false, false, true);
    ui.openInventory(rules::CharacterId{0});
    const auto press = [&](UiKey key) {
        UiInput in;
        in.pressed[unsigned(key)] = true;
        routeHudInput(ui, in);
    };
    for (unsigned n = 0; n < equipmentSlots; ++n) {
        const auto r = inventoryLayout::equipment(int(n));
        UiInput in;
        in.pointerTracked = true;
        in.pointerHeld = true;
        in.pointer = ArtPoint{float(r.x + r.width / 2), float(r.y + r.height / 2)};
        in.pressed[unsigned(UiKey::Click)] = true;
        routeHudInput(ui, in);
        check(ui.snapshot().inventory.cursor == bagCells + n, "Every compact equipment box is selectable");
        in.pressed.fill(false);
        in.pointerHeld = false;
        in.pointerReleased = true;
        routeHudInput(ui, in);
        const auto label = inventoryLayout::equipmentLabel(int(n));
        ui.inventorySelect(0);
        in.pointerTracked = true;
        in.pointerHeld = true;
        in.pointerReleased = false;
        in.pressed[unsigned(UiKey::Click)] = true;
        in.pointer = ArtPoint{float(label.x + label.width / 2), float(label.y + label.height / 2)};
        routeHudInput(ui, in);
        check(ui.snapshot().inventory.cursor == bagCells + n, "Attached captions select their own slot");
        in.pressed.fill(false);
        in.pointerHeld = false;
        in.pointerReleased = true;
        routeHudInput(ui, in);
        check(r.width == 24 && r.height == 24, "Compact targets occupy 72 native pixels");
        for (unsigned m = 0; m < n; ++m) {
            const auto other = inventoryLayout::equipment(int(m));
            check(r.x + r.width <= other.x || other.x + other.width <= r.x || r.y + r.height <= other.y ||
                      other.y + other.height <= r.y,
                  "Equipment pointer targets never overlap");
        }
    }
    ui.inventorySelect(0);
    press(UiKey::Tab);
    check(ui.snapshot().inventory.cursor == unsigned(EquipmentSlot::Head), "Tab enters top-left equipment");
    press(UiKey::Right);
    check(ui.snapshot().inventory.cursor == unsigned(EquipmentSlot::Amulet), "Right reaches upper side slot");
    press(UiKey::Down);
    check(ui.snapshot().inventory.cursor == unsigned(EquipmentSlot::Cloak), "Down follows visual column");
    for (const auto key : {UiKey::Down, UiKey::Down})
        press(key);
    check(ui.snapshot().inventory.cursor == unsigned(EquipmentSlot::Ring2), "Keyboard reaches second ring");
    press(UiKey::Down);
    check(ui.snapshot().inventory.cursor == unsigned(EquipmentSlot::Ranged), "Down enters weapon row");
    press(UiKey::Left);
    check(ui.snapshot().inventory.cursor == unsigned(EquipmentSlot::OffHand), "Left traverses weapon row");
    for (std::uint16_t definition : {std::uint16_t(17), std::uint16_t(18), std::uint16_t(21)}) {
        check(bool(s.party().acquire({{0}, {definition}, 1, s.party().inventory().revision()})),
              "Grant ring fixture");
        ui.refresh({}, false, false, true);
        const auto ring = findInventoryItem(s.party(), 0, definition);
        ui.inventorySelect(s.party().inventory().locate(ring).cell);
        if (definition == 18)
            check(!compareItem(s.party(), {0}, *cat.item({definition})).equipped,
                  "Ring comparison uses the same empty second position as Equip");
        allocations = 0;
        watching = true;
        ui.inventoryAction(InventoryAction::Equip);
        ui.refresh({}, false, false, true);
        watching = false;
        check(allocations == 0, "First and repeated wearable equips/HUD refresh allocate no C++ storage");
        const auto expected = definition == 18 ? EquipmentSlot::Ring2 : EquipmentSlot::Ring1;
        check(s.party().inventory().at({{0}, static_cast<std::uint16_t>(expected)}) == ring,
              "Equip fills empty ring positions before deterministically replacing Ring 1");
    }
}
void physicalEquipmentInput() {
    const auto cat = blobber::test::withMagicItems(inventoryCatalog(wearableFixture()));
    const auto map = hybridRoom();
    const Player spawn{{6, 8}, Heading::North};
    const NpcWorld empty(map, spawn.cell, {}, {});
    GameSession s(map, cat, empty, 5, spawn, false);
    CommandController ui(s);
    ui.refresh({}, false, false, true);
    ui.openInventory(rules::CharacterId{0});
    const auto physical = [&](UiRect r, bool press, bool held, bool release) {
        UiInput in;
        in.pointerTracked = true;
        in.pointer = ArtPoint{float(r.x + r.width / 2), float(r.y + r.height / 2)};
        in.pointerHeld = held;
        in.pointerReleased = release;
        in.pressed[unsigned(UiKey::Click)] = press;
        routeHudInput(ui, in);
    };
    const auto drag = [&](UiRect from, UiRect to) {
        physical(from, true, true, false);
        check(ui.snapshot().inventory.pointerDown, "Physical card/bag press captures an item");
        const auto before = s.party().inventory().revision();
        physical(to, false, true, false);
        check(ui.snapshot().inventory.dragging && s.party().inventory().revision() == before,
              "Physical drag stages without changing inventory before release");
        physical(to, false, false, true);
    };
    for (unsigned n = 0; n < equipmentSlots; ++n) {
        const auto slot = std::uint16_t(bagCells + n);
        auto handle = s.party().inventory().at({{0}, slot});
        if (!handle) {
            const auto definition = std::uint16_t(n == 3 ? 3 : 12 + n - 4);
            check(bool(s.party().acquire({{0}, {definition}, 1, s.party().inventory().revision()})),
                  "Acquire owned item for an empty equipment position");
            handle = findInventoryItem(s.party(), 0, definition);
        }
        std::uint16_t target = 0;
        while (target < bagCells && s.party().inventory().at({{0}, target}))
            ++target;
        check(target < bagCells, "Equipment regression has an empty destination");
        ui.refresh({}, false, false, true);
        const auto frame = inventoryLayout::equipmentFrame(int(n));
        if (s.party().inventory().locate(handle).cell >= bagCells)
            drag(frame, inventoryLayout::cell(target));
        const auto source = s.party().inventory().locate(handle).cell;
        check(source < bagCells, "Physical unequip returns the original equipped item to the bag");
        drag(inventoryLayout::cell(source), frame);
        check(s.party().inventory().at({{0}, slot}) == handle, "Physical drop equips every position");
        drag(inventoryLayout::equipmentLabel(int(n)), inventoryLayout::cell(source));
        check(s.party().inventory().at({{0}, source}) == handle && !s.party().inventory().at({{0}, slot}),
              "Dragging the attached caption unequips every position into its bag");
        check(!hudButtonTargets(ui.snapshot()).overflow, "All equipment controls fit bounded router storage");
    }
    const auto beltId = cat.findItem("belt_of_vigor");
    check(bool(s.party().acquire({{0}, beltId, 1, s.party().inventory().revision()})), "Acquire vigor belt");
    const auto belt = findInventoryItem(s.party(), 0, beltId.index);
    const auto bag = s.party().inventory().locate(belt).cell;
    const auto frame = inventoryLayout::equipmentFrame(int(unsigned(EquipmentSlot::Belt) - bagCells));
    const auto hp = s.party().character({0})->stats.maxHitPoints;
    ui.refresh({}, false, false, true);
    drag(inventoryLayout::cell(bag), frame);
    check(s.party().character({0})->stats.maxHitPoints == hp + 3, "Physical belt equip adds its HP capacity");
    s.party().heal({0}, 100);
    drag(frame, inventoryLayout::cell(bag));
    check(s.party().character({0})->stats.maxHitPoints == hp && s.party().character({0})->hitPoints == hp,
          "Physical vigor belt removal reconciles the HP cap and clamps current HP");
    drag(inventoryLayout::cell(bag), frame);
    const auto revision = s.party().inventory().revision();
    physical(frame, true, true, false);
    physical(inventoryLayout::cell(bag), false, true, false);
    UiInput cancel;
    cancel.pressed[unsigned(UiKey::Cancel)] = true;
    routeHudInput(ui, cancel);
    physical(inventoryLayout::cell(bag), false, false, true);
    check(s.party().inventory().revision() == revision && !ui.snapshot().inventory.dragging,
          "Cancel followed by physical release cannot unequip a captured belt");
    while (s.party().acquire({{0}, {21}, 1, s.party().inventory().revision()})) {
    }
    ui.refresh({}, false, false, true);
    ui.inventorySelect(unsigned(EquipmentSlot::Belt));
    ui.inventoryAction(InventoryAction::Equip);
    check(s.party().inventory().at({{0}, unsigned(EquipmentSlot::Belt)}) == belt &&
              !ui.snapshot().inventory.actions[unsigned(InventoryAction::Equip)].enabled,
          "A full bag disables Unequip and preserves the belt");

    const NpcWorld enemies(map, spawn.cell, {"goblin"}, {{{0}, {6, 7}, {0, 0}, Heading::South}});
    auto danger = std::make_unique<GameSession>(map, cat, enemies, 5, spawn, false);
    check(bool(danger->party().acquire({{0}, beltId, 1, danger->party().inventory().revision()})),
          "Acquire unsafe-removal fixture");
    const auto worn = findInventoryItem(danger->party(), 0, beltId.index);
    check(bool(danger->party().inventoryChange(inventoryRequest(danger->party(), worn, 0,
                                                               unsigned(EquipmentSlot::Belt)))),
          "Prepare equipped belt before testing session safety");
    CommandController locked(*danger);
    locked.refresh({}, false, false, true);
    locked.openInventory(rules::CharacterId{0});
    locked.inventorySelect(unsigned(EquipmentSlot::Belt));
    locked.inventoryAction(InventoryAction::Equip);
    check(danger->party().inventory().at({{0}, unsigned(EquipmentSlot::Belt)}) == worn &&
              !locked.snapshot().inventory.actions[unsigned(InventoryAction::Equip)].enabled,
          "Nearby danger still blocks belt removal through the normal action");
}
void crossbowAmmunition() {
    using namespace rules;
    auto f = inventoryFixture();
    ItemDefinition bolt;
    bolt.id = "fixture.bolt";
    bolt.name = "Bolt";
    bolt.description = "Owned crossbow ammunition";
    bolt.kind = ItemKind::Ammunition;
    bolt.ammunition = Ammunition::Bolt;
    const ItemId boltId{std::uint16_t(f.items.size())};
    f.items.push_back(bolt);
    auto crossbow = f.items[3];
    crossbow.id = "fixture.crossbow";
    crossbow.name = "Crossbow";
    crossbow.weapon = WeaponType::LightCrossbow;
    crossbow.ammunition = Ammunition::Bolt;
    crossbow.loading = true;
    crossbow.damage = {1, 8, 0};
    const ItemId crossbowId{std::uint16_t(f.items.size())};
    f.items.push_back(crossbow);
    auto& rogue = f.preset.characters[2];
    rogue.items[2].item = crossbowId;
    const auto cat = inventoryCatalog(f);
    const auto map = hybridRoom();
    const Player pose{{6, 8}, Heading::North};
    NpcWorld placements(map, pose.cell, {"goblin"}, {{{0}, {6, 4}, {0, 0}, Heading::North}});
    GameSession game(map, cat, placements, 9, pose, false);
    auto shot = turnCommand(game, PartyAction::Ranged);
    shot.character = {2};
    shot.target = game.actors().id(0);
    check(game.submit(shot).reason == CommandReason::NoAmmunition, "Arrows cannot substitute for bolts");
    check(bool(game.party().acquire({{2}, boltId, 1, game.party().inventory().revision()})),
          "Acquire one owned bolt");
    shot = turnCommand(game, PartyAction::Ranged);
    shot.character = {2};
    shot.target = game.actors().id(0);
    const auto arrows = game.party().inventory().count({2}, Ammunition::Arrow, cat);
    const auto outcome = game.submit(shot);
    check(outcome.reason == CommandReason::Resolved &&
              game.party().inventory().count({2}, Ammunition::Bolt, cat) == 0 &&
              game.party().inventory().count({2}, Ammunition::Arrow, cat) == arrows,
          "Crossbow consumes its bolt at acceptance and preserves arrows");
    check(game.submit(shot).reason == CommandReason::StaleTurn,
          "Repeated crossbow request cannot create another shot");
}
void magicEquipment() {
    using namespace rules;
    const auto cat = blobber::test::withMagicItems(inventoryCatalog());
    Party party(cat);
    const CharacterId wizard{3};
    const auto baseline = party.character(wizard)->stats;
    const int mana = party.character(wizard)->mana;
    constexpr const char* names[]{"circlet_of_focus",   "gloves_of_striking",  "boots_of_balance",
                                  "belt_of_vigor",      "cloak_of_protection", "ring_of_resolve",
                                  "amulet_of_reserves", "apprentice_bracers"};
    for (const auto* name : names) {
        const auto id = cat.findItem(name);
        check(bool(party.acquire({wizard, id, 1, party.inventory().revision()})),
              "Acquire dedicated magic fixture");
        const auto handle = findInventoryItem(party, 3, id.index);
        const auto slot = rules::itemEquipmentPosition(*cat.item(id));
        allocations = 0;
        watching = true;
        const auto changed = party.inventoryChange(inventoryRequest(party, handle, 3, unsigned(slot)));
        watching = false;
        check(bool(changed) && allocations == 0,
              "Every magic equip succeeds allocation free, including fourth through eighth");
    }
    auto current = party.character(wizard);
    check(current->stats.armorClass == baseline.armorClass + 2 && current->stats.spellAttackBonus == 1 &&
              current->stats.initiative == baseline.initiative + 1,
          "Conditional bracers, cloak, circlet and boots compose");
    for (unsigned i = 0; i < 6; ++i)
        check(current->stats.saves[i] == baseline.saves[i] + 2 + (i == 1),
              "Ring/cloak/boots save bonuses compose");
    check(current->hitPoints == baseline.maxHitPoints &&
              current->stats.maxHitPoints == baseline.maxHitPoints + 3 && current->mana == mana &&
              current->maxMana == mana + 1,
          "Increasing equipment capacity never fills HP or mana");
    check(current->stats.weapons[unsigned(EquipmentSlot::MainHand)].damage.modifier ==
              baseline.weapons[unsigned(EquipmentSlot::MainHand)].damage.modifier + 1,
          "Gloves add one flat melee weapon damage");
    party.heal(wizard, 100);
    party.restoreMana(wizard, 100);
    const auto saved = party.checkpoint();
    Party restored(cat, saved);
    check(restored.character(wizard)->mana == mana + 1 &&
              restored.character(wizard)->stats.armorClass == current->stats.armorClass,
          "Restoration derives wearable stats before validating resource caps");
    for (const auto* name : {"belt_of_vigor", "amulet_of_reserves"}) {
        const auto h = findInventoryItem(party, 3, cat.findItem(name).index);
        check(bool(party.inventoryChange(inventoryRequest(party, h, 3, 47))), "Unequip capacity item");
        check(bool(party.inventoryChange(inventoryRequest(
                  party, h, 3, unsigned(rules::itemEquipmentPosition(*cat.item(cat.findItem(name))))))),
              "Reequip capacity item");
    }
    check(party.character(wizard)->hitPoints == baseline.maxHitPoints &&
              party.character(wizard)->mana == mana,
          "Capacity removal clamps and swapping cannot generate resources");
    const auto offered = compareItem(party, wizard, *cat.item(cat.findItem("cloak_of_protection")));
    check(offered.offeredStats.armorClass == party.character(wizard)->stats.armorClass,
          "Comparison derives the same bonuses as actual equipment");
    const auto belt = findInventoryItem(party, 3, cat.findItem("belt_of_vigor").index);
    const auto stale = inventoryRequest(party, belt, 3, 46);
    party.acquire({wizard, cat.findItem("dagger_plus1"), 1, party.inventory().revision()});
    check(!party.inventoryChange(stale), "Stale magic equip preserves resources");
    const auto enchanted = findInventoryItem(party, 3, cat.findItem("dagger_plus1").index);
    check(
        bool(party.inventoryChange(inventoryRequest(party, enchanted, 3, unsigned(EquipmentSlot::MainHand)))),
        "Equip enchanted weapon");
    const auto& weapon = party.character(wizard)->stats.weapons[unsigned(EquipmentSlot::MainHand)];
    check(weapon.attackBonus == baseline.weapons[unsigned(EquipmentSlot::MainHand)].attackBonus + 1 &&
              weapon.damage.modifier ==
                  baseline.weapons[unsigned(EquipmentSlot::MainHand)].damage.modifier + 2,
          "Weapon enchantment and gloves combine without changing spell modifiers");
    auto f = inventoryFixture();
    f.races[2].size = CreatureSize::Small;
    auto heavy = f.items[0];
    heavy.id = "fixture.heavy";
    heavy.weapon = WeaponType::Greatsword;
    heavy.damage = {2, 6, 0};
    heavy.versatileSides = 0;
    heavy.twoHanded = heavy.heavy = true;
    f.items.push_back(heavy);
    const auto heavyCat = inventoryCatalog(f);
    Party heavyParty(heavyCat);
    const ItemId heavyId{std::uint16_t(f.items.size() - 1)};
    check(bool(heavyParty.acquire({{2}, heavyId, 1, heavyParty.inventory().revision()})),
          "Acquire heavy weapon");
    auto handle = findInventoryItem(heavyParty, 2, heavyId.index);
    check(bool(heavyParty.inventoryChange(
              inventoryRequest(heavyParty, handle, 2, unsigned(EquipmentSlot::MainHand)))),
          "Small characters can equip heavy weapons");
    check(heavyParty.character({2})->stats.weapons[unsigned(EquipmentSlot::MainHand)].heavyDisadvantage,
          "Heavy weapon records Small wielder disadvantage without rejecting equipment");
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
        inventoryToggle();
        contextualActions();
        characterSheet();
        wearableEquipment();
        magicEquipment();
        crossbowAmmunition();
        paperdollGeometry();
        wearableInput();
        physicalEquipmentInput();
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

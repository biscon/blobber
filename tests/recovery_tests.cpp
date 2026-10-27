#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "support/recovery_fixtures.hpp"
#include "ui/interaction.hpp"
#include <iostream>
#include <memory>
using namespace blobber;
namespace {
const Player pose{{3, 3}, Heading::North};
void rests() {
    const auto cat = recoveryCatalog();
    const auto map = combatMap();
    const auto empty = combatPlacements(map, pose, 0);
    auto owner = std::make_unique<GameSession>(map, cat, empty, 7, pose, false);
    auto& s = *owner;
    check(s.previewRest(restRequest(s)).error == RestError::NoBenefit, "Reject no-op long rest");
    s.party().damage({0}, 6);
    s.party().spendMana({3}, 1);
    s.party().useFeature({0}, rules::Feature::SecondWind);
    s.party().useFeature({3}, rules::Feature::ArcaneRecovery);
    auto shortRest = restRequest(s, RestKind::Short);
    shortRest.dice[0] = 1;
    const auto hp = s.party().character({0})->hitPoints, mana = s.party().character({3})->mana;
    const auto time = s.timeMs();
    const auto before = s.combat().snapshot().gameplayMs;
    const auto out = s.rest(shortRest);
    check(bool(out) && out.diceSpent[0] == 1 && out.healed[0] > 0,
          "Short rest spends selected die and heals");
    check(s.party().character({0})->hitPoints > hp && s.party().character({0})->availableHitDice == 0,
          "Bounded health and die use");
    check(s.party().character({3})->mana == mana && !s.party().character({3})->arcaneRecovery &&
              s.party().character({0})->secondWind,
          "Short rest refreshes only Second Wind");
    check(s.timeMs() == time && s.combat().snapshot().gameplayMs == before + 3600000,
          "Separate presentation/gameplay clocks");
    check(s.rest(shortRest).error == RestError::Stale && inventoryQuantity(s.party(), 0, 13) == 2,
          "Repeated request cannot apply twice");
    auto invalid = restRequest(s, RestKind::Short);
    invalid.dice[0] = 1;
    check(s.rest(invalid).error == RestError::InvalidDice, "Spent dice cannot be reused");
    const auto longRest = restRequest(s);
    const auto preview = s.previewRest(longRest);
    check(bool(preview) && preview.kits == 2 && preview.supplier.index == 0, "Preview cost owner");
    check(bool(s.rest(longRest)), "Field long rest");
    check(inventoryQuantity(s.party(), 0, 13) == 1, "Exactly one kit for whole party");
    check(s.party().character({0})->hitPoints == s.party().character({0})->stats.maxHitPoints &&
              s.party().character({0})->availableHitDice == 1,
          "Full HP and recovered dice");
    check(s.party().character({3})->mana == s.party().character({3})->maxMana &&
              s.party().character({3})->arcaneRecovery,
          "Mana and Arcane charge restored");
    check(s.rest(longRest).error == RestError::Stale && inventoryQuantity(s.party(), 0, 13) == 1,
          "Duplicate long rest no consumption");
    check(s.rest(restRequest(s)).error == RestError::NoBenefit, "No-op after recovery");
    s.party().damage({1}, s.party().character({1})->hitPoints);
    check(s.rest(restRequest(s)).error == RestError::Dying, "Dying blocks rest");
    s.party().stabilize({1});
    s.party().damage({2}, 1000);
    check(bool(s.rest(restRequest(s))), "Stable can recover while dead stays dead");
    check(s.party().character({1})->hitPoints > 0 && s.party().character({2})->life == rules::Life::Dead,
          "Recovery never resurrects");
    s.party().damage({0}, 1);
    check(s.rest(restRequest(s)).error == RestError::NoSupplies, "Supplies exhaust");
    auto foreign = restRequest(s);
    ++foreign.world;
    check(s.rest(foreign).error == RestError::Stale, "Foreign rest owner");
}
void sanctuaryAndEffects() {
    const auto cat = recoveryCatalog(false);
    const auto map = combatMap();
    const auto empty = combatPlacements(map, pose, 0);
    RecoveryLevel level;
    level.sanctuaries[0] = pose.cell;
    level.sanctuaryCount = 1;
    auto owner =
        std::make_unique<GameSession>(map, cat, empty, 5, pose, false, std::vector<NpcTimings>{}, level);
    auto& s = *owner;
    s.party().damage({3}, 2);
    CombatEvents events;
    EffectRequest e;
    e.world = s.actors().snapshot().world;
    e.caster = {3};
    e.kind = EffectKind::MageArmor;
    e.targets[0] = rules::CharacterId{3};
    e.count = 1;
    check(bool(s.applyEffect(e)), "Apply timed armor");
    check(s.previewRest(restRequest(s)).sanctuary && bool(s.rest(restRequest(s))),
          "Authored sanctuary is free");
    check(!s.effects().has(rules::CharacterId{3}, EffectKind::MageArmor),
          "Eight-hour effects expire at exact deadline");
    for (std::uint16_t i = 0; i < 4; ++i) {
        s.party().damage({i}, s.party().character({i})->hitPoints);
        s.party().stabilize({i});
    }
    check(s.rest(restRequest(s)).error == RestError::NoLeader, "Camp requires able leader");
}
void clocksAndSupplier() {
    const auto cat = recoveryCatalog();
    const auto map = combatMap();
    const NpcWorld places(map, pose.cell, {"goblin"}, {{{0}, {1, 1}, {0, 0}, Heading::North}});
    auto owner = std::make_unique<GameSession>(map, cat, places, 7, pose, true);
    auto& s = *owner;
    s.party().damage({0}, 5);
    PartyCommand wind;
    wind.character = {0};
    wind.action = PartyAction::Feature;
    wind.feature = rules::Feature::SecondWind;
    wind.world = s.actors().snapshot().world;
    wind.turn = s.combat().snapshot().turn;
    check(s.submit(wind).reason == CommandReason::Resolved, "Feature establishes recovery cooldown");
    s.update(.7, false);
    check(s.previewRest(restRequest(s, RestKind::Short)).error == RestError::Unsafe,
          "Rest waits for settled presentation");
    s.update(.4, false);
    check(s.combat().snapshot().cooldownMs[0] > 0, "Fixture retains cooldown before camp");
    const auto cell = s.actors().snapshot().actors[0].npc.cell;
    check(bool(s.rest(restRequest(s, RestKind::Short))), "Feature-only short rest has benefit");
    check(s.actors().snapshot().actors[0].npc.cell == cell && s.combat().snapshot().cooldownMs[0] == 0,
          "Rest expires cooldown without AI catch-up");
    s.party().damage({0}, 1000);
    s.party().damage({1}, 1);
    check(s.previewRest(restRequest(s)).supplier.index == 0 && bool(s.rest(restRequest(s))),
          "Supplies may come from dead companion bag");
    check(s.party().character({0})->life == rules::Life::Dead && inventoryQuantity(s.party(), 0, 13) == 1,
          "Cost consumed without resurrection");
}
void safetyAndUi() {
    const auto cat = recoveryCatalog();
    const auto map = combatMap();
    const auto nearby = combatPlacements(map, pose, 1);
    auto unsafe = std::make_unique<GameSession>(map, cat, nearby, 7, pose, false);
    unsafe->party().damage({0}, 2);
    check(unsafe->rest(restRequest(*unsafe)).error == RestError::Unsafe, "Adjacent threat prevents camp");
    const auto empty = combatPlacements(map, pose, 0);
    auto owner = std::make_unique<GameSession>(map, cat, empty, 7, pose, false);
    auto& s = *owner;
    s.party().damage({0}, 2);
    CommandController ui(s);
    ui.refresh({}, false, false, true);
    ui.openCamp();
    ui.campDice(0, 1);
    const auto initial = s.combat().snapshot().gameplayMs;
    UiInput input;
    input.focused = true;
    input.pressed[std::size_t(UiKey::Cancel)] = true;
    routeHudInput(ui, input, {});
    check(!ui.snapshot().camp.open && s.combat().snapshot().gameplayMs == initial,
          "Cancel preview has no side effects");
    ui.openCamp();
    ui.campDice(0, 1);
    ui.startRest();
    const auto committed = s.combat().snapshot().gameplayMs;
    check(committed == initial + 3600000, "UI commits rest once");
    ui.startRest();
    check(s.combat().snapshot().gameplayMs == committed, "Playing cannot retrigger");
    ui.tickCamp(.1, false);
    check(ui.snapshot().camp.elapsedMs == 0, "Focus loss pauses cosmetics");
    routeHudInput(ui, input, {});
    check(!ui.snapshot().camp.open && s.combat().snapshot().gameplayMs == committed,
          "Escape skips without rollback");
    s.setAnimated(true);
    s.move(Action::Back);
    check(s.previewRest(restRequest(s)).error == RestError::Moving, "Motion prevents recovery");
}
void lootAndPickup() {
    const auto cat = recoveryCatalog();
    const auto map = combatMap();
    const auto actors = combatPlacements(map, pose, 2);
    auto owner = std::make_unique<GameSession>(map, cat, actors, 19, pose, false);
    auto& s = *owner;
    const auto world = s.actors().snapshot().world;
    s.actors().damage(s.actors().id(0), 100, 0);
    s.actors().remove(s.actors().id(0));
    s.actors().remove(s.actors().id(1));
    s.update(0, false);
    s.combat().continueExploring();
    check(s.loot().find({world, 0}) && s.loot().find({world, 1}), "Death survives immediate removal");
    check(!s.loot().find({world, 8}), "Removing living actor gives no reward");
    const auto gold = s.loot().find({world, 1})->quantity;
    check(gold >= 1 && gold <= 6, "Gold roll range");
    const auto revision = s.party().inventory().revision();
    check(s.pickup({{world + 1, 0}, {0}, revision}).error == InventoryError::Stale, "Foreign floor handle");
    check(bool(s.pickup({{world, 1}, {3}, revision}, true)) && s.loot().find({world, 1}),
          "Preview does not take ownership");
    check(bool(s.pickup({{world, 1}, {3}, revision})), "Acquire for explicit recipient");
    check(inventoryQuantity(s.party(), 3, 12) == gold && !s.loot().find({world, 1}),
          "Ownership transfers atomically");
    check(s.pickup({{world, 1}, {0}, s.party().inventory().revision()}).error == InventoryError::Stale,
          "Double pickup fails");
    check(s.pickup({{world, 0}, {0}, revision}).error == InventoryError::Stale && s.loot().find({world, 0}),
          "Stale inventory leaves floor unchanged");
    s.update(0, true);
    check(!s.loot().find({world, 1}), "Reconcile cannot respawn collected drops");
    auto second = std::make_unique<GameSession>(map, cat, actors, 19, pose, false);
    second->actors().damage(second->actors().id(1), 100, 0);
    second->actors().damage(second->actors().id(0), 100, 0);
    second->update(0, true);
    check(second->loot().snapshot().stacks[1].quantity == gold, "Kill ordering independent RNG");
    bool healing = false, mana = false, none = false;
    for (unsigned seed = 0; seed < 100; ++seed) {
        LootWorld rolls(second->actors(), {}, seed);
        rolls.reconcile(second->actors());
        const auto& drops = rolls.snapshot().stacks;
        check(!(drops[2].active && drops[3].active), "At most one optional potion");
        healing |= drops[2].active;
        mana |= drops[3].active;
        none |= !drops[2].active && !drops[3].active;
    }
    check(healing && mana && none, "Seeded outcomes include all potion categories");
    CommandController ui(s);
    ui.refresh({}, false, false, true);
    ui.selectCharacter(1);
    ui.openInventory(rules::CharacterId{3});
    ui.closeOverlay();
    ui.collectLoot({world, 0});
    check(inventoryQuantity(s.party(), 1, 0) == 1 && inventoryQuantity(s.party(), 3, 0) == 0,
          "Viewed sheet character never replaces selected floor recipient");
}
void capacityStress() {
    const auto cat = recoveryCatalog(false, true);
    std::vector<std::string> rows(12, std::string(12, '.'));
    rows.front() = rows.back() = std::string(12, '#');
    for (auto& row : rows)
        row.front() = row.back() = '#';
    const Map map(rows);
    std::vector<NpcInstance> instances;
    RecoveryLevel level;
    for (int y = 2; y < 10; ++y)
        for (int x = 2; x < 10; ++x) {
            level.ground[level.groundCount++] = {{x, y}, {12}, 1};
            for (int sub = 0; sub < 4; ++sub)
                instances.push_back({{0}, {x, y}, {sub % 2, sub / 2}, Heading::North});
        }
    const NpcWorld placements(map, {1, 1}, {"goblin"}, instances);
    ActorWorld actors(map, cat, placements);
    LootWorld loot(actors, level, 5);
    for (std::size_t i = 0; i < instances.size(); ++i) {
        actors.damage(actors.id(i), 100, 0);
        if (i % 2)
            actors.remove(actors.id(i));
    }
    loot.reconcile(actors);
    unsigned count = 0;
    for (const auto& stack : loot.snapshot().stacks)
        count += stack.active;
    check(count == lootCapacity, "All 256 deaths plus 64 placed stacks publish without exhaustion");
    loot.reconcile(actors);
    for (std::uint16_t i = 0; i < lootCapacity; ++i)
        check(loot.collect({actors.snapshot().world, i}), "Every reservation collectible once");
    loot.reconcile(actors);
    for (const auto& stack : loot.snapshot().stacks)
        check(!stack.active, "Capacity stress never respawns collected loot");
}
void acquisitionFailures() {
    auto f = inventoryFixture();
    rules::ItemDefinition token;
    token.id = "token";
    token.name = token.description = "Token";
    token.kind = rules::ItemKind::Gear;
    f.items.push_back(token);
    auto& build = f.preset.characters[3];
    while (build.itemCount < build.items.size())
        build.items[build.itemCount++] = {{12}, 1};
    const auto cat = inventoryCatalog(f);
    Party p(cat);
    // Fill every remaining bag cell through the same atomic API.
    while (p.acquire({{3}, {12}, 1, p.inventory().revision()})) {
    }
    auto rev = p.inventory().revision();
    const auto q = inventoryQuantity(p, 3, 10);
    check(!p.acquire({{3}, {10}, 20, rev}), "Partial merge plus no new slot fails");
    check(p.inventory().revision() == rev && inventoryQuantity(p, 3, 10) == q,
          "Failed merge rolls back completely");
    Party light(cat);
    for (int i = 0; i < 20; ++i) {
        const auto before = light.inventory().revision();
        const auto count = inventoryQuantity(light, 3, 7);
        if (!light.acquire({{3}, {7}, 1, before})) {
            check(before == light.inventory().revision() && count == inventoryQuantity(light, 3, 7),
                  "Weight failure has no partial mutation");
            return;
        }
    }
    check(false, "Fixture must reach capacity weight");
}
} // namespace
int main(int argc, char**) {
    try {
        if (argc > 1) {
            capacityStress();
            std::cout << "Loot capacity stress passed\n";
            return 0;
        }
        rests();
        sanctuaryAndEffects();
        clocksAndSupplier();
        safetyAndUi();
        lootAndPickup();
        acquisitionFailures();
        std::cout << "Recovery and loot checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

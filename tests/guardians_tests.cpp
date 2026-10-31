#include "game/commands.hpp"
#include "game/perception.hpp"
#include "generation/population.hpp"
#include "support/automation_fixtures.hpp"
#include <iostream>
using namespace blobber;
using namespace blobber::generation;
using namespace blobber::automation::test;
namespace {
unsigned active(const GameSession& g) {
    unsigned n = 0;
    for (const auto& s : g.loot().snapshot().stacks)
        n += s.active;
    return n;
}
void objective() {
    const auto cat = catalog(false, 50, 7);
    const auto defs = definitions();
    const auto floor = generate({5, 2});
    check(bool(floor), "generated guardian fixture");
    const auto& f = *floor.floor;
    const auto p = preparePopulation(f, cat, defs.types);
    GameSession g(f.map, cat, p.placements, f.runtimeSeed, f.layout.spawn, true, defs.timings, p.recovery,
                  f.layout.fixtures, p.objective);
    check(g.objective().count == 2 && !g.objective().completed && active(g) == 2,
          "only optional supplies start active");
    check(g.landingSafety(0).safe && g.landingSafety(1).safe && !g.landingSafety(2).safe,
          "both initially protected landings are safe");
    const auto gate = f.layout.fixtures.entries[f.layout.guardianGate].cell;
    check(g.geometry().solid(gate), "sealed gate is effective geometry");
    const auto filler = cat.findItem("holy_symbol");
    for (std::uint16_t c = 0; c < rules::partySize; ++c) {
        while (g.party().acquire({{c}, filler, 1, g.party().inventory().revision()})) {
        }
        for (std::uint16_t b = 0; b < bagCells; ++b)
            check(bool(g.party().inventory().stack(g.party().inventory().at({{c}, b}))),
                  "owned bags filled before guardian rewards");
    }
    const auto first = g.actors().id(2), second = g.actors().id(3);
    check(g.guardian(first) && !g.guardian(g.actors().id(0)) && !g.guardian({first.slot, first.world + 1}),
          "designation rejects foreign actors");
    g.actors().damage(g.actors().id(0), 1000, 0);
    g.update(0, false);
    check(!g.objective().completed && g.geometry().solid(gate), "unrelated death cannot open gate");
    g.actors().damage(first, 1000, 0);
    g.actors().remove(first);
    g.update(0, false);
    check(!g.objective().completed, "partial designated deaths keep gate closed, even after corpse removal");
    const auto revision = g.fixtures().geometryRevision();
    const auto before = active(g);
    g.actors().damage(second, 1000, 0);
    g.update(0, false);
    check(g.objective().completed && g.objective().rewardIssued && !g.geometry().solid(gate),
          "all authoritative deaths open gate and issue reward");
    check(g.fixtures().geometryRevision() == revision + 1 && active(g) == before + 3,
          "one gate revision, one corpse stack and two reserved supplies");
    const auto loot = g.loot().snapshot().revision;
    const auto xp = g.party().character({0})->experience;
    for (unsigned i = 0; i < 10; ++i)
        g.update(0, false);
    check(g.loot().snapshot().revision == loot && g.party().character({0})->experience == xp,
          "reconciliation cannot duplicate loot or XP");
    GameSession removed(f.map, cat, p.placements, 5, f.layout.spawn, false, {}, p.recovery, f.layout.fixtures,
                        p.objective);
    removed.actors().remove(removed.actors().id(2));
    removed.actors().damage(removed.actors().id(3), 1000, 0);
    removed.update(0, false);
    check(!removed.objective().completed && removed.geometry().solid(gate),
          "removing a live guardian never counts as death");
    GameSession exhausted(f.map, cat, p.placements, 5, f.layout.spawn, false, {}, p.recovery,
                          f.layout.fixtures, p.objective);
    auto& exhaustedLoot = const_cast<LootSnapshot&>(exhausted.loot().snapshot());
    exhaustedLoot.revision = UINT64_MAX;
    exhausted.actors().damage(exhausted.actors().id(2), 1000, 0);
    exhausted.actors().damage(exhausted.actors().id(3), 1000, 0);
    exhausted.update(0, false);
    check(exhausted.objective().capacityFailure && !exhausted.objective().completed &&
              exhausted.geometry().solid(gate) && active(exhausted) == 2 &&
              exhaustedLoot.revision == UINT64_MAX,
          "revision exhaustion cannot partially publish gate or reward or wrap loot");
    GameSession drops(f.map, cat, p.placements, 5, {f.layout.guardianReward, Heading::North}, false, {},
                      p.recovery, f.layout.fixtures, p.objective);
    for (unsigned n = 0; n < lootDropCapacity; ++n) {
        check(bool(drops.party().acquire({{0}, filler, 1, drops.party().inventory().revision()})),
              "drop fixture adds one item");
        ItemHandle handle;
        for (std::uint16_t b = 0; b < bagCells; ++b) {
            const auto h = drops.party().inventory().at({{0}, b});
            const auto* stack = drops.party().inventory().stack(h);
            if (stack && stack->item.index == filler.index) {
                handle = h;
                break;
            }
        }
        check(bool(drops.dropItem(handle, drops.party().inventory().revision())),
              "fill player-drop pool through checked transactions");
    }
    drops.actors().damage(drops.actors().id(2), 1000, 0);
    drops.actors().damage(drops.actors().id(3), 1000, 0);
    drops.update(0, false);
    check(drops.objective().completed && active(drops) == lootDropCapacity + 6,
          "full player-drop pool cannot exhaust reserved guardian rewards");
    const LootId reward{drops.actors().snapshot().world,
                        std::uint16_t(NpcWorld::maxInstances * lootPerActor + p.recovery.groundCount), 1};
    drops.update(.3, false);
    const auto beforePickup = drops.loot().snapshot().revision;
    const PickupRequest pickup{reward, {0}, drops.party().inventory().revision()};
    check(bool(drops.pickup(pickup, true)) && drops.loot().snapshot().revision == beforePickup,
          "guardian pickup preview cannot consume reward");
    check(bool(drops.pickup(pickup)) && !drops.pickup(pickup), "guardian stack can be collected only once");
    drops.update(0, false);
    check(!drops.loot().find(reward), "collected guardian reward never republishes");
    const auto approach = f.layout.fixtures.entries[f.layout.approachDoor];
    GameSession contact(f.map, cat, p.placements, 5, {f.layout.guardianApproach, approach.facing}, true,
                        defs.timings, p.recovery, f.layout.fixtures, p.objective);
    contact.setAnimated(false);
    contact.move(Action::Forward);
    const auto lockedPose = contact.movement().player;
    check(!realTime(contact.combat().snapshot().phase),
          "guardian approach enters ordinary movement-locked contact");
    contact.move(Action::Back);
    check(contact.movement().player == lockedPose, "guardian designation does not permit combat retreat");
    auto bad = p.objective;
    bad.members[1] = bad.members[0];
    bool rejected = false;
    try {
        GameSession invalid(f.map, cat, p.placements, 5, f.layout.spawn, false, {}, p.recovery,
                            f.layout.fixtures, bad);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "duplicate guardian binding rejects preparation");
    bad = p.objective;
    bad.members[0] = 255;
    rejected = false;
    try {
        GameSession invalid(f.map, cat, p.placements, 5, f.layout.spawn, false, {}, p.recovery,
                            f.layout.fixtures, bad);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "invalid guardian binding rejects preparation");
    auto full = p.recovery;
    full.groundCount = 63;
    for (auto& r : full.ground)
        r = p.recovery.ground[0];
    rejected = false;
    try {
        GameSession invalid(f.map, cat, p.placements, 5, f.layout.spawn, false, {}, full, f.layout.fixtures,
                            p.objective);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "reward reservations fail at preparation rather than at final death");
}
void landings() {
    const auto cat = catalog(false, 50, 7, true);
    const auto defs = definitions();
    for (unsigned depth = 1; depth <= 3; ++depth)
        for (bool fallback : {false, true}) {
            const auto result = generate({13, depth}, templates(), fallback);
            check(bool(result), "valid landing fixture and fallback");
            const auto& f = *result.floor;
            const auto p = preparePopulation(f, cat, defs.types);
            // Open every ordinary door through its normal command, using fresh sessions
            // at a faced legal interaction cell. Verify static exclusion even when all open.
            auto fixtures = f.layout.fixtures;
            FloorGeometry initial(f.map, fixtures);
            for (unsigned l = 0; l < fixtures.landingCount; ++l) {
                const auto landing = fixtures.landings[l];
                for (int y = 0; y < f.map.height(); ++y)
                    for (int x = 0; x < f.map.width(); ++x) {
                        const Cell c{x, y};
                        if (!f.map.solid(c) && !fixtures.npcExcluded[unsigned(y * f.map.width() + x)])
                            check(!lineOfSight(f.map, c, landing) && !combatAdjacent(f.map, c, landing),
                                  "open-door landing has no hostile sight or contact");
                    }
            }
            GameSession g(f.map, cat, p.placements, 5, f.layout.spawn, true, defs.timings, p.recovery,
                          fixtures, p.objective);
            const auto item = g.party().inventory().at({{0}, 0});
            const auto inventory = g.party().inventory().revision();
            check(!g.dropItem(item, inventory) && g.party().inventory().revision() == inventory,
                  "protected room rejects drop without consuming inventory");
            for (unsigned l = 0; l < fixtures.landingCount; ++l) {
                const auto landing = fixtures.landings[l];
                check(!g.actors().move(g.actors().id(0), landing, {0, 0}, {-1, -1}),
                      "checked actor move cannot enter protected landing");
            }
            for (unsigned i = 0; i < 100; ++i)
                g.update(.1, false);
            check(g.combat().snapshot().phase == CombatPhase::Exploration && g.landingSafety(0).safe,
                  "initial arrival remains out of contact under real AI");
            const auto doorIndex = depth == 1 ? 3u : 4u;
            const auto door = fixtures.entries[doorIndex];
            GameSession opened(f.map, cat, p.placements, 5,
                               {door.cell + forward(door.facing) * -1, door.facing}, false, defs.timings,
                               p.recovery, fixtures, p.objective);
            check(opened.interact({opened.fixtures().id(doorIndex), opened.fixtures().revision()}).changed,
                  "stairs room opens with no key");
            NpcAiController ai(opened.geometry(), opened.actors(), 5);
            for (unsigned i = 0; i < opened.actors().snapshot().count; ++i)
                ai.alert(opened.actors().id(i), fixtures.landings[0]);
            for (unsigned tick = 0; tick < 200; ++tick) {
                ai.update(100, tick * 100, {fixtures.landings[0], Heading::South});
                for (const auto& a : opened.actors().snapshot().actors)
                    if (a.maxHitPoints) {
                        check(opened.geometry().npcAllowed(a.npc.cell),
                              "pursuing and returning actors respect open-door exclusion");
                        check(!lineOfSight(opened.geometry(), a.npc.cell, fixtures.landings[0]),
                              "moving melee/ranged actors cannot acquire open-door arrival sight");
                    }
            }
            auto broken = f.layout;
            broken.fixtures.npcExcluded.reset();
            check(validateFixtures(broken).code == Error::Landing, "unprotected arrivals reject");
            broken = f.layout;
            broken.population[0].cell = broken.spawn.cell;
            check(validateFixtures(broken).code == Error::Population, "protected spawn overlap rejects");
            broken = f.layout;
            broken.fixtures.entries[broken.guardianGate].cell = broken.rooms[0].anchor;
            check(bool(validateFixtures(broken)), "off-route guardian gate rejects");
        }
}
} // namespace
int main() {
    try {
        objective();
        landings();
        std::cout << "Guardian progression and landing contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

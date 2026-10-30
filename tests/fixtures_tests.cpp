#include "game/perception.hpp"
#include "generation/floor.hpp"
#include "support/check.hpp"
#include "support/fixture_world.hpp"
#include "support/spell_fixtures.hpp"
#include "ui/interaction.hpp"
#include <iostream>
using namespace blobber;
namespace {
void interactions() {
    auto cat = fixtureCatalog();
    auto map = fixtureMap();
    NpcWorld placements(map, {2, 3}, {"goblin"}, {});
    auto owner =
        std::make_unique<GameSession>(map, cat, placements, 5, Player{{2, 3}, Heading::East}, false,
                                      std::vector<NpcTimings>{}, RecoveryLevel{}, fixtureDefinitions());
    auto& g = *owner;
    check(g.fixtures().known(0) && !g.fixtures().known(1) && !g.fixtures().known(2),
          "Facing discovery sees blocker, not side key or behind door");
    const auto hidden = g.interact(fixtureRequest(g, 1), true);
    const auto absent = g.interact({{g.fixtures().world(), 31}, 0}, true);
    check(!hidden && !absent && std::string(hidden.reason) == absent.reason,
          "Hidden and nonexistent fixture errors are identical");
    g.move(Action::Forward);
    check(!g.interact(fixtureRequest(g, 0)), "Moving cannot interact");
    g.update(.2, false);
    const auto revision = g.fixtures().revision();
    check(!g.interact(fixtureRequest(g, 0)) && g.fixtures().revision() == revision,
          "Missing key rejection is atomic");
    g.setAnimated(false);
    g.move(Action::Back);
    g.move(Action::TurnLeft);
    check(g.fixtures().known(1), "Turning reveals side key");
    for (std::uint16_t c = 0; c < 4; ++c) {
        while (g.party().acquire({{c}, {12}, 1, g.party().inventory().revision()})) {
        }
        for (std::uint16_t i = 0; i < 48; ++i)
            check(bool(g.party().inventory().stack(g.party().inventory().at({{c}, i}))),
                  "All party bags filled");
    }
    const auto inventory = g.party().inventory().revision();
    auto key = fixtureRequest(g, 1);
    check(bool(g.interact(key, true)) && !g.party().questKeys() && g.fixtures().revision() == revision,
          "Key preview is pure");
    check(g.interact(key).changed && g.party().questKeys() == 4 &&
              g.party().inventory().revision() == inventory,
          "Full inventory acquires unweighted quest key without inventory mutation");
    check(!g.interact(key), "Old revision rejected");
    check(g.interact(fixtureRequest(g, 1)).accepted && !g.interact(fixtureRequest(g, 1)).changed,
          "Key cannot be collected twice");
    g.move(Action::TurnRight);
    g.move(Action::Forward);
    auto door = fixtureRequest(g, 0);
    const auto geo = g.geometry().revision();
    check(g.geometry().solid({4, 3}) && bool(g.interact(door, true)) && g.geometry().revision() == geo,
          "Door preview preserves collision");
    check(g.interact(door).changed && !g.geometry().solid({4, 3}) && g.geometry().revision() == geo + 1,
          "Opening publishes geometry exactly once");
    check(g.fixtures().known(2) && g.fixtures().discovered({6, 3}), "Stationary opening updates discovery");
    const auto again = g.interact(fixtureRequest(g, 0));
    check(again.accepted && !again.changed && g.party().questKeys() == 4,
          "Opening never consumes the key or repeats mutation");
    g.move(Action::Forward);
    check(g.movement().player.cell == Cell{4, 3}, "Opened door admits player");
    check(!g.nearbyFixture().id.world, "Standing on an open threshold has no faced-door prompt");
    auto reset =
        std::make_unique<GameSession>(map, cat, placements, 5, Player{{2, 3}, Heading::East}, false,
                                      std::vector<NpcTimings>{}, RecoveryLevel{}, fixtureDefinitions());
    check(!reset->interact(door) && !reset->party().questKeys() && reset->geometry().solid({4, 3}) &&
              !reset->fixtures().known(2),
          "Reset isolates identity, keys, geometry and knowledge");
}
void geometry() {
    auto cat = fixtureCatalog();
    auto map = fixtureMap();
    NpcWorld placements(map, {2, 3}, {"goblin"}, {{{0}, {5, 3}, {0, 0}, Heading::West}});
    auto owner =
        std::make_unique<GameSession>(map, cat, placements, 5, Player{{2, 3}, Heading::East}, false,
                                      std::vector<NpcTimings>{}, RecoveryLevel{}, fixtureDefinitions(false));
    auto& g = *owner;
    g.setAnimated(false);
    g.move(Action::Forward);
    auto geometry = g.geometry();
    check(!lineOfSight(geometry, {3, 3}, {6, 3}) && !npcSees(geometry, {6, 3}, Heading::West, {3, 3}),
          "Closed door occludes reciprocal ranged sight");
    check(!spellCell(geometry, {{3, 3}, Heading::East}, {5, 3}) && !combatAdjacent(geometry, {3, 3}, {4, 3}),
          "Spell area and melee share collision");
    check(!g.actors().move(g.actors().id(0), {4, 3}, {0, 0}, {3, 3}), "Actor cannot occupy closed door");
    Navigation nav(geometry, g.actors());
    nav.request(0, {5, 3}, {2, 3}, false);
    nav.update({1, 1});
    check(nav.lastWork() <= Navigation::nodeBudget, "BFS bounded before opening");
    check(g.interact(fixtureRequest(g, 0)).changed, "Unlocked door opens safely");
    check(!nav.take(0).ready, "Opening discards completed stale path result");
    nav.request(0, {5, 3}, {2, 3}, false);
    nav.update({1, 1});
    auto route = nav.take(0);
    check(route.ready && route.found && route.step == Cell{4, 3}, "Fresh BFS routes through opened door");
    check(lineOfSight(geometry, {3, 3}, {6, 3}) && spellCell(geometry, {{3, 3}, Heading::East}, {5, 3}),
          "Existing borrowed view sees mutation");
    check(bool(g.actors().move(g.actors().id(0), {4, 3}, {0, 0}, {3, 3})), "Actor accepts opened cell");
    check(!discoveryVisible(geometry, {3, 2}, Heading::East, {5, 3}),
          "Corner supercover cannot reveal through wall seam");
    auto bad = fixtureDefinitions();
    bad.entries[1].cell = bad.entries[0].cell;
    rejects([&] { FloorGeometry f(map, bad); }, "Overlapping definitions rejected");
    bad = fixtureDefinitions();
    bad.entries[1].key = 3;
    rejects([&] { FloorGeometry f(map, bad); }, "Mismatched key rejected");
    rejects(
        [&] {
            NpcWorld p(map, {2, 3}, {"goblin"}, {{{0}, {4, 3}, {0, 0}, Heading::West}});
            GameSession s(map, cat, p, 5, {{2, 3}, Heading::East}, false, {}, {}, fixtureDefinitions());
        },
        "Actor loading validates closed fixtures");
}
void safetyAndUi() {
    auto cat = fixtureCatalog();
    auto map = fixtureMap();
    NpcWorld empty(map, {2, 3}, {"goblin"}, {});
    auto owner =
        std::make_unique<GameSession>(map, cat, empty, 5, Player{{2, 3}, Heading::East}, false,
                                      std::vector<NpcTimings>{}, RecoveryLevel{}, fixtureDefinitions(false));
    auto& g = *owner;
    g.setAnimated(false);
    g.move(Action::Forward);
    CommandController commands(g);
    commands.refresh({}, false, false, true);
    check(commands.snapshot().fixturePrompt[0], "Nearby fixture has contextual prompt");
    UiInput input;
    input.pressed[std::size_t(UiKey::Interact)] = true;
    commands.refresh({}, false, true, true);
    routeHudInput(commands, input);
    check(g.geometry().solid({4, 3}), "Map modal blocks fixture input");
    commands.refresh({}, false, false, false);
    routeHudInput(commands, input);
    check(g.geometry().solid({4, 3}), "Unfocused fixture input rejected");
    commands.refresh({}, false, false, true);
    routeHudInput(commands, input);
    check(!g.geometry().solid({4, 3}), "F routes through checked interaction");
    auto incapacitated =
        std::make_unique<GameSession>(map, cat, empty, 5, Player{{3, 3}, Heading::East}, false,
                                      std::vector<NpcTimings>{}, RecoveryLevel{}, fixtureDefinitions(false));
    for (unsigned i = 0; i < 4; ++i)
        incapacitated->party().damage({std::uint16_t(i)}, 100000);
    check(!incapacitated->interact(fixtureRequest(*incapacitated, 0)), "No able companion cannot open door");
    NpcWorld enemy(map, {3, 3}, {"goblin"}, {{{0}, {2, 3}, {0, 0}, Heading::East}});
    auto combat =
        std::make_unique<GameSession>(map, cat, enemy, 5, Player{{3, 3}, Heading::East}, true,
                                      std::vector<NpcTimings>{}, RecoveryLevel{}, fixtureDefinitions(false));
    check(!combat->interact(fixtureRequest(*combat, 0)) && combat->geometry().solid({4, 3}),
          "Contact/combat cannot open door");
}
void incrementalAndThreats() {
    auto cat = fixtureCatalog();
    std::vector<std::string> rows(140, std::string(140, '.'));
    rows.front() = rows.back() = std::string(140, '#');
    for (auto& row : rows) {
        row.front() = row.back() = '#';
        row[70] = '#';
    }
    rows[70][70] = '.';
    const Map map(rows);
    FloorFixtures defs;
    defs.count = 1;
    defs.entries[0] = {{70, 70}, Heading::East, FixtureKind::Door, 0};
    NpcWorld npc(map, {69, 70}, {"goblin"}, {{{0}, {100, 100}, {0, 0}, Heading::West}});
    auto g = std::make_unique<GameSession>(map, cat, npc, 5, Player{{69, 70}, Heading::East}, false,
                                           std::vector<NpcTimings>{}, RecoveryLevel{}, defs);
    Navigation nav(g->geometry(), g->actors());
    nav.request(0, {100, 100}, {2, 2}, false);
    nav.update({69, 70});
    check(nav.lastWork() == Navigation::nodeBudget && !nav.take(0).ready,
          "Large correctness fixture leaves an incremental search pending");
    check(g->interact(fixtureRequest(*g, 0)).changed, "Open while old BFS is pending");
    Navigation::Result result;
    for (unsigned i = 0; i < 100 && !result.ready; ++i) {
        nav.request(0, {100, 100}, {2, 2}, false);
        nav.update({69, 69});
        check(nav.lastWork() <= Navigation::nodeBudget && nav.lastCompleted() <= Navigation::requestBudget,
              "Invalidation cleanup retains per-tick work bound");
        result = nav.take(0);
    }
    check(result.ready && result.found, "Canceled partial search restarts against opened geometry");
    const auto small = fixtureMap();
    NpcWorld guard(small, {3, 3}, {"goblin"}, {{{0}, {6, 3}, {0, 0}, Heading::West}});
    auto live =
        std::make_unique<GameSession>(small, cat, guard, 5, Player{{3, 3}, Heading::East}, true,
                                      std::vector<NpcTimings>{}, RecoveryLevel{}, fixtureDefinitions(false));
    for (unsigned i = 0; i < 10; ++i)
        live->update(.1, false);
    check(live->ai().actors[0].awareness == Awareness::Routine,
          "Actual AI cannot perceive through closed door");
    check(live->interact(fixtureRequest(*live, 0)).changed, "Quiet approach permits opening");
    live->update(.1, false);
    check(live->ai().actors[0].awareness != Awareness::Routine && !live->interact(fixtureRequest(*live, 0)),
          "Opening exposes actual perception; active approach blocks further interaction");
}
void hazardAndQueuedDiscovery() {
    const auto map = fixtureMap();
    const auto cat = spellCatalog();
    NpcWorld empty(map, {3, 3}, {"goblin"}, {});
    auto g =
        std::make_unique<GameSession>(map, cat, empty, 5, Player{{3, 3}, Heading::East}, false,
                                      std::vector<NpcTimings>{}, RecoveryLevel{}, fixtureDefinitions(false));
    auto grease = spellCommand(*g, rules::SpellKind::Grease);
    grease.aimedCell = Cell{3, 3};
    check(g->submit(grease).reason == CommandReason::Resolved,
          "Owned spell fixture creates party-cell hazard");
    g->update(1, false);
    check(g->effects().area({3, 3}) && !g->interact(fixtureRequest(*g, 0)),
          "Settled party-cell hazard still rejects fixture operation");
    const Map open(
        {"#########", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#", "#########"});
    FloorFixtures discovered;
    discovered.discovery = true;
    NpcWorld none(open, {2, 3}, {"goblin"}, {});
    auto queued = std::make_unique<GameSession>(open, cat, none, 5, Player{{2, 3}, Heading::East}, false,
                                                std::vector<NpcTimings>{}, RecoveryLevel{}, discovered);
    queued->move(Action::TurnLeft);
    queued->move(Action::TurnLeft);
    check(!queued->fixtures().discovered({2, 1}), "In-flight turn has not revealed its endpoint");
    queued->update(.16, false);
    check(queued->movement().transition.active() && queued->fixtures().discovered({2, 1}),
          "Completed turn reveals before immediately queued turn begins");
}
void generationContracts() {
    using namespace generation;
    for (unsigned depth = 1; depth <= 3; ++depth)
        for (unsigned seed : {1u, 5u, 8u})
            for (bool fallback : {false, true}) {
                const auto result = generate({seed, depth}, templates(), fallback);
                check(bool(result) && !validateFixtures(result.floor->layout),
                      "Normal and fallback fixture progression validated at each depth");
                auto f = result.floor->layout;
                f.fixtures.entries[0].cell = f.rooms[0].anchor;
                check(bool(validateFixtures(f)), "Door moved off mandatory route is rejected");
                if (depth > 1) {
                    f = result.floor->layout;
                    for (unsigned i = 0; i < f.roomCount; ++i)
                        if (f.rooms[i].role == Role::Approach)
                            f.fixtures.entries[1].cell = f.rooms[i].anchor;
                    check(bool(validateFixtures(f)), "Key behind own lock rejected");
                }
            }
}
} // namespace
int main() {
    try {
        interactions();
        geometry();
        safetyAndUi();
        incrementalAndThreats();
        hazardAndQueuedDiscovery();
        generationContracts();
        std::cout << "Fixture contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

#include "game/npc_ai.hpp"
#include "game/perception.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
using namespace blobber;
namespace {
Map room(int width = 15, int height = 15) {
    std::vector<std::string> rows(height, std::string(width, '.'));
    rows.front() = rows.back() = std::string(width, '#');
    for (auto& row : rows)
        row.front() = row.back() = '#';
    return Map(rows);
}
NpcInstance npc(Cell c, Heading h = Heading::South, Cell sub = {0, 0}) {
    return {{0}, c, sub, h};
}
int manhattan(Cell a, Cell b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}
void sight() {
    const auto map = room(19, 19);
    for (int d = 0; d < 4; ++d) {
        const auto h = static_cast<Heading>(d);
        const Cell origin{9, 9};
        const auto f = forward(h), r = right(h);
        check(npcSees(map, origin, h, origin + f * 6), "Sight includes six-cell range boundary");
        check(!npcSees(map, origin, h, origin + f * 7), "Sight excludes beyond six cells");
        check(npcSees(map, origin, h, origin + f * 4 + r * 4), "Cone includes diagonal boundary");
        check(!npcSees(map, origin, h, origin + f * 3 + r * 4), "Cone excludes wider angles");
        check(!npcSees(map, origin, h, origin + f * -1), "No rear sight");
        check(!npcSees(map, origin, h, origin + r), "No perpendicular sight");
    }
    const Map blocked({"#######", "#.....#", "#.#...#", "#.....#", "#.....#", "#.....#", "#######"});
    check(!npcSees(blocked, {2, 1}, Heading::South, {2, 4}), "Walls occlude sight");
    check(!npcSees(blocked, {1, 1}, Heading::South, {3, 3}), "Corner seam is conservatively blocked");
    check(npcSees(blocked, {4, 1}, Heading::South, {4, 5}), "Parallel clear corridor sees through");
}
void memoryAndRoutines() {
    const auto map = room();
    const auto catalog = combatCatalog();
    NpcWorld placements(map, {7, 6}, {"goblin"}, {npc({7, 2})});
    ActorWorld world(map, catalog, placements);
    NpcAiController ai(map, world, 5);
    ai.update(100, 100, {{7, 6}, Heading::North});
    check(ai.snapshot().actors[0].awareness == Awareness::Pursuing, "Directional sight alerts guard");
    for (int t = 200; t <= 700; t += 100)
        ai.update(100, t, {{7, 6}, Heading::North});
    check(world.snapshot().actors[0].npc.cell == Cell{7, 3}, "First pursuit step waits 600 ms");
    ai.update(100, 800, {{1, 1}, Heading::North});
    check(ai.snapshot().actors[0].awareness == Awareness::Investigating &&
              ai.snapshot().actors[0].lastSeen == Cell{7, 6},
          "Loss of sight uses last seen position, not hidden live party");
    bool searched = false, returned = false;
    for (int t = 900; t <= 16000; t += 100) {
        ai.update(100, t, {{1, 1}, Heading::North});
        searched |= ai.snapshot().actors[0].awareness == Awareness::Searching;
        returned |= ai.snapshot().actors[0].awareness == Awareness::Returning;
    }
    check(searched && returned && world.snapshot().actors[0].npc.cell == Cell{7, 2} &&
              world.snapshot().actors[0].npc.facing == Heading::South,
          "Guard searches then returns home and restores facing");
    auto patrol = npc({3, 3}, Heading::North);
    patrol.ai.routine = NpcRoutine::Patrol;
    patrol.ai.waypoints[0] = {6, 3};
    patrol.ai.waypoints[1] = {3, 3};
    patrol.ai.waypointCount = 2;
    auto wander = npc({10, 10}, Heading::South);
    wander.ai.routine = NpcRoutine::Wander;
    NpcWorld routines(map, {1, 1}, {"goblin"}, {patrol, wander});
    ActorWorld wa(map, catalog, routines), wb(map, catalog, routines);
    NpcAiController a(map, wa, 18), b(map, wb, 18);
    bool far = false, loop = false, wandered = false;
    for (int t = 100; t <= 18000; t += 100) {
        a.update(100, t, {{1, 13}, Heading::North});
        b.update(100, t, {{1, 13}, Heading::North});
        const auto p = wa.snapshot().actors[0].npc.cell, w = wa.snapshot().actors[1].npc.cell;
        far |= p == Cell{6, 3};
        loop |= far && p == Cell{3, 3};
        wandered |= !(w == Cell{10, 10});
        check(w == wb.snapshot().actors[1].npc.cell && manhattan(w, {10, 10}) <= 3,
              "Seeded wander stays in home radius");
    }
    check(far, "Patrol reaches far waypoint");
    check(loop, "Patrol returns along loop");
    check(wandered, "Wander moves");
    auto bad = patrol;
    bad.ai.waypointCount = 9;
    rejects([&] { NpcWorld invalid(map, {1, 1}, {"goblin"}, {bad}); },
            "Waypoint capacity rejects before indexing");
    bad = patrol;
    bad.ai.waypoints[0] = {0, 0};
    rejects([&] { NpcWorld invalid(map, {1, 1}, {"goblin"}, {bad}); }, "Wall patrol destination rejected");
    const Map disconnected({"#######", "#..#..#", "#..#..#", "#######"});
    bad = npc({1, 1});
    bad.ai.routine = NpcRoutine::Patrol;
    bad.ai.waypointCount = 1;
    bad.ai.waypoints[0] = {5, 1};
    rejects([&] { NpcWorld invalid(disconnected, {2, 1}, {"goblin"}, {bad}); },
            "Disconnected patrol destination rejected");
}
void navigation() {
    const Map map({"#########", "#...#...#", "#...#...#", "#...#...#", "#.......#", "#########"});
    const auto catalog = combatCatalog();
    NpcWorld placements(map, {1, 4}, {"goblin"}, {npc({2, 1})});
    ActorWorld world(map, catalog, placements);
    Navigation nav(map, world);
    nav.request(0, {2, 1}, {6, 1}, false);
    nav.update({1, 4});
    auto result = nav.take(0);
    check(result.ready && result.found && manhattan(result.step, {2, 1}) == 1,
          "BFS returns a legal first step around a wall");
    nav.request(0, {2, 1}, {0, 0}, false);
    nav.update({1, 4});
    result = nav.take(0);
    check(result.ready && !result.found, "Unreachable goal reports no path");
    nav.request(0, {2, 1}, {1, 4}, false);
    nav.update({1, 4});
    result = nav.take(0);
    check(result.ready && !result.found, "Party cell cannot be a navigation destination");
    const auto large = room(100, 100);
    NpcWorld lp(large, {1, 1}, {"goblin"}, {npc({2, 2}), npc({3, 2})});
    ActorWorld lw(large, catalog, lp);
    Navigation ln(large, lw);
    ln.request(0, {2, 2}, {98, 98}, false);
    ln.request(1, {3, 2}, {4, 2}, false);
    ln.update({1, 1});
    check(!ln.take(0).ready && ln.lastWork() == Navigation::nodeBudget,
          "Large BFS yields at bounded work limit");
    bool first = false, second = false;
    for (int i = 0; i < 20; ++i) {
        ln.update({1, 1});
        first |= ln.take(0).ready;
        second |= ln.take(1).ready;
        check(ln.lastWork() <= Navigation::nodeBudget && ln.lastCompleted() <= Navigation::requestBudget,
              "Search and cleanup share bounded budget");
    }
    check(first && second, "Queued actor served after finite large search and cleanup");
    ln.request(0, {2, 2}, {98, 98}, false);
    ln.update({1, 1});
    ln.cancel(0);
    for (int i = 0; i < 10; ++i)
        ln.update({1, 1});
    check(!ln.take(0).ready, "Canceled actor never receives an actionable stale result");
}
void arrivalBeforeCombat() {
    const auto map = combatMap();
    const auto catalog = combatCatalog();
    const Player player{{3, 5}, Heading::North};
    NpcWorld placements(map, player.cell, {"goblin"}, {{{0}, {3, 3}, {0, 0}, Heading::South}});
    GameSession game(map, catalog, placements, 5, player, false);
    game.actors().walk(game.actors().id(0), {3, 4}, {0, 0}, player.cell, 0);
    game.update(0, false);
    check(game.combat().snapshot().phase == CombatPhase::Contact && !game.combat().snapshot().awaitingAction,
          "Approach locks contact without enabling an attack");
    game.update(.599, false);
    check(game.combat().snapshot().phase == CombatPhase::Contact,
          "Combat waits through the last travel frame");
    game.update(60, true);
    check(game.timeMs() == 599 && game.combat().snapshot().phase == CombatPhase::Contact,
          "Pause cannot skip arrival");
    const auto outcome = game.update(.001, false).step;
    check(outcome.event == CombatEvent::Started && game.combat().snapshot().phase == CombatPhase::Turns,
          "Combat starts exactly at arrival");
}
void contentionAndSession() {
    const auto map = room();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    std::vector<NpcInstance> instances;
    for (int i = 0; i < 4; ++i)
        instances.push_back(npc({7, 3}, Heading::South, {i % 2, i / 2}));
    const Player party{{7, 7}, Heading::East};
    NpcWorld p(map, party.cell, {"goblin"}, instances);
    GameSession game(map, catalog, p, 5, party);
    game.setAnimated(false);
    for (int i = 0; i < 25; ++i)
        game.update(.1, false);
    check(!realTime(game.combat().snapshot().phase), "Side arrivals start hybrid contact");
    check(game.actors().occupiedSlots({7, 6}) == 15,
          "Four pursuers share legal distinct destination subslots");
    for (const auto& a : game.actors().snapshot().actors) {
        if (a.maxHitPoints)
            check(!(a.npc.cell == party.cell), "Actors cannot enter party cell");
    }
    const auto before = game.actors().snapshot().revision, clock = game.ai().timeMs;
    game.update(60, true);
    check(game.actors().snapshot().revision == before && game.ai().timeMs == clock,
          "Pause freezes AI and actors");
    const auto poseBefore = game.movement().player;
    game.move(Action::TurnLeft);
    check(game.movement().player == poseBefore, "Contact locks exploration turns during combat");
    game.update(.1, false);
    const auto frozen = game.ai().timeMs;
    for (int i = 0; i < 10; ++i)
        game.update(.1, false);
    check(game.ai().timeMs == frozen, "Combat stops AI clock and navigation");
    for (std::size_t i = 0; i < 4; ++i)
        game.actors().damage(game.actors().id(i), 1000, game.timeMs());
    game.update(.1, false);
    game.update(60, false);
    check(game.combat().snapshot().phase == CombatPhase::Victory && game.ai().timeMs == frozen,
          "Victory advances presentation but not AI");
    check(game.combat().continueExploring(), "Continue returns to exploration");
    game.update(.1, false);
    check(game.ai().timeMs == frozen + 100, "Continue has no accumulated AI time");
    GameSession restart(map, catalog, p, 5, party);
    check(restart.ai().timeMs == 0 && !restart.actors().actor(game.actors().id(0)),
          "Restart resets AI and actor identities");
    // Removing/death during pursuit cancels work and releases capacity immediately.
    restart.update(.1, false);
    restart.actors().remove(restart.actors().id(0));
    restart.actors().damage(restart.actors().id(1), 1000, restart.timeMs());
    restart.update(60, false);
    check(restart.actors().snapshot().actors[0].life == ActorLife::Removed &&
              restart.actors().snapshot().actors[1].life == ActorLife::Dead,
          "Terminal actors never resume movement");
    check(manhattan(restart.actors().snapshot().actors[2].npc.cell, {7, 3}) <= 1,
          "Large delta never catches up multiple steps");
}
void preferredApproachRoutes() {
    const auto map = room();
    const auto catalog = combatCatalog();
    for (int d = 0; d < 4; ++d) {
        const Player party{{7, 7}, static_cast<Heading>(d)};
        const auto f = forward(party.heading), r = right(party.heading);
        const auto from = party.cell + f * 2 + r;
        NpcWorld places(map, party.cell, {"goblin"}, {npc(from)});
        ActorWorld actors(map, catalog, places);
        Navigation nav(map, actors);
        nav.request(0, from, party.cell, true, party.cell + f);
        nav.update(party.cell);
        const auto result = nav.take(0);
        check(result.ready && result.found && result.step == party.cell + f * 2,
              "Tied approach moves toward front axis instead of into the flank");
        check(result.preferred == party.cell + f && nav.lastWork() <= Navigation::nodeBudget,
              "Preferred route preserves request metadata and work budget");
    }
    const Map isolated({"#######", "#######", "###.###", "#.....#", "#.....#", "#######"});
    NpcWorld places(isolated, {3, 3}, {"goblin"}, {npc({5, 3})});
    ActorWorld actors(isolated, catalog, places);
    Navigation nav(isolated, actors);
    nav.request(0, {5, 3}, {3, 3}, true, Cell{3, 2});
    nav.update({3, 3});
    const auto fallback = nav.take(0);
    check(fallback.ready && fallback.found && fallback.step == Cell{4, 3},
          "Unreachable preferred front falls back to a reachable adjacent tile");
    nav.request(0, {5, 3}, {3, 3}, true, Cell{3, 2});
    nav.cancel(0);
    nav.update({3, 3});
    check(!nav.take(0).ready, "Canceled preferred search cannot publish its fallback");

    for (bool full : {false, true}) {
        const Map blocked(
            {"#######", "#.....#", full ? "#.....#" : "###.###", "#.....#", "#.....#", "#######"});
        const Cell party{2, 3}, front{2, 2};
        std::vector<NpcInstance> instances{npc({4, 3})};
        if (full)
            for (int i = 0; i < 4; ++i)
                instances.push_back(npc(front, Heading::South, {i % 2, i / 2}));
        NpcWorld world(blocked, party, {"goblin"}, instances);
        ActorWorld a(blocked, catalog, world);
        Navigation n(blocked, a);
        n.request(0, {4, 3}, party, true, front);
        n.update(party);
        const auto result = n.take(0);
        check(result.ready && result.found && result.step == Cell{3, 3},
              "Solid or fully occupied front uses normal adjacency without blocking pursuit");
    }
    // An unreachable preference may need several bounded updates, including cleanup.
    std::vector<std::string> rows(80, std::string(80, '.'));
    rows.front() = rows.back() = std::string(80, '#');
    for (auto& row : rows)
        row.front() = row.back() = '#';
    rows[38][40] = rows[39][39] = rows[39][41] = '#';
    const Map large(rows);
    NpcWorld largePlaces(large, {40, 40}, {"goblin"}, {npc({42, 40})});
    ActorWorld largeActors(large, catalog, largePlaces);
    Navigation bounded(large, largeActors);
    bounded.request(0, {42, 40}, {40, 40}, true, Cell{40, 39});
    bool found = false;
    int updates = 0;
    for (; updates < 20 && !found; ++updates) {
        bounded.update({40, 40});
        check(bounded.lastWork() <= Navigation::nodeBudget &&
                  bounded.lastCompleted() <= Navigation::requestBudget,
              "Preferred fallback search respects incremental work budgets");
        found = bounded.take(0).found;
    }
    check(found && updates > 1, "Large unreachable preference eventually publishes its adjacent fallback");
}
void fullCenterUsesDiagonalFront() {
    const auto map = room();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    for (int d = 0; d < 4; ++d) {
        const Player party{{7, 7}, static_cast<Heading>(d)};
        const auto f = forward(party.heading), r = right(party.heading);
        for (const int fill : {1, 3}) {
            std::vector<NpcInstance> instances{npc(party.cell + f * 3 + r)};
            for (int side = -1; side <= 1; ++side) {
                if (fill == 1 && side)
                    continue;
                for (int sub = 0; sub < 4; ++sub)
                    instances.push_back(npc(party.cell + f + r * side, Heading::South, {sub % 2, sub / 2}));
            }
            NpcWorld places(map, party.cell, {"goblin"}, instances);
            ActorWorld actors(map, catalog, places);
            NpcAiController ai(map, actors, 5);
            bool reached = false;
            for (int i = 0; i < 20 && !reached; ++i) {
                ai.approach(actors.id(0), party, i * npcWalkDurationMs);
                reached = combatAdjacent(map, actors.actor(actors.id(0))->npc.cell, party.cell);
            }
            const auto cell = actors.actor(actors.id(0))->npc.cell;
            check(reached && actors.occupiedSlots(cell) != 15,
                  "Approach finds a free contact slot without overflowing occupancy");
            check(inFrontBand(party.cell, party.heading, cell) == (fill == 1),
                  "Full center uses diagonal front before flanks; full band permits other contact");
        }
    }
}
void blockedRoutesAndRng() {
    const Map corridor({"#########", "#.......#", "#########"});
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    std::vector<NpcInstance> blockers{npc({2, 1}, Heading::East)};
    for (int i = 0; i < 4; ++i)
        blockers.push_back(npc({3, 1}, Heading::North, {i % 2, i / 2}));
    NpcWorld p(corridor, {7, 1}, {"goblin"}, blockers);
    ActorWorld actors(corridor, catalog, p);
    Navigation nav(corridor, actors);
    nav.request(0, {2, 1}, {6, 1}, false);
    nav.update({7, 1});
    check(!nav.take(0).found, "Full cell blocks narrow corridor");
    actors.damage(actors.id(1), 1000, 0);
    nav.request(0, {2, 1}, {6, 1}, false);
    nav.update({7, 1});
    const auto route = nav.take(0);
    check(route.ready && route.found && route.step == Cell{3, 1},
          "Immediate death release opens route through free subslot");
    check(actors.walk(actors.id(0), route.step, {0, 0}, {7, 1}, 100).changed &&
              actors.occupiedSlots({3, 1}) == 15,
          "Checked walker occupies the freed slot without overlap");
    const auto map = room(25, 25);
    auto wander = npc({20, 20}, Heading::North);
    wander.ai.routine = NpcRoutine::Wander;
    const Player spawn{{3, 10}, Heading::North};
    NpcWorld placed(map, spawn.cell, {"goblin"}, {npc({3, 5}, Heading::North), wander});
    GameSession ai(map, catalog, placed, 18, spawn), stationary(map, catalog, placed, 18, spawn, false);
    ai.setAnimated(false);
    stationary.setAnimated(false);
    for (int i = 0; i < 100; ++i) {
        ai.update(.1, false);
        stationary.update(.1, false);
    }
    check(ai.actors().snapshot().actors[1].hasWalk, "RNG isolation fixture consumed wandering choices");
    for (int i = 0; i < 4; ++i) {
        ai.move(Action::Forward);
        stationary.move(Action::Forward);
    }
    for (int i = 0; i < 30; ++i) {
        const auto a = ai.update(.45, false), b = stationary.update(.45, false);
        check(a.count == b.count, "Roaming preserves combat event count");
        for (std::size_t event = 0; event < a.count; ++event)
            check(a.values[event].event == b.values[event].event &&
                      a.values[event].roll == b.values[event].roll &&
                      a.values[event].hit == b.values[event].hit &&
                      a.values[event].damage == b.values[event].damage,
                  "Roaming does not alter authoritative impact rolls");
        check(ai.combat().snapshot().activeCharacter.value_or(rules::CharacterId{}).index ==
                  stationary.combat().snapshot().activeCharacter.value_or(rules::CharacterId{}).index,
              "Roaming does not alter initiative order");
        if (ai.combat().snapshot().awaitingAction) {
            ai.submit(turnCommand(ai));
            stationary.submit(turnCommand(stationary));
        }
    }
    NpcWorld turning(map, {10, 9}, {"goblin"}, {npc({9, 5}, Heading::South)});
    ActorWorld turner(map, catalog, turning);
    NpcAiController pursuit(map, turner, 5);
    for (int t = 100; t <= 4000; t += 100)
        pursuit.update(100, t, {{10, 9}, Heading::East});
    check(combatAdjacent(map, turner.snapshot().actors[0].npc.cell, {10, 9}),
          "Turning along a path cannot strand pursuit at a still-occupied last-seen cell");
    ActorWorld watcher(map, catalog, placed);
    NpcAiController brain(map, watcher, 5);
    brain.update(100, 100, {{3, 2}, Heading::South});
    brain.update(100, 200, {{20, 2}, Heading::South});
    check(brain.snapshot().actors[0].awareness == Awareness::Investigating,
          "Watcher investigates after sight loss");
    brain.update(100, 300, {{3, 1}, Heading::South});
    check(brain.snapshot().actors[0].awareness == Awareness::Pursuing &&
              brain.snapshot().actors[0].lastSeen == Cell{3, 1},
          "Reacquisition interrupts investigation and updates memory");
}
void stress(int dimension) {
    const auto map = room(dimension, dimension);
    const auto catalog = combatCatalog();
    std::vector<NpcInstance> instances;
    for (int i = 0; i < 256; ++i)
        instances.push_back(npc({2 + i % 16, 2 + i / 16}));
    NpcWorld p(map, {1, 1}, {"goblin"}, instances);
    ActorWorld actors(map, catalog, p);
    Navigation nav(map, actors);
    for (std::size_t i = 0; i < 256; ++i)
        nav.request(i, instances[i].cell, {dimension - 2, dimension - 2}, false);
    const auto start = std::chrono::steady_clock::now();
    std::size_t finished = 0, ticks = 0;
    while (finished < 256 && ticks < 140000) {
        nav.update({1, 1});
        ++ticks;
        check(nav.lastWork() <= 4096 && nav.lastCompleted() <= 8, "Stress work bound");
        for (std::size_t i = 0; i < 256; ++i) {
            const auto r = nav.take(i);
            if (r.ready) {
                check(r.found, "Stress goal reachable");
                ++finished;
            }
        }
    }
    check(finished == 256, "Stress queue eventually serves all 256 actors");
    const auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start)
            .count();
    std::cout << "Stress " << dimension << "x" << dimension << ": " << finished << " actors, " << ticks
              << " ticks, " << nav.storageBytes() << " scratch bytes, " << ms << " wall ms\n";
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--stress") {
            stress(25);
            stress(1024);
        } else {
            sight();
            memoryAndRoutines();
            navigation();
            contentionAndSession();
            arrivalBeforeCombat();
            preferredApproachRoutes();
            fullCenterUsesDiagonalFront();
            blockedRoutesAndRng();
        }
        std::cout << "AI checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

#include "game/commands.hpp"
#include "game/perception.hpp"
#include "generation/dressing.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "support/generation_blueprints.hpp"
#include "support/recovery_fixtures.hpp"
#include <iostream>
#include <memory>
#include <string_view>
using namespace blobber;
namespace {
generation::FloorLayout roomFixture() {
    generation::FloorLayout f;
    f.width = 13;
    f.height = 9;
    f.roomCount = 1;
    f.rooms[0].origin = {2, 2};
    f.rooms[0].size = {7, 5};
    f.rooms[0].anchor = {3, 3};
    for (int y = 2; y <= 6; ++y)
        for (int x = 2; x <= 8; ++x) {
            f.floor[unsigned(y * f.width + x)] = 1;
            f.membership[unsigned(y * f.width + x)] = 0;
        }
    return f;
}
void placementRules() {
    using namespace generation;
    auto f = roomFixture();
    CellMask reserved{};
    Decoration prop{{2, 2}, Heading::North, DecorationKind::Cage};
    check(blockingPropFitsRoom(f, prop, reserved), "Spacious wall-backed room admits a cage");
    prop.kind = DecorationKind::Rack;
    check(blockingPropFitsRoom(f, prop, reserved), "Single-cell rack uses same room clearance");
    reserved[unsigned(2 * f.width + 3)] = true;
    check(!blockingPropFitsRoom(f, prop, reserved), "Existing furniture consumes supporting clearance");
    reserved.fill(false);
    f.membership[unsigned(2 * f.width + 2)] = noRegion;
    check(!blockingPropFitsRoom(f, prop, reserved), "Bounding rectangle cannot admit corridor cells");
    f = roomFixture();
    f.rooms[0].role = Role::Entrance;
    check(!blockingPropFitsRoom(f, prop, reserved), "Actual entrance membership excludes props");
    f.rooms[0].role = Role::Objective;
    check(!blockingPropFitsRoom(f, prop, reserved), "Actual objective membership excludes props");
    for (unsigned width : {1u, 2u}) {
        f = roomFixture();
        for (int y = 2 + int(width); y <= 6; ++y)
            for (int x = 2; x <= 8; ++x) {
                f.floor[unsigned(y * f.width + x)] = 0;
                f.membership[unsigned(y * f.width + x)] = noRegion;
            }
        f.rooms[0].anchor = {3, 2};
        check(!blockingPropFitsRoom(f, prop, reserved), "One/two-cell-wide room strips reject props");
    }
    f = roomFixture();
    f.connectionCount = 1;
    f.connections[0].a = 0;
    f.connections[0].b = 1;
    f.connections[0].socketA = {2, 1};
    f.connections[0].facingA = Heading::North;
    check(!blockingPropFitsRoom(f, prop, reserved), "Room-side passage entrance remains free");
    // Two spacious lobes share a narrow mouth. An outside detour preserves global
    // connectivity, but must never make the room-local obstruction acceptable.
    f = roomFixture();
    for (int y = 2; y <= 6; ++y)
        for (int x = 2; x <= 8; ++x) {
            const auto n = unsigned(y * f.width + x);
            const bool room = y <= 4 && (x != 5 || y == 3);
            const bool detour = y == 6 || (y == 5 && (x == 2 || x == 8));
            f.floor[n] = room || detour;
            f.membership[n] = room ? 0 : noRegion;
        }
    prop.kind = DecorationKind::Cage;
    prop.cell = {4, 3};
    WorldDefinition world;
    world.decorations.push_back(prop);
    std::vector<std::string> rows(std::size_t(f.height), std::string(std::size_t(f.width), '#'));
    for (int y = 0; y < f.height; ++y)
        for (int x = 0; x < f.width; ++x)
            if (floorCell(f, {x, y}))
                rows[unsigned(y)][unsigned(x)] = '.';
    check(Map(rows, world).connected(f.rooms[0].anchor), "Fixture retains a global alternate route");
    check(!blockingPropFitsRoom(f, prop, reserved), "Outside detour cannot justify blocking a room");
}
void collisionProgression() {
    using namespace generation;
    const auto result = generate({5, 2}, blobber::test::campaignBlueprints());
    check(bool(result), "Owned keyed progression fixture generated");
    const auto& f = result.floor->layout;
    CellMask blocked{};
    check(!validateFixtures(f, &blocked), "Empty collision mask preserves progression");
    // Both entry-side approaches reach the key room. With all doors open,
    // the sector branch is a detour; before finding the key, that branch is locked.
    std::vector<std::string> rows(std::size_t(f.height), std::string(std::size_t(f.width), '#'));
    for (int y = 0; y < f.height; ++y)
        for (int x = 0; x < f.width; ++x)
            if (floorCell(f, {x, y}))
                rows[unsigned(y)][unsigned(x)] = '.';
    bool found = false;
    // Find a small cut of the actual upstream cycle; never assume room IDs or edges.
    for (unsigned i = 0; i < f.connectionCount && !found; ++i)
        for (unsigned j = i; j < f.connectionCount && !found; ++j) {
            blocked.fill(false);
            auto world = result.floor->map.definition();
            bool eligible = true;
            for (unsigned edge : {i, j}) {
                const auto& e = f.connections[edge];
                if (f.rooms[e.a].role != Role::Junction || f.rooms[e.b].role != Role::Junction) {
                    eligible = false;
                    break;
                }
                const auto c = pathCell(f, e, e.count / 2);
                if (!blocked[unsigned(c.y * f.width + c.x)])
                    world.decorations.push_back({c, Heading::North, DecorationKind::Cage});
                blocked[unsigned(c.y * f.width + c.x)] = true;
            }
            if (eligible && validateFixtures(f, &blocked).code == Error::Progression &&
                Map(rows, world).connected(f.spawn.cell))
                found = true;
        }
    check(found,
          "An open-door detour cannot substitute for pre-key progression through a blocked upstream cycle");
    blocked.fill(false);
    blocked[unsigned(f.guardianApproach.y * f.width + f.guardianApproach.x)] = true;
    check(validateFixtures(f, &blocked).code == Error::Progression,
          "Furniture blocking guardian access rejects progression");
    check(!validateFixtures(f), "Terrain-only validation misses the furniture obstruction");
    blocked.fill(false);
    blocked[unsigned(f.spawn.cell.y * f.width + f.spawn.cell.x)] = true;
    check(validateFixtures(f, &blocked).code == Error::Progression, "Blocked spawn rejects progression");
}
void generated() {
    for (unsigned seed : {0u, 1u, 5u, 13u})
        for (unsigned depth = 1; depth <= 3; ++depth) {
            const auto a = generation::generate({seed, depth}, blobber::test::campaignBlueprints()),
                       b = generation::generate({seed, depth}, blobber::test::campaignBlueprints());
            check(bool(a) && bool(b), "Dressed floor generated");
            const auto& f = *a.floor;
            check(f.map.definition().assetPack == "underkeep" && f.layout.cacheCount == 3u,
                  "Biome and chest count");
            check(f.map.definition().fingerprint() == b.floor->map.definition().fingerprint(),
                  "Dressing deterministic");
            check(f.map.connected(f.layout.spawn.cell), "Furniture preserves walkable connectivity");
            unsigned chests = 0, large = 0;
            generation::CellMask reserved{}, blocked{};
            for (const auto& d : f.map.definition().decorations)
                for (unsigned k = 0; k < d.footprintCount(); ++k) {
                    const auto c = d.footprint(k);
                    reserved[unsigned(c.y * f.layout.width + c.x)] = true;
                    blocked[unsigned(c.y * f.layout.width + c.x)] = d.blocks();
                }
            check(!generation::validateFixtures(f.layout, &blocked), "Dressed progression remains valid");
            for (const auto& d : f.map.definition().decorations) {
                if (d.kind == DecorationKind::Chest) {
                    ++chests;
                    check(d.container < f.layout.cacheCount && d.cell == f.layout.caches[d.container].cell,
                          "Container site binding");
                    check(!f.map.movementBlocked(d.cell + forward(d.face) * -1), "Chest has reachable front");
                }
                if (d.kind == DecorationKind::Cage || d.kind == DecorationKind::Rack) {
                    ++large;
                    auto others = reserved;
                    for (unsigned k = 0; k < d.footprintCount(); ++k) {
                        const auto c = d.footprint(k);
                        others[unsigned(c.y * f.layout.width + c.x)] = false;
                    }
                    check(generation::blockingPropFitsRoom(f.layout, d, others),
                          "Published furniture has same-room clearance and bypass");
                }
                if (d.kind == DecorationKind::Rack)
                    check(depth == 3, "Rack only at deepest depth");
                for (unsigned k = 0; k < d.footprintCount(); ++k) {
                    const auto c = d.footprint(k);
                    check(!f.map.solid(c) && f.map.movementBlocked(c) == d.blocks() && f.map.npcBlocked(c),
                          "Physical furniture stays reserved; only chest cells admit the player");
                    check(!f.layout.fixtures.npcExcluded[unsigned(c.y * f.layout.width + c.x)],
                          "Stairs protected");
                    for (unsigned n = 0; n < f.layout.pathCount; ++n)
                        check(f.layout.paths[n] != unsigned(c.y * f.layout.width + c.x), "Passage protected");
                    for (unsigned n = 0; n < f.layout.populationCount; ++n)
                        check(!(f.layout.population[n].cell == c), "Actors protected");
                }
            }
            check(chests == f.layout.cacheCount && large <= 2, "Dressing counts bounded");
        }
    for (unsigned seed : {0u, 5u})
        for (unsigned depth = 1; depth <= 3; ++depth) {
            const auto f = generation::generate({seed, depth}, blobber::test::campaignBlueprints(), true);
            check(bool(f) && f.diagnostics.fallback && f.floor->map.connected(f.floor->layout.spawn.cell),
                  "Frozen fallback dresses safely");
        }
}
WorldDefinition furniture() {
    WorldDefinition w;
    Decoration c{{3, 4}, Heading::North, DecorationKind::Chest, 0, .5f, 0, .76f, .36f, .58f};
    c.container = 0;
    w.decorations.push_back(c);
    c.cell = {5, 4};
    c.container = 1;
    c.sealed = true;
    w.decorations.push_back(c);
    w.decorations.push_back({{1, 1}, Heading::North, DecorationKind::Cage, 0, .5f, 0, .68f, .84f, .72f});
    w.decorations.push_back({{5, 6}, Heading::East, DecorationKind::Rack, 0, .5f, 0, .68f, .36f, 1.7f});
    return w;
}
void containers() {
    const auto catalog = recoveryCatalog();
    const std::vector<std::string> rows{"#########", "#.......#", "#.......#", "#.......#", "#.......#",
                                        "#.......#", "#.......#", "#.......#", "#########"};
    const Map map(rows, furniture());
    const Player pose{{3, 5}, Heading::North};
    const auto empty = combatPlacements(map, pose, 0);
    RecoveryLevel level;
    level.ground[0] = {{3, 4}, {10}, 1, 0};
    level.ground[1] = {{3, 4}, {11}, 2, 0};
    level.ground[2] = {pose.cell, {12}, 3};
    level.ground[3] = {{5, 4}, {11}, 1, 1};
    level.groundCount = 4;
    auto owner =
        std::make_unique<GameSession>(map, catalog, empty, 7, pose, false, std::vector<NpcTimings>{}, level);
    auto& game = *owner;
    const auto world = game.loot().snapshot().world;
    const ContainerId chest{world, 0};
    const LootId first{world, std::uint16_t(NpcWorld::maxInstances * lootPerActor), 1};
    CommandController promptUi(game);
    check(std::string_view(promptUi.snapshot().fixturePrompt.data()) == "[F] Open",
          "Closed accessible chest offers Open");
    check(map.canStep(pose.cell, {3, 4}) && map.canStep(pose.cell, {4, 5}) &&
              !game.geometry().npcAllowed({3, 4}),
          "Player can enter chest cell while NPCs cannot overlap it");
    game.setAnimated(false);
    game.move(Action::Forward);
    check(game.movement().player.cell == Cell{3, 4} && game.nearbyContainer().slot == 0 &&
              bool(game.containerAccess(chest)),
          "Step onto chest tile and face its backing wall to interact");
    game.move(Action::TurnRight);
    check(bool(game.containerAccess(chest)),
          "Same-cell chest access remains available while turning in place");
    game.move(Action::TurnLeft);
    check(!map.canStep({4, 6}, {5, 6}) && !game.geometry().npcAllowed({5, 6}) &&
              map.canStep({6, 5}, {6, 6}) && game.geometry().npcAllowed({6, 6}),
          "Rack blocks its anchor; party and NPCs can use the former second cell");
    check(lineOfSight(game.geometry(), pose.cell, {3, 3}),
          "Low furniture does not become full-height sight wall");
    check(!game.pickup({first, {0}, game.party().inventory().revision()}),
          "Closed chest cannot be picked up");
    check(!game.openContainer({world + 1, 0}) && !game.openContainer({world, 7}),
          "Foreign and invalid container rejected");
    watching = true;
    allocations = 0;
    const auto opened = game.openContainer(chest);
    watching = false;
    check(bool(opened) && allocations == 0, "Open chest from adjacent front without C++ allocation");
    const auto revision = game.loot().snapshot().revision;
    check(bool(game.openContainer(chest)) && game.loot().snapshot().revision == revision,
          "Repeated open is idempotent");
    check(game.loot().findContainer(chest)->opened, "Open state retained");
    promptUi.refresh({}, false, false, true);
    check(std::string_view(promptUi.snapshot().fixturePrompt.data()) == "[F] Inspect",
          "Opened chest offers Inspect");
    while (game.party().acquire({{3}, {4}, 1, game.party().inventory().revision()})) {
    }
    while (game.party().acquire({{3}, {10}, 1, game.party().inventory().revision()})) {
    }
    const auto beforeFailure = game.loot().snapshot().revision;
    check(!game.pickup({first, {3}, game.party().inventory().revision()}) && game.loot().find(first) &&
              game.loot().snapshot().revision == beforeFailure,
          "Full recipient retains chest contents unchanged");
    CommandController commands(game);
    commands.openContainer(chest);
    check(commands.snapshot().loot.open && commands.snapshot().loot.count == 2 &&
              commands.snapshot().loot.container == 0,
          "Chest UI filters its own contents");
    commands.lootTake(0);
    check(commands.snapshot().loot.count == 1, "Partial collection retains remainder");
    commands.lootTakeAll();
    check(!commands.snapshot().loot.open && !game.loot().find(first) &&
              game.loot().findContainer(chest)->opened,
          "Empty chest remains open after panel closes");
    commands.openContainer(chest);
    check(commands.snapshot().loot.open && commands.snapshot().loot.count == 0,
          "Empty chest can be inspected");
    check(game.loot().snapshot().stacks[NpcWorld::maxInstances * lootPerActor + 2].active,
          "Chest collection leaves loose loot intact");
    game.restorePose({{5, 5}, Heading::North}, false);
    promptUi.refresh({}, false, false, true);
    check(!promptUi.snapshot().interactionPrompt(), "Sealed chest offers no unavailable action");
    check(!game.openContainer({world, 1}) && !game.loot().findContainer({world, 1})->opened,
          "Sealed chest cannot open");
    auto saved = std::make_unique<FloorState>();
    game.exportFloor(*saved);
    auto restored = std::make_unique<GameSession>(map, catalog, empty, 7, Player{{3, 5}, Heading::North},
                                                  false, std::vector<NpcTimings>{}, level);
    restored->importFloor(*saved);
    check(restored->loot().snapshot().containers[0].opened &&
              !restored->loot().snapshot().stacks[first.slot].active,
          "Opened empty chest restores without reroll");
    saved->loot.state.containers[1].opened = true;
    rejects([&] { restored->importFloor(*saved); }, "Corrupt sealed/open state rejected");
    game.exportFloor(*saved);
    saved->loot.state.stacks[first.slot].container = 1;
    rejects([&] { restored->importFloor(*saved); }, "Foreign stack ownership rejected");
    game.restorePose({{3, 4}, Heading::North}, false);
    game.exportFloor(*saved);
    restored->importFloor(*saved);
    restored->restorePose(game.movement().player, false);
    check(bool(restored->openContainer({restored->loot().snapshot().world, 0})),
          "Player pose and opened chest state restore on walkable chest cell");
    auto overlapping = furniture();
    overlapping.decorations[3].cell = {3, 4};
    rejects([&] { Map bad(rows, overlapping); }, "Overlapping furniture rejected");
    auto badId = furniture();
    badId.decorations[1].container = 0;
    rejects([&] { Map bad(rows, badId); }, "Duplicate container rejected");
}
} // namespace
int main() {
    try {
        placementRules();
        collisionProgression();
        generated();
        containers();
        std::cout << "Underkeep dressing and container checks passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

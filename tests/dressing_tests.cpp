#include "game/commands.hpp"
#include "game/perception.hpp"
#include "generation/dressing.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "support/recovery_fixtures.hpp"
#include <iostream>
#include <memory>
#include <string_view>
using namespace blobber;
namespace {
void generated() {
    for (unsigned seed : {0u, 1u, 5u, 13u})
        for (unsigned depth = 1; depth <= 3; ++depth) {
            const auto a = generation::generate({seed, depth}), b = generation::generate({seed, depth});
            check(bool(a) && bool(b), "Dressed floor generated");
            const auto& f = *a.floor;
            check(f.map.definition().assetPack == "underkeep" &&
                      f.layout.cacheCount == (depth == 3 ? 2u : 3u),
                  "Biome and chest count");
            check(f.map.definition().fingerprint() == b.floor->map.definition().fingerprint(),
                  "Dressing deterministic");
            check(f.map.connected(f.layout.spawn.cell), "Furniture preserves walkable connectivity");
            unsigned chests = 0, large = 0;
            for (const auto& d : f.map.definition().decorations) {
                if (d.kind == DecorationKind::Chest) {
                    ++chests;
                    check(d.container < f.layout.cacheCount && d.cell == f.layout.caches[d.container].cell,
                          "Container site binding");
                    check(!f.map.movementBlocked(d.cell + forward(d.face) * -1), "Chest has reachable front");
                }
                if (d.kind == DecorationKind::Cage || d.kind == DecorationKind::Rack)
                    ++large;
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
            const auto f = generation::generate({seed, depth}, true);
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
    check(!game.geometry().npcAllowed({5, 6}) && !game.geometry().npcAllowed({6, 6}),
          "Both rack cells block NPCs");
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
        generated();
        containers();
        std::cout << "Underkeep dressing and container checks passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

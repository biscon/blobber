#include "generation/floor.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/fixture_world.hpp"
#include "ui/map_model.hpp"
#include <iostream>
using namespace blobber;
namespace {
Map roomsMap() {
    return Map({"###########", "#....#....#", "#.#..#....#", "#.........#", "#....#....#", "#....#....#",
                "###########"});
}
generation::FloorLayout membership(const Map& map) {
    generation::FloorLayout layout;
    layout.width = map.width();
    layout.height = map.height();
    layout.roomCount = 2;
    for (int y = 0; y < map.height(); ++y)
        for (int x = 0; x < map.width(); ++x)
            if (!map.solid({x, y}))
                layout.membership[unsigned(y * map.width() + x)] = x < 5 ? 0 : 1;
    return layout;
}
FloorFixtures definitions(FixtureKind threshold = FixtureKind::Door) {
    FloorFixtures f;
    f.discovery = true;
    f.depth = 1;
    f.entries[f.count++] = {{5, 3}, Heading::East, threshold, 0};
    f.entries[f.count++] = {{9, 3}, Heading::East, FixtureKind::Downstairs, 0};
    return f;
}
void roomKnowledge() {
    const auto catalog = fixtureCatalog();
    const auto map = roomsMap();
    const auto layout = membership(map);
    const NpcWorld empty(map, {4, 3}, {"goblin"}, {});
    auto game = std::make_unique<GameSession>(map, catalog, empty, 5, Player{{4, 3}, Heading::East}, false,
                                              std::vector<NpcTimings>{}, RecoveryLevel{}, definitions());
    MapOverlayModel model;
    game->setAnimated(false);
    model.prepare(map, &layout);
    model.refresh(&game->fixtures());
    check(!game->fixtures().discovered({1, 5}) && model.tile({1, 5}) == MapTile::Floor,
          "An interior cell discovered through LOS maps the whole recorded room");
    check(game->fixtures().discovered({5, 3}) && model.tile({6, 3}) == MapTile::Hidden &&
              model.tile({9, 5}) == MapTile::Hidden,
          "Seen closed threshold belonging to the next room cannot seed that room");
    check(model.tile({0, 0}) == MapTile::Wall && model.tile({2, 2}) == MapTile::Wall &&
              model.tile({5, 1}) == MapTile::Wall,
          "Room corners, internal wall island and far boundary are complete");
    check(model.tile({10, 6}) == MapTile::Hidden,
          "Wall completion does not propagate into another room or its outer walls");
    auto saved = std::make_unique<FloorState>();
    game->exportFloor(*saved);
    const auto digest = saved->digest();
    model.refresh(&game->fixtures());
    game->exportFloor(*saved);
    check(digest == saved->digest(), "Derived map knowledge does not mutate simulation");
    auto restored =
        std::make_unique<GameSession>(map, catalog, empty, 5, Player{{4, 3}, Heading::East}, false,
                                      std::vector<NpcTimings>{}, RecoveryLevel{}, definitions());
    restored->importFloor(*saved);
    MapOverlayModel restoredMap;
    restoredMap.prepare(map, &layout);
    restoredMap.refresh(&restored->fixtures());
    for (int y = 0; y < map.height(); ++y)
        for (int x = 0; x < map.width(); ++x)
            check(model.tile({x, y}) == restoredMap.tile({x, y}),
                  "Existing persisted LOS bits reproduce complete room maps");
    check(mapFixtureMarker(game->fixtures(), 0, false) == MapMarker::ClosedDoor &&
              mapFixtureMarker(game->fixtures(), 1, false) == MapMarker::None,
          "Known door and hidden stairs retain existing fixture knowledge");
    check(game->interact(fixtureRequest(*game, 0)).changed, "Owned test opens the blocking door");
    model.refresh(&game->fixtures());
    check(game->fixtures().discovered({6, 3}) && model.tile({9, 5}) == MapTile::Floor,
          "Opening the door supplies real LOS and reveals the next room");
    check(mapFixtureMarker(game->fixtures(), 0, false) == MapMarker::OpenDoor,
          "Open door uses remembered open-door pictogram");
    check(game->interact(fixtureRequest(*game, 0)).changed, "Owned test recloses the door");
    model.refresh(&game->fixtures());
    check(model.tile({9, 5}) == MapTile::Floor &&
              mapFixtureMarker(game->fixtures(), 0, false) == MapMarker::ClosedDoor,
          "Reclosing does not erase previously mapped rooms and updates remembered state");
    for (int y = 0; y < map.height(); ++y)
        for (int x = 0; x < map.width(); ++x)
            check(model.tile({x, y}) == model.tile({x, y}, true),
                  "Fully explored rooms have the same floors and boundaries as full-map mode");
    allocations = 0;
    watching = true;
    for (int frame = 0; frame < 40; ++frame) {
        model.refresh(&game->fixtures());
        for (int y = 0; y < map.height(); ++y)
            for (int x = 0; x < map.width(); ++x)
                check(model.tile({x, y}) == model.tile({x, y}, true), "Repeated map queries remain stable");
    }
    // Also exercise uncached refresh, not just the revision shortcut.
    model.prepare(map, &layout);
    model.refresh(&game->fixtures());
    watching = false;
    check(allocations == 0, "Cached queries, preparation and discovery refresh allocate no C++ heap storage");
    auto gates = definitions(FixtureKind::GuardianGate);
    gates.entries[gates.count++] = {{8, 1}, Heading::North, FixtureKind::Door, 0};
    FloorObjectiveDefinition objective;
    objective.count = 1;
    objective.members[0] = 0;
    objective.approachDoor = 2;
    objective.gate = 0;
    const NpcWorld guards(map, {4, 3}, {"goblin"}, {{{0}, {6, 5}, {0, 0}, Heading::South}});
    auto gateGame =
        std::make_unique<GameSession>(map, catalog, guards, 5, Player{{4, 3}, Heading::East}, false,
                                      std::vector<NpcTimings>{}, RecoveryLevel{}, gates, objective);
    model.refresh(&gateGame->fixtures());
    check(model.tile({9, 5}) == MapTile::Hidden &&
              mapFixtureMarker(gateGame->fixtures(), 0, false) == MapMarker::GuardianGate,
          "New world identity invalidates cache and a seen guardian gate blocks room revelation");
    game->exportFloor(*saved);
    saved->fixtures.open[0] = true;
    restored->importFloor(*saved);
    check(mapFixtureMarker(restored->fixtures(), 0, false) == MapMarker::ClosedDoor &&
              mapFixtureMarker(restored->fixtures(), 0, true) == MapMarker::OpenDoor &&
              mapFixtureMarker(restored->fixtures(), fixtureCapacity, true) == MapMarker::None,
          "Player markers honor remembered state, full reveal uses current state, invalid slots are empty");
}
void corridorAndFallback() {
    const auto catalog = fixtureCatalog();
    const Map map({"#########", "#.......#", "#######.#", "#.......#", "#########"});
    const NpcWorld empty(map, {1, 1}, {"goblin"}, {});
    FloorFixtures defs;
    defs.discovery = true;
    auto game = std::make_unique<GameSession>(map, catalog, empty, 5, Player{{1, 1}, Heading::East}, false,
                                              std::vector<NpcTimings>{}, RecoveryLevel{}, defs);
    MapOverlayModel model;
    model.prepare(map);
    model.refresh(&game->fixtures());
    check(model.tile({6, 1}) == MapTile::Floor && model.tile({1, 3}) == MapTile::Hidden,
          "Without room metadata corridors remain sight-based instead of flooding around bends");
    check(model.tile({6, 0}) == MapTile::Wall && model.tile({6, 2}) == MapTile::Wall,
          "Both sides of discovered corridors have continuous walls");
    auto bad = membership(map);
    bad.membership[1 * map.width() + 1] = 100;
    model.prepare(map, &bad);
    model.refresh(&game->fixtures());
    check(model.tile({1, 3}) == MapTile::Hidden, "Invalid metadata safely falls back to direct discovery");
    model.refresh(nullptr);
    check(model.tile({1, 3}) == MapTile::Floor && model.tile({-1, 1}) == MapTile::Hidden,
          "Non-discovery maps show full terrain and out-of-map cells stay dark");
}
void framing() {
    auto view = MapGridView::fit(12, 10, {1, 1}, 8, 42, 370, 254);
    check(view.first == Cell{0, 0} && view.columns == 12 && view.rows == 10 && view.x == 157 && view.y == 139,
          "Small maps center independently on both axes");
    view = MapGridView::fit(100, 10, {0, 1}, 8, 42, 370, 254);
    check(view.columns == 61 && view.first.x == -30 && view.rows == 10 && view.first.y == 0 &&
              view.left({0, 1}) + 3 == 193 && view.y == 139,
          "Wide maps keep the party centered even at a map edge while fitting the short axis");
    view = MapGridView::fit(100, 100, {99, 99}, 8, 42, 370, 254);
    check(view.columns == 61 && view.rows == 41 && view.first == Cell{69, 79} &&
              view.left({99, 99}) + 3 == 193 && view.top({99, 99}) + 3 == 169,
          "Oversized axes keep fixed cell size and center the party without edge clamping");
    check(!view.contains({68, 99}) && !view.contains({99, 120}) && view.contains({69, 79}),
          "Whole-cell crop rejects both offscreen axes");
    std::vector<std::string> rows(45, std::string(75, '#'));
    for (int y = 1; y < 44; ++y)
        for (int x = 1; x < 74; ++x)
            rows[y][x] = '.';
    const Map large(rows);
    MapOverlayModel model;
    model.prepare(large);
    model.refresh(nullptr);
    check(model.tile({73, 43}) == MapTile::Floor,
          "Large non-discovery maps are not truncated to discovery mask capacity");
}
void keyMemory() {
    const auto catalog = fixtureCatalog();
    const auto map = fixtureMap();
    const NpcWorld empty(map, {2, 3}, {"goblin"}, {});
    auto game =
        std::make_unique<GameSession>(map, catalog, empty, 5, Player{{2, 3}, Heading::East}, false,
                                      std::vector<NpcTimings>{}, RecoveryLevel{}, fixtureDefinitions());
    game->setAnimated(false);
    game->move(Action::TurnLeft);
    check(mapFixtureMarker(game->fixtures(), 1, false) == MapMarker::Key,
          "Observed uncollected key has a map pictogram");
    check(game->interact(fixtureRequest(*game, 1)).changed &&
              mapFixtureMarker(game->fixtures(), 1, false) == MapMarker::None,
          "Collected keys disappear from the floor map");
}
} // namespace
int main() {
    try {
        roomKnowledge();
        corridorAndFallback();
        framing();
        keyMemory();
        std::cout << "Floor map checks passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}

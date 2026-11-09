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
    check(model.fixtureMarker(game->fixtures(), 0, false) == MapMarker::ClosedDoor &&
              model.fixtureMarker(game->fixtures(), 1, false) == MapMarker::None,
          "Known door and hidden stairs retain existing fixture knowledge");
    check(game->interact(fixtureRequest(*game, 0)).changed, "Owned test opens the blocking door");
    model.refresh(&game->fixtures());
    check(game->fixtures().discovered({6, 3}) && model.tile({9, 5}) == MapTile::Floor,
          "Opening the door supplies real LOS and reveals the next room");
    check(model.fixtureMarker(game->fixtures(), 0, false) == MapMarker::OpenDoor,
          "Open door uses remembered open-door pictogram");
    check(game->interact(fixtureRequest(*game, 0)).changed, "Owned test recloses the door");
    model.refresh(&game->fixtures());
    check(model.tile({9, 5}) == MapTile::Floor &&
              model.fixtureMarker(game->fixtures(), 0, false) == MapMarker::ClosedDoor,
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
              model.fixtureMarker(gateGame->fixtures(), 0, false) == MapMarker::GuardianGate,
          "New world identity invalidates cache and a seen guardian gate blocks room revelation");
    game->exportFloor(*saved);
    saved->fixtures.open[0] = true;
    restored->importFloor(*saved);
    model.refresh(&restored->fixtures());
    check(model.fixtureMarker(restored->fixtures(), 0, false) == MapMarker::ClosedDoor &&
              model.fixtureMarker(restored->fixtures(), 0, true) == MapMarker::OpenDoor &&
              model.fixtureMarker(restored->fixtures(), fixtureCapacity, true) == MapMarker::None,
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
void revealedFixtures() {
    const auto map = roomsMap();
    const auto layout = membership(map);
    auto defs = definitions();
    defs.entries[defs.count++] = {{1, 3}, Heading::West, FixtureKind::Upstairs};
    defs.entries[defs.count++] = {{1, 1}, Heading::North, FixtureKind::Key, 2};
    defs.entries[defs.count++] = {{1, 5}, Heading::South, FixtureKind::Door, 2};
    defs.entries[defs.count++] = {{3, 1}, Heading::North, FixtureKind::Endpoint};
    defs.entries[defs.count++] = {{3, 5}, Heading::South, FixtureKind::GuardianGate};
    // Spawn on the stair landing, looking into the room rather than back at the stairs.
    const auto catalog = fixtureCatalog();
    const NpcWorld guards(map, {2, 3}, {"goblin"}, {{{0}, {8, 5}, {0, 0}, Heading::South}});
    FloorObjectiveDefinition objective;
    objective.count = 1;
    objective.members[0] = 0;
    objective.approachDoor = 0;
    objective.gate = 6;
    auto game = std::make_unique<GameSession>(map, catalog, guards, 5, Player{{2, 3}, Heading::East}, false,
                                             std::vector<NpcTimings>{}, RecoveryLevel{}, defs, objective);
    const auto& floor = game->fixtures();
    check(!floor.known(2) && !floor.discovered({1, 3}), "Arrival has not seen the upstairs behind it");
    MapOverlayModel model;
    model.prepare(map, &layout);
    model.refresh(&floor);
    constexpr MapMarker expected[]{MapMarker::ClosedDoor, MapMarker::None, MapMarker::Upstairs,
                                    MapMarker::Key, MapMarker::ClosedDoor, MapMarker::Objective,
                                    MapMarker::GuardianGate};
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < defs.count; ++i)
        check(model.fixtureMarker(floor, i) == expected[i], "Every mapped fixture has its room-revealed icon");
    check(model.fixtureMarker(floor, 1, true) == MapMarker::Downstairs,
          "Full map reveals downstairs beyond the closed threshold");
    watching = false;
    check(allocations == 0 && !floor.known(2), "Marker queries allocate nothing and do not grant sight knowledge");
    auto saved = std::make_unique<FloorState>();
    game->exportFloor(*saved);
    const auto digest = saved->digest();
    auto restored = std::make_unique<GameSession>(map, catalog, guards, 5, Player{{2, 3}, Heading::East}, false,
                                                 std::vector<NpcTimings>{}, RecoveryLevel{}, defs, objective);
    restored->importFloor(*saved);
    MapOverlayModel restoredModel;
    restoredModel.prepare(map, &layout);
    restoredModel.refresh(&restored->fixtures());
    for (unsigned i = 0; i < defs.count; ++i)
        check(restoredModel.fixtureMarker(restored->fixtures(), i) == expected[i],
              "Persisted discovery reconstructs room fixture icons without turning to see them");
    game->exportFloor(*saved);
    check(saved->digest() == digest, "Room-revealed markers leave saved simulation state unchanged");
    // Import a changed world state through the preparation boundary; leave sight memory unchanged.
    saved->fixtures.completed[3] = saved->fixtures.completed[4] = saved->fixtures.completed[6] = true;
    saved->fixtures.open[4] = true;
    game->importFloor(*saved);
    check(model.fixtureMarker(floor, 3) == MapMarker::None &&
              model.fixtureMarker(floor, 4) == MapMarker::OpenDoor &&
              model.fixtureMarker(floor, 6) == MapMarker::OpenDoor,
          "Never-observed mapped fixtures use current completion/open state without a discovery refresh");
    const FloorGeometry other(map, defs);
    check(model.fixtureMarker(other, 2) == MapMarker::None, "A model cannot expose another floor's markers");
    model.prepare(map);
    model.refresh(&floor);
    check(model.fixtureMarker(floor, 2) == MapMarker::None,
          "Without room metadata an unseen corridor fixture stays hidden");
}
void framing() {
    auto view = MapGridView::fit({{0, 0}, 12, 10}, {1, 1}, 8, 42, 370, 254);
    check(view.first == Cell{0, 0} && view.columns == 12 && view.rows == 10 && view.x == 139 && view.y == 124,
          "Small maps center independently on both axes");
    for (int y = 0; y < 10; ++y)
        for (int x = 0; x < 12; ++x) {
            const auto moved = MapGridView::fit({{0, 0}, 12, 10}, {x, y}, 8, 42, 370, 254);
            check(moved.first == view.first && moved.x == view.x && moved.y == view.y,
                  "A fitting map never scrolls as the party moves to any cell");
        }
    view = MapGridView::fit({{0, 0}, 100, 10}, {0, 1}, 8, 42, 370, 254);
    check(view.columns == 41 && view.first.x == 0 && view.rows == 10 && view.first.y == 0 && view.x == 8.5f &&
              view.y == 124,
          "Wide maps stop at the left edge while centering the fitting vertical axis");
    view = MapGridView::fit({{0, 0}, 12, 100}, {11, 99}, 8, 42, 370, 254);
    check(view.first == Cell{0, 72} && view.columns == 12 && view.rows == 28 && view.x == 139,
          "Tall maps clamp vertically while the fitting horizontal axis remains centered");
    view = MapGridView::fit({{0, 0}, 100, 100}, {99, 99}, 8, 42, 370, 254);
    check(view.columns == 41 && view.rows == 28 && view.first == Cell{59, 72} &&
              view.left({99, 99}) + 9 == 377.5f && view.top({99, 99}) + 9 == 295,
          "Bottom-right camera uses all whole cells and stops at both terrain edges");
    check(!view.contains({58, 99}) && !view.contains({99, 100}) && view.contains({59, 72}),
          "Whole-cell crop rejects both offscreen axes");
    for (int y = 0; y < 100; ++y)
        for (int x = 0; x < 100; ++x) {
            view = MapGridView::fit({{0, 0}, 100, 100}, {x, y}, 8, 42, 370, 254);
            check(view.first.x >= 0 && view.first.y >= 0 && view.first.x + view.columns <= 100 &&
                      view.first.y + view.rows <= 100 && view.contains({x, y}),
                  "Tracking across the level never exposes outside cells or loses the party");
            if (x >= 20 && x <= 79)
                check(view.left({x, y}) + 4.5f == 193,
                      "Horizontal tracking centers the party away from edges");
            if (y >= 14 && y <= 85)
                check(view.top({x, y}) + 4.5f == 173.5f,
                      "Even row counts track within half a cell of the canvas center");
        }
    view = MapGridView::fit({{0, 0}, 100, 100}, {99, 0}, 8, 42, 370, 254);
    check(view.first == Cell{59, 0}, "Top-right corner clamps both axes");
    view = MapGridView::fit({{0, 0}, 100, 100}, {0, 99}, 8, 42, 370, 254);
    check(view.first == Cell{0, 72}, "Bottom-left corner clamps both axes");
    view = MapGridView::fit({{7, 13}, 41, 28}, {47, 40}, 8, 42, 370, 254);
    check(view.first == Cell{7, 13} && view.columns == 41 && view.rows == 28 && view.x == 8.5f &&
              view.y == 43,
          "Exact fits retain even capacity and center fixed nonzero world bounds");
    view = MapGridView::fit({{7, 13}, 100, 100}, {106, 13}, 8, 42, 370, 254);
    check(view.first == Cell{66, 13}, "Oversized offset bounds clamp in world coordinates");
    view = MapGridView::fit({{0, 0}, 100, 100}, {99, 99}, 8, 42, 8, 0);
    check(view.columns == 0 && view.rows == 0 && !view.contains({99, 99}),
          "A viewport smaller than one cell has no drawable cells");
    view = MapGridView::fit({}, {}, 8, 42, 370, 254);
    check(view.columns == 0 && view.rows == 0, "Empty bounds do not produce drawable cells");
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
void paddedBounds() {
    std::vector<std::string> rows(36, std::string(36, '#'));
    for (int y = 8; y <= 17; ++y)
        for (int x = 11; x <= 22; ++x)
            rows[y][x] = '.';
    const Map map(rows);
    MapOverlayModel model;
    model.prepare(map);
    const auto bounds = model.bounds();
    check(bounds.first == Cell{10, 7} && bounds.width == 14 && bounds.height == 12,
          "Camera bounds include the dungeon wall ring but exclude unused storage padding");
    const auto before = MapGridView::fit(bounds, {11, 8}, 8, 42, 370, 254);
    const auto after = MapGridView::fit(bounds, {22, 17}, 8, 42, 370, 254);
    check(before.first == after.first && before.x == after.x && before.y == after.y && before.x == 130 &&
              before.y == 115,
          "A padded dungeon that fits stays centered at opposite party corners");
    FloorFixtures fixtures;
    fixtures.discovery = true;
    FloorGeometry floor(map, fixtures);
    allocations = 0;
    watching = true;
    model.refresh(&floor);
    const auto hidden = model.bounds();
    model.refresh(nullptr);
    const auto full = model.bounds();
    const auto fitted = MapGridView::fit(full, {22, 17}, 8, 42, 370, 254);
    watching = false;
    check(allocations == 0 && hidden.first == full.first && hidden.width == full.width &&
              hidden.height == full.height && fitted.x == before.x && fitted.y == before.y,
          "Discovery/full-map changes and repeated fitting do not move bounds or allocate");
    const Map edge({"###", "#.#", "###"});
    model.prepare(edge);
    check(model.bounds().first == Cell{0, 0} && model.bounds().width == 3 && model.bounds().height == 3,
          "Boundary ring clips to actual storage edges");
    const Map solid({"###", "###"});
    model.prepare(solid);
    check(model.bounds().first == Cell{0, 0} && model.bounds().width == 3 && model.bounds().height == 2,
          "All-solid maps retain stable fallback bounds");
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
    MapOverlayModel model;
    model.prepare(map);
    model.refresh(&game->fixtures());
    check(model.fixtureMarker(game->fixtures(), 1, false) == MapMarker::Key,
          "Observed uncollected key has a map pictogram");
    check(game->interact(fixtureRequest(*game, 1)).changed, "Collect observed key");
    model.refresh(&game->fixtures());
    check(model.fixtureMarker(game->fixtures(), 1, false) == MapMarker::None,
          "Collected keys disappear from the floor map");
}
} // namespace
int main() {
    try {
        roomKnowledge();
        corridorAndFallback();
        revealedFixtures();
        framing();
        paddedBounds();
        keyMemory();
        std::cout << "Floor map checks passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}

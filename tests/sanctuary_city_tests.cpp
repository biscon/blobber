#include "game/movement.hpp"
#include "game/perception.hpp"
#include "support/check.hpp"
#include "world/sanctuary_city.hpp"
#include <iostream>
using namespace blobber;
int main() try {
    const auto city = sanctuaryCity();
    validateSanctuaryCity(city);
    check(city.map.width() == 24 && city.map.height() == 24 && city.doorCount == 12,
          "Complete ten-building town plus dungeon and gate");
    check(city.map.connected(city.spawn.cell), "Every street is connected");
    check(city.map.definition().scenery.size() == 6 && city.map.movementBlocked({4, 4}) &&
              !city.map.solid({4, 4}) && city.map.canStep({3, 5}, {4, 5}) &&
              lineOfSight(city.map, {3, 4}, {5, 4}),
          "Planting bases preserve adjacent walking and cell sight");
    check(city.map.definition().buildings.size() <= 96, "Town stays within content budget");
    for (unsigned i = 0; i < city.doorCount; ++i) {
        const auto p = city.doors[i].approach;
        check(!city.map.canStep(p.cell, p.cell + forward(p.heading)), "Closed entrances block entry");
        check(!lineOfSight(city.map, p.cell, p.cell + forward(p.heading) * 2),
              "Building and gate backing remain opaque");
    }
    for (int x = 4; x <= 6; ++x) {
        check(city.map.solid({x, 9}) && city.map.solid({x, 11}), "Alley is exactly one cell wide");
        check(city.map.canStep({x, 10}, {x + 1, 10}), "Narrow alley allows walking through");
    }
    for (int x = 11; x <= 13; ++x) {
        check(city.map.canStep({x, 18}, {x, 19}) && city.map.canStep({x, 19}, {x, 20}),
              "Gate has a clear three-cell approach");
        check(!city.map.canStep({x, 20}, {x, 21}), "Entire gate opening is sealed");
    }
    for (int y = 9; y <= 14; ++y)
        for (int x = 8; x <= 13; ++x)
            check(city.map.movementBlocked({x, y}) == (x >= 10 && x <= 11 && y >= 11 && y <= 12),
                  "Only fountain base occupies its two-cell circulation margin");
    for (unsigned i = 0; i < city.reservationCount; ++i) {
        const auto& r = city.reservations[i];
        for (unsigned y = 0; y < r.depth; ++y)
            for (unsigned x = 0; x < r.width; ++x)
                check(city.map.movementBlocked({r.origin.x + int(x), r.origin.y + int(y)}) &&
                          !city.map.solid({r.origin.x + int(x), r.origin.y + int(y)}),
                      "Dressing fills reserved bases without changing terrain sight");
    }
    check(city.map.canStep({15, 4}, {16, 4}) && city.map.canStep({15, 5}, {16, 5}) &&
              city.map.canStep({15, 5}, {15, 6}),
          "Dungeon entrance leaves two clear cells beside the inn");
    for (int x : {8, 9})
        check(city.map.movementBlocked({x, 15}) && city.map.canStep({x, 13}, {x, 14}) &&
                  city.map.canStep({x, 16}, {x, 17}),
              "Compact stalls face the open square and preserve southern circulation");
    for (int y : {11, 12})
        check(city.map.canStep({19, y}, {20, y}) && city.map.canStep({20, y}, {21, y}),
              "Former market alley is open");
    FloorFixtures future;
    future.count = 1;
    future.entries[0] = {city.dungeonFixture, city.dungeonLanding.heading, FixtureKind::Downstairs};
    future.landingCount = 1;
    future.landings[0] = city.dungeonLanding.cell;
    future.npcExcluded.set(std::size_t(city.dungeonFixture.y * city.map.width() + city.dungeonFixture.x));
    future.npcExcluded.set(
        std::size_t(city.dungeonLanding.cell.y * city.map.width() + city.dungeonLanding.cell.x));
    const FloorGeometry futureGeometry(city.map, future);
    check(futureGeometry.view().movementBlocked(city.dungeonFixture) &&
              !futureGeometry.view().movementBlocked(city.dungeonLanding.cell) &&
              stairLanding(future.entries[0]) == city.dungeonLanding.cell &&
              fixtureReachable(future.entries[0], city.dungeonLanding.cell, city.dungeonLanding.heading),
          "Reserved dungeon cells admit an existing stair fixture without changing the map");
    auto bad = city;
    bad.doorCount = cityDoorwayCapacity + 1;
    rejects([&] { validateSanctuaryCity(bad); }, "Door count overflow rejects");
    bad = city;
    bad.reservationCount = cityReservationCapacity + 1;
    rejects([&] { validateSanctuaryCity(bad); }, "Reservation count overflow rejects");
    bad = city;
    bad.doors[1].name = bad.doors[0].name;
    rejects([&] { validateSanctuaryCity(bad); }, "Duplicate doorway identity rejects");
    bad = city;
    bad.doors[0].placement = bad.doors[1].placement;
    rejects([&] { validateSanctuaryCity(bad); }, "Approach must meet its own building");
    bad = city;
    bad.doors[0].approach.heading = Heading::South;
    rejects([&] { validateSanctuaryCity(bad); }, "Outward-facing approach rejects");
    bad = city;
    bad.reservations[1] = {"overlap", {10, 11}, 1, 1};
    rejects([&] { validateSanctuaryCity(bad); }, "Overlapping future bases reject");
    bad = city;
    bad.reservations[1] = {"blocking", {11, 19}, 1, 1};
    rejects([&] { validateSanctuaryCity(bad); }, "Gate approach remains reserved");
    bad = city;
    bad.reservations[1] = {"blocking", city.rest.cell, 1, 1};
    rejects([&] { validateSanctuaryCity(bad); }, "Future scenery cannot occupy rest anchor");
    bad = city;
    bad.reservations[1] = {"blocking", {3, 6}, 1, 1};
    rejects([&] { validateSanctuaryCity(bad); }, "Named loop must remain traversable");
    bad = city;
    // Cut off the otherwise walkable pocket between the western wall and a house.
    bad.reservations[1] = {"blocking", {2, 7}, 2, 3};
    bad.reservations[bad.reservationCount++] = {"blocking-north", {2, 5}, 5, 1};
    rejects([&] { validateSanctuaryCity(bad); }, "Future scenery cannot break town circulation");
    bad = city;
    bad.reservations[0].origin = {23, 23};
    rejects([&] { validateSanctuaryCity(bad); }, "Out-of-bounds reservation rejects");
    bad = city;
    bad.reservations[2] = {"misplaced-planting", {3, 3}, 1, 1};
    rejects([&] { validateSanctuaryCity(bad); }, "Installed base must belong to a dressing reservation");
    bad = city;
    bad.dungeonFixture = {14, 5};
    rejects([&] { validateSanctuaryCity(bad); }, "Future stair cannot occupy the solid building shell");
    bad = city;
    bad.spawn.cell = {2, 20};
    rejects([&] { validateSanctuaryCity(bad); }, "Convenient arrival walking distances enforced");
    rejects([&] { city.doorway("missing"); }, "Missing authored identity rejects");
    std::cout << "Sanctuary city layout, reservations and failure checks passed\n";
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}

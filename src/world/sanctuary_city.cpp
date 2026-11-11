#include "world/sanctuary_city.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>
namespace blobber {
namespace {
constexpr int townSize = 24;
void require(bool condition, const std::string& message) {
    if (!condition)
        throw std::invalid_argument("Sanctuary city: " + message);
}
bool inside(const Map& map, Cell c) {
    return c.x > 0 && c.y > 0 && c.x < map.width() - 1 && c.y < map.height() - 1;
}
std::vector<int> distances(const SanctuaryCity& city, Cell start, bool dressed) {
    const auto& map = city.map;
    std::vector<int> result(std::size_t(map.width()) * map.height(), -1);
    auto blocked = [&](Cell c) {
        if (map.movementBlocked(c) || (dressed && c == city.dungeonFixture))
            return true;
        for (unsigned i = 0; dressed && i < city.reservationCount; ++i)
            if (city.reservations[i].contains(c))
                return true;
        return false;
    };
    if (blocked(start))
        return result;
    std::vector<Cell> todo{start};
    auto index = [&](Cell c) { return std::size_t(c.y * map.width() + c.x); };
    result[index(start)] = 0;
    for (std::size_t i = 0; i < todo.size(); ++i)
        for (unsigned h = 0; h < 4; ++h) {
            const auto c = todo[i] + forward(Heading(h));
            if (!blocked(c) && map.canStep(todo[i], c) && result[index(c)] < 0) {
                result[index(c)] = result[index(todo[i])] + 1;
                todo.push_back(c);
            }
        }
    return result;
}
} // namespace
const CityDoorway& SanctuaryCity::doorway(std::string_view name) const {
    for (unsigned i = 0; i < doorCount && i < doors.size(); ++i)
        if (doors[i].name && name == doors[i].name)
            return doors[i];
    throw std::invalid_argument("Unknown city doorway: " + std::string(name));
}
void validateSanctuaryCity(const SanctuaryCity& city) {
    const auto& map = city.map;
    const auto& buildings = map.definition().buildings;
    require(city.doorCount <= city.doors.size() && city.reservationCount <= city.reservations.size(),
            "metadata capacity exceeded");
    require(buildings.size() <= 96, "SC02 placement budget exceeded");
    for (const auto& s : map.definition().scenery)
        for (unsigned z = 0; z < s.depth; ++z)
            for (unsigned x = 0; x < s.width; ++x) {
                bool reserved = false;
                for (unsigned i = 0; i < city.reservationCount; ++i)
                    reserved |= city.reservations[i].contains(s.cell(x, z));
                require(reserved, "installed scenery leaves dressing reservations");
            }
    require(map.connected(city.spawn.cell), "streets must connect");
    for (unsigned i = 0; i < city.doorCount; ++i) {
        const auto& d = city.doors[i];
        require(d.name && *d.name, "doorway has no identity");
        require(d.placement < buildings.size() && unsigned(d.approach.heading) < 4,
                std::string(d.name) + " has invalid placement/heading");
        for (unsigned j = 0; j < i; ++j)
            require(std::string_view(d.name) != city.doors[j].name, "duplicate doorway identity");
        const auto& b = buildings[d.placement];
        require(inside(map, d.approach.cell) && !map.movementBlocked(d.approach.cell),
                std::string(d.name) + " approach is blocked");
        const auto facing = forward(d.approach.heading), outward = forward(b.facing);
        require(facing.x == -outward.x && facing.y == -outward.y,
                std::string(d.name) + " approach faces away from facade");
        bool owns = false;
        for (unsigned z = 0; z < b.depth; ++z)
            for (unsigned x = 0; x < b.width; ++x)
                owns |= b.cell(x, z) == d.approach.cell + facing;
        require(owns, std::string(d.name) + " approach does not meet its own facade");
    }
    auto rect = [&](const CityReservation& r) {
        require(r.name && *r.name && r.width && r.depth && r.width <= 24 && r.depth <= 24 &&
                    inside(map, r.origin),
                "invalid named reservation");
        require(inside(map, r.origin + Cell{int(r.width) - 1, int(r.depth) - 1}),
                std::string(r.name) + " is outside the town");
    };
    rect(city.gateApproach);
    rect(city.alley);
    for (unsigned i = 0; i < city.reservationCount; ++i) {
        const auto& r = city.reservations[i];
        rect(r);
        for (unsigned j = 0; j < i; ++j)
            require(std::string_view(r.name) != city.reservations[j].name, "duplicate reservation identity");
        for (unsigned y = 0; y < r.depth; ++y)
            for (unsigned x = 0; x < r.width; ++x) {
                const Cell c = r.origin + Cell{int(x), int(y)};
                require(!map.solid(c), std::string(r.name) + " overlaps architecture");
                require(!city.gateApproach.contains(c) && !city.alley.contains(c),
                        std::string(r.name) + " occupies a protected route");
                require(!(c == city.spawn.cell) && !(c == city.recallArrival.cell) &&
                            !(c == city.dungeonLanding.cell) && !(c == city.dungeonFixture),
                        std::string(r.name) + " occupies a protected anchor");
                for (unsigned d = 0; d < city.doorCount; ++d)
                    require(!(c == city.doors[d].approach.cell), std::string(r.name) + " blocks a doorway");
                for (unsigned j = 0; j < i; ++j)
                    require(!city.reservations[j].contains(c), "overlapping dressing reservations");
            }
    }
    const auto future = distances(city, city.spawn.cell, true);
    auto reachable = [&](Cell c) {
        return inside(map, c) && future[std::size_t(c.y * map.width() + c.x)] >= 0;
    };
    for (int y = 1; y < map.height() - 1; ++y)
        for (int x = 1; x < map.width() - 1; ++x) {
            bool reserved = Cell{x, y} == city.dungeonFixture;
            for (unsigned i = 0; i < city.reservationCount; ++i)
                reserved |= city.reservations[i].contains({x, y});
            require(map.movementBlocked({x, y}) || reserved || reachable({x, y}),
                    "future dressing disconnects streets");
        }
    for (const auto* r : {&city.gateApproach, &city.alley})
        for (unsigned y = 0; y < r->depth; ++y)
            for (unsigned x = 0; x < r->width; ++x)
                require(reachable(r->origin + Cell{int(x), int(y)}), std::string(r->name) + " is blocked");
    for (const auto& loop : city.loops) {
        require(loop.name && loop.corners.front() == loop.corners.back(), "loop must close");
        for (unsigned i = 1; i < loop.corners.size(); ++i) {
            auto c = loop.corners[i - 1];
            const auto end = loop.corners[i];
            require(inside(map, c) && inside(map, end) && ((c.x == end.x) != (c.y == end.y)),
                    "loop legs must be nonempty cardinal segments");
            const Cell step{(end.x > c.x) - (end.x < c.x), (end.y > c.y) - (end.y < c.y)};
            require(reachable(c), std::string(loop.name) + " is blocked");
            while (!(c == end)) {
                require(map.canStep(c, c + step) && reachable(c + step),
                        std::string(loop.name) + " is blocked");
                c = c + step;
            }
        }
    }
    require(reachable(city.recallArrival.cell) && reachable(city.dungeonLanding.cell) &&
                reachable(city.gate.cell),
            "landmark approach is unreachable");
    require(city.recallArrival.cell == city.doorway("sanctuary").approach.cell &&
                city.dungeonFixture == city.doorway("dungeon").approach.cell &&
                city.gate.cell == city.doorway("city-gate").approach.cell,
            "landmark anchors differ from their doorways");
    require(city.dungeonFixture == city.dungeonLanding.cell + forward(city.dungeonLanding.heading) &&
                map.canStep(city.dungeonLanding.cell, city.dungeonFixture) &&
                city.dungeonLanding.heading == city.doorway("dungeon").approach.heading,
            "future stair requires a clear cell and level adjacent landing");
    auto distance = [&](const std::vector<int>& d, Cell c) {
        return d[std::size_t(c.y * map.width() + c.x)];
    };
    require(distance(future, city.recallArrival.cell) <= 6 &&
                distance(future, city.dungeonLanding.cell) <= 6 &&
                distance(distances(city, city.recallArrival.cell, true), city.dungeonLanding.cell) <= 8,
            "arrival/rest/dungeon routes exceed walking budget");
}
SanctuaryCity sanctuaryCity() {
    std::vector<std::string> rows(townSize, std::string(townSize, '.'));
    for (int i = 0; i < townSize; ++i)
        rows[0][i] = rows[23][i] = rows[i][0] = rows[i][23] = '#';
    WorldDefinition world;
    world.assetPack = "sanctuary-city";
    world.surfaces.resize(townSize * townSize);
    for (auto& s : world.surfaces) {
        s.ceiling = noCeiling;
        s.environment = Environment::Daylight;
    }
    std::array<CityDoorway, cityDoorwayCapacity> doors{};
    unsigned count = 0;
    auto house = [&](const char* id, const char* model, Cell approach, Heading facing, unsigned depth = 3) {
        const auto h = Heading((unsigned(facing) + 2) % 4);
        doors[count++] = {id, unsigned(world.buildings.size()), {approach, h}};
        world.buildings.push_back(
            {model, approach + forward(facing) * -int(depth - 1), {-1, -int(depth - 2)}, facing, 3, depth});
    };
    house("sanctuary", "sanctuary", {9, 6}, Heading::South);
    house("dungeon", "dungeon-entrance", {13, 6}, Heading::South);
    house("inn", "inn", {16, 5}, Heading::West, 4);
    house("emporium", "provisioner", {16, 9}, Heading::West);
    house("smithy", "smithy", {15, 12}, Heading::West);
    house("west-home", "low-house", {7, 8}, Heading::East);
    house("alley-home", "low-house-hip-ivy", {7, 12}, Heading::East);
    house("courtyard-home", "tall-house-ivy", {7, 17}, Heading::East, 4);
    house("southwest-home", "low-house-hip", {8, 18}, Heading::North);
    house("southeast-home", "low-house", {16, 18}, Heading::North);
    house("east-home", "tall-house", {20, 14}, Heading::North, 4);
    doors[count++] = {"city-gate", unsigned(world.buildings.size()), {{12, 20}, Heading::South}};
    world.buildings.push_back({"city-gate", {12, 21}, {-2, 0}, Heading::North, 5, 2});
    // Complete modeled enclosure inside the outermost immutable solid ring.
    for (int y = 1; y <= 22; ++y)
        for (int x = 1; x <= 22; ++x)
            if ((x == 1 || x == 22 || y == 1 || y == 22) && !(y == 22 && x >= 10 && x <= 14))
                world.buildings.push_back({"boundary-wall", {x, y}, {0, 0}, Heading::North, 1, 1});
    for (const auto& b : world.buildings)
        for (unsigned z = 0; z < b.depth; ++z)
            for (unsigned x = 0; x < b.width; ++x) {
                const auto c = b.cell(x, z);
                rows[c.y][c.x] = '#';
            }
    // SC03 dressing and user-reviewed square layout.
    world.scenery = {
        {"courtyard-garden", {4, 4}, {-1, -1}, Heading::North, 3, 2},
        {"lane-garden", {4, 15}, {0, 0}, Heading::North, 2, 1},
        {"east-garden", {20, 19}, {0, 0}, Heading::North, 2, 2},
        {"market-produce", {8, 15}, {0, 0}, Heading::North, 1, 1},
        {"market-grain", {9, 15}, {0, 0}, Heading::North, 1, 1},
        {"fountain", {10, 11}, {0, 0}, Heading::North, 2, 2},
    };
    SanctuaryCity result{Map(rows, std::move(world)), {{11, 8}, Heading::West}};
    result.doors = doors;
    result.doorCount = count;
    result.reservations[0] = {"fountain", {10, 11}, 2, 2};
    result.reservations[1] = {"market", {8, 15}, 2, 1};
    result.reservations[2] = {"courtyard-planting", {3, 3}, 3, 2};
    result.reservations[3] = {"lane-planting", {4, 15}, 2, 1};
    result.reservations[4] = {"east-planting", {20, 19}, 2, 2};
    result.reservationCount = 5;
    result.recallArrival = result.doorway("sanctuary").approach;
    result.dungeonFixture = result.doorway("dungeon").approach.cell;
    result.dungeonLanding = {result.dungeonFixture + forward(result.doorway("dungeon").approach.heading) * -1,
                             result.doorway("dungeon").approach.heading};
    result.gate = result.doorway("city-gate").approach;
    result.gateApproach = {"gate-approach", {11, 18}, 3, 3};
    result.alley = {"residential-alley", {4, 10}, 3, 1};
    result.loops = {{{"residential-loop", {{{7, 6}, {7, 14}, {3, 14}, {3, 6}, {7, 6}}}},
                     {"service-loop", {{{15, 7}, {21, 7}, {21, 14}, {15, 14}, {15, 7}}}}}};
    validateSanctuaryCity(result);
    return result;
}
} // namespace blobber

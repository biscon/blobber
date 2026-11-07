#include "game/emporium_interior.hpp"
#include "game/campaign.hpp"
#include <stdexcept>
namespace blobber {
SanctuaryHub emporiumInterior() {
    constexpr int size = 12;
    std::vector<std::string> rows(size, std::string(size, '#'));
    for (int y = 2; y <= 9; ++y)
        for (int x = 2; x <= 9; ++x)
            rows[y][x] = '.';
    WorldDefinition world;
    world.assetPack = "sanctuary-emporium";
    world.surfaces.resize(size * size);
    for (auto& surface : world.surfaces) {
        surface.ceiling = 6;
        surface.environment = Environment::Daylight;
    }
    for (int x = 2; x <= 9; ++x) {
        world.buildings.push_back({"wall", {x, 1}, {0, 0}, Heading::South, 1, 1});
        world.buildings.push_back({x == 5 ? "exit-door" : "wall", {x, 10}, {0, 0}, Heading::North, 1, 1});
    }
    for (int y = 2; y <= 9; ++y) {
        world.buildings.push_back({"wall", {1, y}, {0, 0}, Heading::East, 1, 1});
        world.buildings.push_back({y == 5 ? "window-wall" : "wall", {10, y}, {0, 0}, Heading::West, 1, 1});
    }
    world.scenery = {
        {"potion-cabinet", {2, 3}, {-1, 0}, Heading::East, 3, 1},
        {"scroll-cabinet", {2, 7}, {-1, 0}, Heading::East, 3, 1},
        {"rear-shelves", {6, 2}, {-1, 0}, Heading::South, 3, 1},
        {"counter", {6, 4}, {-1, 0}, Heading::South, 3, 1},
        {"alchemy-bench", {9, 6}, {0, 0}, Heading::West, 3, 1},
    };
    const Player entry{{5, 9}, Heading::North}, exit{{5, 9}, Heading::South};
    SanctuaryHub result{Map(rows, std::move(world)), entry, entry, exit, {}, {}};
    result.fixtures.discovery = true;
    result.fixtures.landingCount = 1;
    result.fixtures.landings[0] = entry.cell;
    result.fixtures.npcExcluded.set(std::size_t(entry.cell.y * size + entry.cell.x));
    if (!result.map.connected(entry.cell))
        throw std::invalid_argument("Emporium furniture disconnects walking routes");
    return result;
}
void validateEmporiumInterior(const Map& map, const CampaignLocationState& state) {
    const auto expected = emporiumInterior();
    const auto& f = state.fixtures;
    const auto& e = expected.fixtures;
    if (map.width() != expected.map.width() || map.height() != expected.map.height() ||
        map.definition().fingerprint() != expected.map.definition().fingerprint() ||
        !(state.entry == expected.entry) || !(state.exit == expected.exit) || !state.placements.empty() ||
        state.objective.count || state.objective.rewardCount || state.recovery.groundCount ||
        state.recovery.sanctuaryCount || state.recovery.freeRestEverywhere || f.count || f.depth ||
        !f.discovery || f.landingCount != e.landingCount || f.landings != e.landings ||
        f.npcExcluded != e.npcExcluded)
        throw std::invalid_argument("Unsupported Arcane Emporium definition");
    for (int y = 0; y < map.height(); ++y)
        for (int x = 0; x < map.width(); ++x)
            if (map.solid({x, y}) != expected.map.solid({x, y}))
                throw std::invalid_argument("Unsupported Arcane Emporium terrain");
}
} // namespace blobber

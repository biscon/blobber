#include "game/inn_interior.hpp"
#include "game/campaign.hpp"
#include <stdexcept>
namespace blobber {
SanctuaryHub innInterior() {
    constexpr int width = 12, height = 12;
    std::vector<std::string> rows(height, std::string(width, '#'));
    for (int y = 2; y <= 9; ++y)
        for (int x = 2; x <= 9; ++x)
            rows[y][x] = '.';
    WorldDefinition world;
    world.assetPack = "sanctuary-inn";
    world.surfaces.resize(width * height);
    for (auto& surface : world.surfaces) {
        surface.ceiling = 6; // 1.5-cell ceiling above the fixed .5-cell eye.
        surface.environment = Environment::Daylight;
    }
    // Wall modules own the solid backing and include the beams/wainscot.
    for (int x = 1; x <= 8; ++x) {
        world.buildings.push_back({"wall", {x, 0}, {0, 0}, Heading::South, 1, 1});
        world.buildings.push_back({x == 4 ? "exit-door" : "wall", {x, 9}, {0, 0}, Heading::North, 1, 1});
    }
    for (int y = 1; y <= 8; ++y) {
        world.buildings.push_back({"wall", {0, y}, {0, 0}, Heading::East, 1, 1});
        world.buildings.push_back({"wall", {9, y}, {0, 0}, Heading::West, 1, 1});
    }
    world.scenery = {
        {"bar", {5, 3}, {-1, 0}, Heading::North, 3, 1},
        {"shelves", {5, 1}, {-1, 0}, Heading::North, 3, 1},
        {"table-group", {2, 5}, {0, 0}, Heading::North, 2, 2},
        {"table-group", {6, 6}, {0, 0}, Heading::North, 2, 2},
        {"hearth", {1, 2}, {0, 0}, Heading::East, 2, 1},
        {"stairs", {8, 2}, {0, 0}, Heading::North, 1, 3},
        {"barrels", {7, 1}, {0, 0}, Heading::North, 1, 1},
        {"barrels", {8, 1}, {0, 0}, Heading::North, 1, 1},
        {"plant", {1, 8}, {0, 0}, Heading::North, 1, 1},
        {"plant", {8, 8}, {0, 0}, Heading::North, 1, 1},
    };
    for (auto& building : world.buildings)
        building.anchor = building.anchor + Cell{1, 1};
    for (auto& scenery : world.scenery)
        scenery.anchor = scenery.anchor + Cell{1, 1};
    world.hearth = AmbientFire{2.62f, .20f, 4.0f};
    const Player entry{{5, 9}, Heading::North}, exit{{5, 9}, Heading::South};
    SanctuaryHub inn{Map(rows, std::move(world)), entry, entry, exit, {}, {}};
    inn.fixtures.discovery = true;
    inn.fixtures.landingCount = 1;
    inn.fixtures.landings[0] = entry.cell;
    inn.fixtures.npcExcluded.set(std::size_t(entry.cell.y * width + entry.cell.x));
    inn.recovery.freeRestEverywhere = true;
    if (!inn.map.connected(entry.cell))
        throw std::invalid_argument("Inn furniture disconnects walking routes");
    return inn;
}
void validateInnInterior(const Map& map, const CampaignLocationState& state) {
    const auto expected = innInterior();
    const auto& f = state.fixtures;
    const auto& e = expected.fixtures;
    bool canonical = map.definition().fingerprint() == expected.map.definition().fingerprint();
    if (!canonical) {
        // Only the previously shipped uniform ceiling is compatible. Validate
        // the entire old definition before the loader adopts today's ceiling.
        auto previous = expected.map.definition();
        for (auto& surface : previous.surfaces)
            surface.ceiling = 8;
        canonical = map.definition().fingerprint() == previous.fingerprint();
    }
    if (map.width() != expected.map.width() || map.height() != expected.map.height() ||
        !canonical ||
        !(state.entry == expected.entry) || !(state.exit == expected.exit) || !state.placements.empty() ||
        state.objective.count || state.recovery.groundCount || state.recovery.sanctuaryCount ||
        !state.recovery.freeRestEverywhere || f.count || f.depth || !f.discovery ||
        f.landingCount != e.landingCount || f.landings != e.landings || f.npcExcluded != e.npcExcluded)
        throw std::invalid_argument("Unsupported Sanctuary inn definition");
    for (int y = 0; y < map.height(); ++y)
        for (int x = 0; x < map.width(); ++x)
            if (map.solid({x, y}) != expected.map.solid({x, y}))
                throw std::invalid_argument("Unsupported Sanctuary inn terrain");
}
} // namespace blobber

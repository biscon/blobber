#include "world/sanctuary_city.hpp"
#include <stdexcept>
namespace blobber {
SanctuaryCityReview sanctuaryCityReview() {
    std::vector<std::string> rows(16, std::string(16, '.'));
    for (int i = 0; i < 16; ++i)
        rows[0][i] = rows[15][i] = rows[i][0] = rows[i][15] = '#';
    WorldDefinition world;
    world.assetPack = "sanctuary-city";
    world.surfaces.resize(256);
    for (auto& s : world.surfaces) {
        s.ceiling = noCeiling;
        s.environment = Environment::Daylight;
    }
    world.buildings = {{"low-house", {5, 5}, {-1, -1}, Heading::South, 3, 3},
                       {"tall-house", {11, 8}, {-1, -2}, Heading::West, 3, 4}};
    // Taller shell is 3 local cells wide by 4 deep, rotated toward the lane.
    for (const auto& b : world.buildings)
        for (unsigned z = 0; z < b.depth; ++z)
            for (unsigned x = 0; x < b.width; ++x) {
                const auto c = b.cell(x, z);
                rows[c.y][c.x] = '#';
            }
    SanctuaryCityReview result{
        Map(rows, std::move(world)),
        {{7, 10}, Heading::North},
        {{{"low-house", {{5, 7}, Heading::North}}, {"tall-house", {{8, 8}, Heading::East}}}}};
    if (!result.map.connected(result.spawn.cell))
        throw std::runtime_error("City review streets must connect");
    for (const auto& door : result.doors)
        if (result.map.movementBlocked(door.approach.cell) ||
            !result.map.buildingOwned(door.approach.cell + forward(door.approach.heading)))
            throw std::runtime_error("City review doorway requires an accessible approach");
    return result;
}
} // namespace blobber

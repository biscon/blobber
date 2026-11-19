#include "assets/current_tile.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace blobber::world_assets {
std::vector<unsigned char> prepareCurrentTile(const std::vector<unsigned char>& source) {
    constexpr unsigned side = WaterCurrent::tileSize;
    if (source.size() != waterSourceSize * waterSourceSize * 4)
        throw std::invalid_argument("Invalid water current source dimensions");
    // In the overlap, transition from the continuation of the far edge back to
    // the near edge. Both the sample and its slope agree at each periodic join.
    auto weight = [](unsigned i) {
        const float t = std::min(float(i) / waterOverlap, 1.f);
        return 1.f - t * t * (3.f - 2.f * t);
    };
    std::vector<unsigned char> tile(side * side * 4, 255);
    for (unsigned y = 0; y < side; ++y)
        for (unsigned x = 0; x < side; ++x)
            for (unsigned c = 0; c < 3; ++c) {
                auto row = [&](unsigned sy) {
                    const auto i = (sy * waterSourceSize + x) * 4 + c;
                    return (1.f - weight(x)) * source[i] +
                           (x < waterOverlap ? weight(x) * source[i + side * 4] : 0.f);
                };
                const float value =
                    (1.f - weight(y)) * row(y) + (y < waterOverlap ? weight(y) * row(y + side) : 0.f);
                tile[(y * side + x) * 4 + c] = static_cast<unsigned char>(std::lround(value));
            }
    return tile;
}
} // namespace blobber::world_assets

#pragma once
#include "render/picking.hpp"
#include "ui/layout.hpp"

namespace blobber {
// Bounded native-UI clipping for 12x12 status tiles and 46x6 health bars.
// Merge equal visibility rows so an unobstructed indicator draws just once.
template <typename Draw>
void forEachDepthClip(const PickingBuffer& picking, std::uint16_t owner, UiRect tile, Draw&& draw) {
    constexpr int maxWidth = 64, maxHeight = 12;
    if (tile.width <= 0 || tile.height <= 0 || tile.width > maxWidth || tile.height > maxHeight)
        return;
    std::array<std::uint64_t, maxHeight> rows{};
    for (int y = 0; y < tile.height; ++y)
        for (int x = 0; x < tile.width; ++x)
            if (picking.badgeVisible(owner, tile.x - viewX + x, tile.y - viewY + y))
                rows[y] |= std::uint64_t(1) << x;
    for (int y = 0; y < tile.height;) {
        int end = y + 1;
        while (end < tile.height && rows[end] == rows[y])
            ++end;
        for (int x = 0; x < tile.width;) {
            if (!(rows[y] & (std::uint64_t(1) << x))) {
                ++x;
                continue;
            }
            const int begin = x++;
            while (x < tile.width && (rows[y] & (std::uint64_t(1) << x)))
                ++x;
            draw(UiRect{tile.x + begin, tile.y + y, x - begin, end - y});
        }
        y = end;
    }
}
template <typename Draw>
void forEachStatusClip(const PickingBuffer& picking, std::uint16_t owner, UiRect tile, Draw&& draw) {
    forEachDepthClip(picking, owner, tile, draw);
}
} // namespace blobber

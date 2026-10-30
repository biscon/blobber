#pragma once
#include "render/picking.hpp"
#include "ui/layout.hpp"

namespace blobber {
// Status tiles are 12 art pixels square. Merge equal visibility rows so the usual
// unobstructed icon needs one draw, while silhouette edges retain pixel accuracy.
template <typename Draw>
void forEachStatusClip(const PickingBuffer& picking, std::uint16_t owner, UiRect tile, Draw&& draw) {
    constexpr int size = 12;
    std::array<std::uint16_t, size> rows{};
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x)
            if (picking.badgeVisible(owner, tile.x - viewX + x, tile.y - viewY + y))
                rows[y] |= std::uint16_t(1u << x);
    for (int y = 0; y < size;) {
        int end = y + 1;
        while (end < size && rows[end] == rows[y])
            ++end;
        for (int x = 0; x < size;) {
            if (!(rows[y] & (1u << x))) {
                ++x;
                continue;
            }
            const int begin = x++;
            while (x < size && (rows[y] & (1u << x)))
                ++x;
            draw(UiRect{tile.x + begin, tile.y + y, x - begin, end - y});
        }
        y = end;
    }
}
} // namespace blobber

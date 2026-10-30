#pragma once
#include "game/loot.hpp"
#include "render/frame.hpp"
#include "render/picking.hpp"
#include <vector>
namespace blobber {
// Every integer square size is baked offline. Runtime only copies/blends prepared pixels.
struct PreparedLoot {
    static constexpr int minSize = 4, maxSize = 64, icons = 7;
    static constexpr std::size_t pixelsPerIcon =
        maxSize * (maxSize + 1) * (2 * maxSize + 1) / 6 - (minSize - 1) * minSize * (2 * minSize - 1) / 6;
    std::vector<Color> pixels;
    std::size_t storageBytes() const noexcept { return pixels.size() * sizeof(Color); }
    void compose(const LootSnapshot&, Player, int phase, const std::array<float, viewWidth>& walls,
                 const ActorWorld*, const NpcAnimationSnapshot*, Frame&, PickingBuffer&,
                 const FloorGeometry* = nullptr) const noexcept;
};
} // namespace blobber

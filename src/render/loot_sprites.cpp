#include "render/loot_sprites.hpp"
#include "render/lighting.hpp"
#include "render/projection.hpp"
#include <algorithm>
#include <cmath>
namespace blobber {
void PreparedLoot::compose(const LootSnapshot& loot, Player player, int phase,
                           const std::array<float, viewWidth>& walls, const ActorWorld* actors,
                           const NpcAnimationSnapshot* animation, Frame& frame,
                           PickingBuffer& picking) const noexcept {
    if (pixels.size() != icons * pixelsPerIcon)
        return;
    picking.lootWorld(loot.world);
    struct Visible {
        std::uint16_t slot;
        float lateral, depth;
    };
    std::array<std::uint16_t, lootCapacity> order{};
    std::size_t count = 0;
    for (std::size_t i = 0; i < loot.stacks.size(); ++i)
        if (loot.stacks[i].active)
            order[count++] = static_cast<std::uint16_t>(i);
    std::sort(order.begin(), order.begin() + count, [&](auto a, auto b) {
        const auto ca = loot.stacks[a].cell, cb = loot.stacks[b].cell;
        if (ca.y != cb.y)
            return ca.y < cb.y;
        if (ca.x != cb.x)
            return ca.x < cb.x;
        return a < b;
    });
    std::array<Visible, lootCapacity> visible{};
    std::size_t n = 0;
    const auto f = forward(player.heading), r = right(player.heading);
    for (std::size_t begin = 0; begin < count;) {
        auto end = begin + 1;
        const auto cell = loot.stacks[order[begin]].cell;
        while (end < count && loot.stacks[order[end]].cell == cell)
            ++end;
        const int dx = cell.x - player.cell.x, dy = cell.y - player.cell.y;
        const int row = dx * f.x + dy * f.y, side = dx * r.x + dy * r.y;
        if (row >= 0 && row <= ViewProjection::visibleRows && (row || side == 0)) {
            const float depth = row + ViewProjection::cameraSetback;
            float lateral = side - float(phase) / ViewProjection::strafeSteps;
            if (row == 1 && std::abs(side) == 1) {
                const float limit = (viewWidth * .5f - maxSize / 2) * depth / ViewProjection::focalLength();
                lateral = side * std::min(1.f, limit) - float(phase) / ViewProjection::strafeSteps;
            }
            visible[n++] = {order[begin], lateral, depth};
        }
        begin = end;
    }
    std::sort(visible.begin(), visible.begin() + n, [](const auto& a, const auto& b) {
        return a.depth != b.depth ? a.depth > b.depth : a.slot < b.slot;
    });
    std::array<float, NpcWorld::maxInstances> bodyDepth{};
    if (actors)
        for (std::size_t i = 0; i < actors->snapshot().count; ++i) {
            const auto& a = actors->snapshot().actors[i].npc;
            const float dx = a.cell.x - player.cell.x - .25f + .5f * a.subcell.x +
                             (animation ? animation->frames[i].offsetX : 0);
            const float dy = a.cell.y - player.cell.y - .25f + .5f * a.subcell.y +
                             (animation ? animation->frames[i].offsetY : 0);
            bodyDepth[i] = dx * f.x + dy * f.y + ViewProjection::cameraSetback;
        }
    const float focal = ViewProjection::focalLength();
    for (std::size_t i = 0; i < n; ++i) {
        const auto& v = visible[i];
        const int size = std::clamp(int(std::round(focal * .25f / v.depth)), minSize, maxSize);
        const int left = int(std::round(viewWidth * .5f + focal * v.lateral / v.depth)) - size / 2;
        const int foot =
            std::min(viewHeight - 4,
                     int(std::round(ViewProjection::horizon + focal * ViewProjection::eyeHeight / v.depth)));
        const int top = foot - size;
        const auto offset = (size - 1) * size * (2 * size - 1) / 6 - 14;
        bool seen = false;
        for (int y = std::max(0, top); y < std::min(viewHeight, top + size); ++y)
            for (int x = std::max(0, left); x < std::min(viewWidth, left + size); ++x) {
                if (v.depth >= walls[x])
                    continue;
                const auto body = picking.slot(x, y);
                if (actors && body < actors->snapshot().count && bodyDepth[body] <= v.depth)
                    continue;
                auto src = pixels[offset + (y - top) * size + x - left];
                if (src.a < 16)
                    continue;
                src = shadeScene(src, v.depth);
                auto& dst = frame[y * viewWidth + x];
                const unsigned a = src.a, inv = 255 - a;
                dst = {static_cast<unsigned char>((src.r * a + dst.r * inv + 127) / 255),
                       static_cast<unsigned char>((src.g * a + dst.g * inv + 127) / 255),
                       static_cast<unsigned char>((src.b * a + dst.b * inv + 127) / 255), 255};
                picking.write(y * viewWidth + x, static_cast<std::uint16_t>(NpcWorld::maxInstances + v.slot));
                seen = true;
            }
        if (seen)
            picking.lootAnchor(v.slot, std::clamp(left + size / 2, 0, viewWidth - 1),
                               std::clamp(top + size, 0, viewHeight - 1));
    }
}
} // namespace blobber

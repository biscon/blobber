#include "render/loot_sprites.hpp"
#include "render/lighting.hpp"
#include "render/projection.hpp"
#include <algorithm>
#include <cmath>
namespace blobber {
void PreparedLoot::bind(const rules::Catalog& catalog) {
    items.clear();
    for (std::uint16_t i = 0; i < catalog.itemCount(); ++i) {
        const auto& d = *catalog.item({i});
        unsigned icon = 0;
        using K = rules::ItemKind;
        if (d.kind == K::Armor)
            icon = 1;
        else if (d.kind == K::Shield)
            icon = 2;
        else if (d.kind == K::Ammunition)
            icon = 4;
        else if (d.kind == K::Currency)
            icon = 5;
        else if (d.kind == K::Supply)
            icon = 8;
        else if (d.kind == K::Consumable)
            icon = d.consumable == rules::Consumable::Mana ? 7 : 6;
        else if (d.weapon == rules::WeaponType::Shortbow)
            icon = 3;
        items.push_back(static_cast<std::uint8_t>(icon));
    }
}
void PreparedLoot::compose(const LootSnapshot& loot, const LootPage* page, Player player, int phase,
                           const std::array<float, viewWidth>& walls, const ActorWorld* actors,
                           const NpcAnimationSnapshot* animation, Frame& frame,
                           PickingBuffer& picking) const noexcept {
    if (pixels.size() != icons * pixelsPerIcon)
        return;
    picking.lootWorld(loot.world);
    struct Visible {
        std::uint16_t slot;
        float lateral, depth;
        int ordinal;
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
            const unsigned pages = unsigned((end - begin + 5) / 6);
            const unsigned selected = page && page->cell == cell ? page->page % pages : 0;
            for (std::size_t i = begin + selected * 6; i < std::min(end, begin + selected * 6 + 6); ++i) {
                const int ordinal = int(i - begin - selected * 6);
                // Front row of the party cell remains above the viewport edge at every heading.
                const float depth = row == 0
                                        ? 0.74f + (1 - ordinal / 3) * .08f
                                        : row + ViewProjection::cameraSetback + (ordinal / 3 ? -.22f : .18f);
                float lateral = side + (ordinal % 3 - 1) * .23f - float(phase) / ViewProjection::strafeSteps;
                if (row == 1 && std::abs(side) == 1) {
                    // Keep the three front-cell targets readable within the unchanged FOV.
                    const float limit = (viewWidth * .5f - 24) * depth / ViewProjection::focalLength();
                    lateral = side * std::min(1.f, limit - .23f) + (ordinal % 3 - 1) * .23f -
                              float(phase) / ViewProjection::strafeSteps;
                }
                visible[n++] = {order[i], lateral, depth, ordinal};
            }
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
        const auto& stack = loot.stacks[v.slot];
        if (stack.item.index >= items.size())
            continue;
        const int size = stack.cell == player.cell
                             ? 32
                             : std::clamp(int(std::round(focal * .19f / v.depth)), minSize, maxSize);
        const int left = int(std::round(viewWidth * .5f + focal * v.lateral / v.depth)) - size / 2;
        const int top =
            int(std::round(ViewProjection::horizon + focal * ViewProjection::eyeHeight / v.depth)) - size;
        const auto offset =
            items[stack.item.index] * pixelsPerIcon + (size - 1) * size * (2 * size - 1) / 6 - 14;
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
            picking.lootRegion({loot.world, v.slot}, left, top, size);
        if (seen)
            picking.lootAnchor(v.slot, std::clamp(left + size / 2, 0, viewWidth - 1),
                               std::clamp(top + size, 0, viewHeight - 1));
    }
}
} // namespace blobber

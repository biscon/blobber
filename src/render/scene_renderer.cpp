#include "render/scene_renderer.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace blobber {
void SceneRenderer::prepareFloorPreview() noexcept {
    const auto cell = [](int x, int y) {
        if (y + 0.5f <= ViewProjection::horizon)
            return 0;
        const auto point = floorPoint(x, y);
        if (int(std::floor(point.forward + 0.5f)) != 1)
            return 0;
        const int side = int(std::floor(point.lateral + 0.5f));
        return side >= -1 && side <= 1 ? side + 2 : 0;
    };
    for (int y = int(ViewProjection::horizon) + 1; y < viewHeight; ++y) {
        floorDepth_[y] = floorPoint(0, y).depth;
        for (int x = 0; x < viewWidth; ++x) {
            const auto value = cell(x, y);
            if (!value)
                continue;
            const bool edge = cell(x - 1, y) != value || cell(x + 1, y) != value || cell(x, y - 1) != value ||
                              cell(x, y + 1) != value;
            floorMask_[y * viewWidth + x] = std::uint8_t(value | (edge ? 4 : 0));
        }
    }
}
void SceneRenderer::compose(const Map& map, Player player, Frame& result, const ActorWorld* npcs,
                            const NpcAnimationSnapshot* animation, PickingBuffer* picking,
                            const ProjectilePlayer* projectiles, const SpellArea* area) const {
    composeView(map, player, 0, result, npcs, animation, picking, projectiles, area);
}
void SceneRenderer::composeStrafe(const Map& map, Player from, Player to, float t, Frame& result,
                                  const ActorWorld* npcs, const NpcAnimationSnapshot* animation,
                                  PickingBuffer* picking, const ProjectilePlayer* projectiles,
                                  const SpellArea* area) const {
    const Cell r = right(from.heading);
    const bool movingRight = to.cell == from.cell + r;
    if (from.heading != to.heading || (!movingRight && !(to.cell == from.cell + r * -1)))
        throw std::invalid_argument("Strafe views require adjacent tiles and an unchanged heading");
    if (t <= 0) {
        compose(map, from, result, npcs, animation, picking, projectiles, area);
        return;
    }
    if (t >= 1) {
        compose(map, to, result, npcs, animation, picking, projectiles, area);
        return;
    }
    const float eased = t * t * (3 - 2 * t);
    Player base = movingRight ? from : to;
    const float offset = movingRight ? eased : 1 - eased;
    const int phase = static_cast<int>(std::round(offset * ViewProjection::strafeSteps));
    if (phase == ViewProjection::strafeSteps) {
        base.cell = base.cell + r;
        compose(map, base, result, npcs, animation, picking, projectiles, area);
        return;
    }
    composeView(map, base, phase, result, npcs, animation, picking, projectiles, area);
}
void SceneRenderer::composeView(const Map& map, Player player, int phase, Frame& result,
                                const ActorWorld* npcs, const NpcAnimationSnapshot* animation,
                                PickingBuffer* picking, const ProjectilePlayer* projectiles,
                                const SpellArea* area) const {
    if (picking)
        picking->clear();
    const PreparedView& view = views_[phase];
    const int parity = (player.cell.x & 1) * 2 + (player.cell.y & 1);
    result.copyFrom(view.backgrounds[static_cast<int>(player.heading) * 4 + parity]);
    std::array<float, viewWidth> coverage;
    coverage.fill(std::numeric_limits<float>::infinity());
    const Cell f = forward(player.heading), r = right(player.heading);
    const int wallParity = (player.cell.x * r.x + player.cell.y * r.y) & 1;
    for (const PreparedSprite& sprite : view.sprites) {
        const Cell cell = player.cell + f * sprite.distance + r * sprite.lateral;
        if (!map.solid(cell))
            continue;
        const Cell neighbor = sprite.sideNeighbor ? cell + r * sprite.sideNeighbor : cell + f * -1;
        if (map.solid(neighbor))
            continue;
        for (std::uint32_t c = 0; c < sprite.columnCount; ++c) {
            const PreparedColumn& column = view.columns[sprite.firstColumn + c];
            if (column.depth >= coverage[column.x])
                continue;
            // All walls have the same height: a nearer wall covers the entire vertical
            // extent of a farther wall. One depth per column is therefore sufficient.
            coverage[column.x] = column.depth;
            int index = column.top * viewWidth + column.x;
            for (std::uint32_t i = 0; i < column.length; ++i) {
                result[index] = view.pixels[column.pixelOffsets[wallParity] + i];
                index += viewWidth;
            }
        }
    }
    if (area && phase == 0 &&
        std::any_of(area->eligible.begin(), area->eligible.end(), [](bool v) { return v; })) {
        const Color tint =
            area->kind == rules::SpellKind::Thunderwave ? Color{76, 166, 221, 255} : Color{242, 134, 27, 255};
        std::array<bool, 4> active{};
        for (int i = 0; i < 3; ++i)
            active[i + 1] = area->eligible[i] && area->cells[i] == player.cell + f + r * (i - 1);
        for (int y = int(ViewProjection::horizon) + 1; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x) {
                const auto mask = floorMask_[y * viewWidth + x];
                if (!active[mask & 3] || floorDepth_[y] >= coverage[x])
                    continue;
                const unsigned alpha = mask & 4 ? 155 : 38;
                auto& pixel = result[y * viewWidth + x];
                pixel.r = (pixel.r * (255 - alpha) + tint.r * alpha) / 255;
                pixel.g = (pixel.g * (255 - alpha) + tint.g * alpha) / 255;
                pixel.b = (pixel.b * (255 - alpha) + tint.b * alpha) / 255;
            }
    }
    if (npcs)
        npcs_.compose(*npcs, player, phase, coverage, result, animation, picking);
    if (picking)
        picking->finish(npcs);
    if (projectiles && npcs)
        composeProjectiles(*projectiles, npcs_, *npcs, player, phase, coverage, picking, result);
}
} // namespace blobber

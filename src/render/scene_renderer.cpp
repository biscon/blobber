#include "render/scene_renderer.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace blobber {
void SceneRenderer::compose(const Map& map, Player player, Frame& result, const ActorWorld* npcs,
                            const NpcAnimationSnapshot* animation, PickingBuffer* picking,
                            const ProjectilePlayer* projectiles) const {
    composeView(map, player, 0, result, npcs, animation, picking, projectiles);
}
void SceneRenderer::composeStrafe(const Map& map, Player from, Player to, float t, Frame& result,
                                  const ActorWorld* npcs, const NpcAnimationSnapshot* animation,
                                  PickingBuffer* picking, const ProjectilePlayer* projectiles) const {
    const Cell r = right(from.heading);
    const bool movingRight = to.cell == from.cell + r;
    if (from.heading != to.heading || (!movingRight && !(to.cell == from.cell + r * -1)))
        throw std::invalid_argument("Strafe views require adjacent tiles and an unchanged heading");
    if (t <= 0) {
        compose(map, from, result, npcs, animation, picking, projectiles);
        return;
    }
    if (t >= 1) {
        compose(map, to, result, npcs, animation, picking, projectiles);
        return;
    }
    const float eased = t * t * (3 - 2 * t);
    Player base = movingRight ? from : to;
    const float offset = movingRight ? eased : 1 - eased;
    const int phase = static_cast<int>(std::round(offset * ViewProjection::strafeSteps));
    if (phase == ViewProjection::strafeSteps) {
        base.cell = base.cell + r;
        compose(map, base, result, npcs, animation, picking, projectiles);
        return;
    }
    composeView(map, base, phase, result, npcs, animation, picking, projectiles);
}
void SceneRenderer::composeView(const Map& map, Player player, int phase, Frame& result,
                                const ActorWorld* npcs, const NpcAnimationSnapshot* animation,
                                PickingBuffer* picking, const ProjectilePlayer* projectiles) const {
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
    if (npcs)
        npcs_.compose(*npcs, player, phase, coverage, result, animation, picking);
    if (picking)
        picking->finish(npcs);
    if (projectiles && npcs)
        composeProjectiles(*projectiles, npcs_, *npcs, player, phase, coverage, picking, result);
}
} // namespace blobber

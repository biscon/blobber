#include "render/scene_renderer.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace blobber {
std::optional<Cell> SceneRenderer::groundCell(const Map& map, Player player, int x, int y) noexcept {
    if (x < 0 || x >= viewWidth || y <= ViewProjection::horizon || y >= viewHeight)
        return {};
    const auto point = floorPoint(x, y);
    if (point.forward > 7 || std::abs(point.lateral) > 7)
        return {};
    const auto cell = player.cell + forward(player.heading) * int(std::floor(point.forward + .5f)) +
                      right(player.heading) * int(std::floor(point.lateral + .5f));
    return spellCell(map, player, cell) ? std::optional<Cell>{cell} : std::nullopt;
}
void SceneRenderer::prepareFloorPreview() {
    floorSamples_.resize(ViewProjection::strafeSteps);
    for (int phase = 0; phase < ViewProjection::strafeSteps; ++phase) {
        const auto cell = [&](int x, int y) {
            FloorSample sample;
            if (y + .5f <= ViewProjection::horizon)
                return sample;
            const auto p = floorPoint(x, y, float(phase) / ViewProjection::strafeSteps);
            const int f = int(std::floor(p.forward + .5f)), r = int(std::floor(p.lateral + .5f));
            if (f >= 0 && f <= 12 && r >= -12 && r <= 12) {
                sample.forward = std::int8_t(f);
                sample.lateral = std::int8_t(r);
            }
            return sample;
        };
        for (int y = int(ViewProjection::horizon) + 1; y < viewHeight; ++y) {
            floorDepth_[y] = floorPoint(0, y).depth;
            for (int x = 0; x < viewWidth; ++x) {
                auto v = cell(x, y);
                const auto different = [&](int a, int b) {
                    const auto n = cell(a, b);
                    return n.forward != v.forward || n.lateral != v.lateral;
                };
                v.edge =
                    different(x - 1, y) || different(x + 1, y) || different(x, y - 1) || different(x, y + 1);
                floorSamples_[phase][y * viewWidth + x] = v;
            }
        }
    }
}
void SceneRenderer::compose(const Map& map, Player player, Frame& result, const ActorWorld* npcs,
                            const NpcAnimationSnapshot* animation, PickingBuffer* picking,
                            const ProjectilePlayer* projectiles, const SpellArea* area,
                            const LootSnapshot* loot) const {
    composeView(map, player, 0, result, npcs, animation, picking, projectiles, area, loot);
}
void SceneRenderer::composeStrafe(const Map& map, Player from, Player to, float t, Frame& result,
                                  const ActorWorld* npcs, const NpcAnimationSnapshot* animation,
                                  PickingBuffer* picking, const ProjectilePlayer* projectiles,
                                  const SpellArea* area, const LootSnapshot* loot) const {
    const Cell r = right(from.heading);
    const bool movingRight = to.cell == from.cell + r;
    if (from.heading != to.heading || (!movingRight && !(to.cell == from.cell + r * -1)))
        throw std::invalid_argument("Strafe views require adjacent tiles and an unchanged heading");
    if (t <= 0) {
        compose(map, from, result, npcs, animation, picking, projectiles, area, loot);
        return;
    }
    if (t >= 1) {
        compose(map, to, result, npcs, animation, picking, projectiles, area, loot);
        return;
    }
    const float eased = t * t * (3 - 2 * t);
    Player base = movingRight ? from : to;
    const float offset = movingRight ? eased : 1 - eased;
    const int phase = static_cast<int>(std::round(offset * ViewProjection::strafeSteps));
    if (phase == ViewProjection::strafeSteps) {
        base.cell = base.cell + r;
        compose(map, base, result, npcs, animation, picking, projectiles, area, loot);
        return;
    }
    composeView(map, base, phase, result, npcs, animation, picking, projectiles, area, loot);
}
void SceneRenderer::composeView(const Map& map, Player player, int phase, Frame& result,
                                const ActorWorld* npcs, const NpcAnimationSnapshot* animation,
                                PickingBuffer* picking, const ProjectilePlayer* projectiles,
                                const SpellArea* area, const LootSnapshot* loot) const {
    if (loot && !picking)
        picking = &lootScratch_;
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
    if (area && (std::any_of(area->eligible.begin(), area->eligible.end(), [](bool v) { return v; }) ||
                 std::any_of(area->ground.areas.begin(), area->ground.areas.end(),
                             [](const GroundArea& a) { return a.active; }))) {
        std::array<std::uint8_t, 25 * 13> active{};
        const auto mark = [&](Cell cell, std::uint8_t flag) {
            const auto d = cell + player.cell * -1;
            const int depth = d.x * f.x + d.y * f.y, side = d.x * r.x + d.y * r.y;
            if (depth >= 0 && depth <= 12 && side >= -12 && side <= 12 && !map.solid(cell))
                active[depth * 25 + side + 12] |= flag;
        };
        for (const auto& patch : area->ground.areas)
            if (patch.active)
                mark(patch.cell, 1);
        for (std::size_t i = 0; i < area->eligible.size(); ++i)
            if (area->eligible[i])
                mark(area->cells[i], 2);
        const Color preview = area->kind == rules::SpellKind::Sleep         ? Color{139, 125, 222, 255}
                              : area->kind == rules::SpellKind::ColorSpray  ? Color{218, 103, 196, 255}
                              : area->kind == rules::SpellKind::Thunderwave ? Color{76, 166, 221, 255}
                                                                            : Color{242, 134, 27, 255};
        for (int y = int(ViewProjection::horizon) + 1; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x) {
                const auto sample = floorSamples_[phase][y * viewWidth + x];
                if (sample.forward < 0 || floorDepth_[y] >= coverage[x])
                    continue;
                const auto flags = active[sample.forward * 25 + sample.lateral + 12];
                if (!flags)
                    continue;
                auto& pixel = result[y * viewWidth + x];
                const auto blend = [&](Color source) {
                    const unsigned alpha = source.a;
                    pixel.r = (pixel.r * (255 - alpha) + source.r * alpha) / 255;
                    pixel.g = (pixel.g * (255 - alpha) + source.g * alpha) / 255;
                    pixel.b = (pixel.b * (255 - alpha) + source.b * alpha) / 255;
                };
                if (flags & 1)
                    blend(view.grease[unsigned(player.heading)][y * viewWidth + x]);
                if (flags & 2) {
                    auto tint = preview;
                    tint.a = sample.edge ? 155 : 38;
                    blend(tint);
                }
            }
    }
    if (npcs)
        npcs_.compose(*npcs, player, phase, coverage, result, animation, picking);
    if (loot && picking)
        loot_.compose(*loot, player, phase, coverage, npcs, animation, result, *picking);
    if (picking)
        picking->finish(npcs);
    if (projectiles && npcs)
        composeProjectiles(*projectiles, npcs_, *npcs, player, phase, coverage, picking, result);
}
} // namespace blobber

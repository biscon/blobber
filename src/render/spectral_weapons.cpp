#include "game/effects.hpp"
#include "render/lighting.hpp"
#include "render/scene_renderer.hpp"
#include <algorithm>
#include <cmath>
namespace blobber {
void SceneRenderer::composeWeapons(const PreparedView& view, const GroundSnapshot& ground, Player player,
                                   int phase, const std::array<float, viewWidth>& walls,
                                   const ActorWorld* actors, const NpcAnimationSnapshot* animation,
                                   Frame& frame, PickingBuffer& picking) noexcept {
    if (view.weapon.size() != PreparedView::weaponPixelCount)
        return;
    const auto f = forward(player.heading), r = right(player.heading);
    const float focal = ViewProjection::focalLength();
    struct Visible {
        float depth, side;
        unsigned index;
    };
    std::array<Visible, 4> visible{};
    unsigned count = 0;
    for (unsigned i = 0; i < ground.weapons.size(); ++i) {
        const auto& w = ground.weapons[i];
        if (!w.active)
            continue;
        const auto d = w.cell + player.cell * -1;
        const float depth = d.x * f.x + d.y * f.y + ViewProjection::cameraSetback;
        if (depth < ViewProjection::cameraSetback || depth > ViewProjection::visibleRows + 1)
            continue;
        visible[count++] = {
            depth, d.x * float(r.x) + d.y * float(r.y) - float(phase) / ViewProjection::strafeSteps, i};
    }
    // Four entries at most: insertion sort keeps the bounded ordering explicit.
    for (unsigned i = 1; i < count; ++i) {
        const auto value = visible[i];
        unsigned j = i;
        while (j && (value.depth > visible[j - 1].depth ||
                     (value.depth == visible[j - 1].depth && value.index < visible[j - 1].index))) {
            visible[j] = visible[j - 1];
            --j;
        }
        visible[j] = value;
    }
    for (unsigned i = 0; i < count; ++i) {
        const auto v = visible[i];
        const int size = std::clamp(int(std::round(focal * .65f / v.depth)), PreparedView::weaponMinSize,
                                    PreparedView::weaponMaxSize);
        const int left = int(std::round(viewWidth * .5f + focal * v.side / v.depth)) - size / 2;
        const int top =
            int(std::round(ViewProjection::horizon + focal * (ViewProjection::eyeHeight - .8f) / v.depth)) -
            size / 2;
        const int offset = (size - 1) * size * (2 * size - 1) / 6 - 14;
        for (int y = std::max(0, top); y < std::min(viewHeight, top + size); ++y)
            for (int x = std::max(0, left); x < std::min(viewWidth, left + size); ++x) {
                if (v.depth >= walls[x])
                    continue;
                const auto body = picking.slot(x, y);
                if (actors && body < actors->snapshot().count) {
                    const auto& npc = actors->snapshot().actors[body].npc;
                    const float dx = npc.cell.x - player.cell.x - .25f + .5f * npc.subcell.x +
                                     (animation ? animation->frames[body].offsetX : 0);
                    const float dy = npc.cell.y - player.cell.y - .25f + .5f * npc.subcell.y +
                                     (animation ? animation->frames[body].offsetY : 0);
                    if (dx * f.x + dy * f.y + ViewProjection::cameraSetback <= v.depth)
                        continue;
                }
                const auto src = shadeScene(view.weapon[offset + (y - top) * size + x - left], v.depth);
                auto& dst = frame[y * viewWidth + x];
                const unsigned a = src.a, inv = 255 - a;
                dst = {static_cast<unsigned char>((src.r * a + dst.r * inv + 127) / 255),
                       static_cast<unsigned char>((src.g * a + dst.g * inv + 127) / 255),
                       static_cast<unsigned char>((src.b * a + dst.b * inv + 127) / 255), 255};
            }
    }
}
} // namespace blobber

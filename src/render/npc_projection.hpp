#pragma once
#include "game/movement.hpp"
#include "render/projection.hpp"
#include <algorithm>
#include <cmath>
namespace blobber {
struct NpcProjection {
    float lateral = 0, depth = 0, subDepth = 0, cameraX = 0, cameraY = 0;
    int row = 0;
};
// Shared by bodies, shadows and effect attachments. World occupancy is unchanged.
inline NpcProjection projectNpc(const NpcInstance& npc, ActorPosition position, Player player, int phase,
                                float edgeHalfWidth) noexcept {
    const Cell f = forward(player.heading), r = right(player.heading);
    const float offset = float(phase) / ViewProjection::strafeSteps;
    const int dx = npc.cell.x - player.cell.x, dy = npc.cell.y - player.cell.y;
    const float sx = position.x - npc.cell.x - .5f, sy = position.y - npc.cell.y - .5f;
    NpcProjection p;
    p.cameraX = player.cell.x + .5f - f.x * ViewProjection::cameraSetback + r.x * offset;
    p.cameraY = player.cell.y + .5f - f.y * ViewProjection::cameraSetback + r.y * offset;
    p.row = dx * f.x + dy * f.y;
    p.subDepth = sx * f.x + sy * f.y;
    p.depth = p.row + p.subDepth + ViewProjection::cameraSetback;
    p.lateral = dx * r.x + dy * r.y + sx * r.x + sy * r.y - offset;
    const float forwardPosition = p.row + p.subDepth;
    if (forwardPosition > .25f && forwardPosition < 1.75f) {
        const float limit =
            std::max(0.f, (viewWidth * .5f - 2.f) * p.depth / ViewProjection::focalLength() - edgeHalfWidth);
        const float blend = std::min({std::clamp((forwardPosition - .25f) * 2.f, 0.f, 1.f),
                                      std::clamp((1.75f - forwardPosition) * 2.f, 0.f, 1.f),
                                      std::clamp((1.75f - std::abs(p.lateral)) * 2.f, 0.f, 1.f)});
        p.lateral *= 1.f - (1.f - std::min(1.f, limit / 1.25f)) * blend;
    }
    return p;
}
struct ProjectedAttachment {
    float x = 0, y = 0, depth = 0;
    bool valid = false;
};
} // namespace blobber

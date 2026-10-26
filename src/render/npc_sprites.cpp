#include "render/npc_sprites.hpp"
#include "render/lighting.hpp"
#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>

namespace blobber {
NpcSide npcSide(Heading facing, float x, float y) {
    const Cell f = forward(facing), r = right(facing);
    const float front = x * f.x + y * f.y, side = x * r.x + y * r.y;
    if (std::abs(front) >= std::abs(side))
        return front >= 0 ? NpcSide::Front : NpcSide::Back;
    return side >= 0 ? NpcSide::Right : NpcSide::Left;
}
void PreparedNpcs::compose(const ActorWorld& npcs, Player player, int phase,
                           const std::array<float, viewWidth>& walls, Frame& frame,
                           const NpcAnimationSnapshot* animation, PickingBuffer* picking) const {
    if (phase < 0 || phase >= ViewProjection::strafeSteps)
        throw std::invalid_argument("Invalid NPC strafe phase");
    if (npcs.snapshot().typeCount != types_.size())
        throw std::invalid_argument("NPC world and prepared appearance counts differ");
    if (animation && animation->count != npcs.snapshot().count)
        throw std::invalid_argument("NPC animation/world instance counts differ");
    if (animation &&
        (animation->world != npcs.snapshot().world || animation->actorRevision != npcs.snapshot().revision))
        throw std::invalid_argument("NPC animation snapshot belongs to another world or revision");
    struct Visible {
        const Sprite* sprite;
        const Shadow* shadow;
        float lateral, depth;
        int left;
        std::size_t instance;
        bool alive;
        const TravelSprite* travel;
        float shadowRadius;
    };
    std::array<Visible, NpcWorld::maxInstances> visible;
    std::size_t count = 0;
    const Cell f = forward(player.heading), r = right(player.heading);
    const float offset = float(phase) / ViewProjection::strafeSteps;
    const float focal = ViewProjection::focalLength();
    const float cameraX = player.cell.x + 0.5f - f.x * ViewProjection::cameraSetback + r.x * offset;
    const float cameraY = player.cell.y + 0.5f - f.y * ViewProjection::cameraSetback + r.y * offset;
    for (std::size_t i = 0; i < npcs.snapshot().count; ++i) {
        const auto& actor = npcs.snapshot().actors[i];
        if (actor.life == ActorLife::Removed ||
            (animation ? !animation->frames[i].visible : actor.life != ActorLife::Alive))
            continue;
        const auto& npc = actor.npc;
        const float ox = animation ? animation->frames[i].offsetX : 0;
        const float oy = animation ? animation->frames[i].offsetY : 0;
        if (!std::isfinite(ox) || !std::isfinite(oy) || std::abs(ox) > Map::maxDimension ||
            std::abs(oy) > Map::maxDimension)
            throw std::invalid_argument("Invalid NPC travel offset");
        const bool traveling = ox != 0 || oy != 0;
        const int dx = npc.cell.x - player.cell.x, dy = npc.cell.y - player.cell.y;
        const int row = dx * f.x + dy * f.y;
        const float sx = npc.subcell.x * 0.5f - 0.25f + ox;
        const float sy = npc.subcell.y * 0.5f - 0.25f + oy;
        const float subDepth = sx * f.x + sy * f.y;
        const float depth = row + subDepth + ViewProjection::cameraSetback;
        if (traveling ? (depth < .15f || depth > ViewProjection::visibleRows + .65f)
                      : (row < 0 || row > ViewProjection::visibleRows))
            continue;
        float lateral = dx * r.x + dy * r.y + sx * r.x + sy * r.y - offset;
        // Compress the front row laterally only as much as its outer positions
        // need. Applying one factor preserves distinct subslots instead of
        // clamping two neighboring bodies onto the same screen position.
        // Depth, world occupancy and sprite scale are unchanged.
        const auto& type = types_.at(npc.type.index);
        const float forwardPosition = row + subDepth;
        if (forwardPosition > .25f && forwardPosition < 1.75f) {
            const float limit = std::max(0.f, (viewWidth * .5f - 2.f) * depth / focal - type.edgeHalfWidth);
            // Blend at the row edges so travel/strafe cannot snap between the
            // adjusted front row and ordinary projection.
            const float blend = std::min({std::clamp((forwardPosition - .25f) * 2.f, 0.f, 1.f),
                                          std::clamp((1.75f - forwardPosition) * 2.f, 0.f, 1.f),
                                          std::clamp((1.75f - std::abs(lateral)) * 2.f, 0.f, 1.f)});
            lateral *= 1.f - (1.f - std::min(1.f, limit / 1.25f)) * blend;
        }
        const auto side =
            npcSide(npc.facing, cameraX - (npc.cell.x + 0.5f + sx), cameraY - (npc.cell.y + 0.5f + sy));
        const int slot = traveling ? 0 : row * 2 + (subDepth > 0 ? 1 : 0);
        auto clip = static_cast<std::size_t>(animation ? animation->frames[i].clip : npc.animation);
        const auto direction = static_cast<std::size_t>(side);
        std::size_t frameIndex = animation ? animation->frames[i].active : 0;
        if (clip >= npcAnimationCount || !type.count[clip][0])
            throw std::invalid_argument("NPC selects absent prepared animation");
        if (!type.count[clip][direction]) {
            if (clip != static_cast<std::size_t>(NpcAnimation::Attack))
                throw std::invalid_argument("NPC reaction missing required direction");
            clip = 0;
            frameIndex = animation ? animation->frames[i].idle : 0;
        }
        if (frameIndex >= type.count[clip][direction])
            throw std::invalid_argument("NPC animation frame out of range");
        const auto& s = sprites_[type.first[clip][direction] + frameIndex * depthCount + slot];
        const int left = static_cast<int>(std::round(viewWidth * 0.5f + focal * lateral / depth - s.pivotX));
        // A floor blob may remain visible even when the body is clipped off screen.
        visible[count++] = {&s,
                            &shadows_[npc.type.index][slot],
                            lateral,
                            depth,
                            left,
                            i,
                            actor.life == ActorLife::Alive,
                            traveling ? &travelSprites_[type.travelFirst[clip][direction] + frameIndex]
                                      : nullptr,
                            type.shadowRadius};
    }
    std::sort(visible.begin(), visible.begin() + count, [](const Visible& a, const Visible& b) {
        if (a.depth != b.depth)
            return a.depth > b.depth;
        if (a.alive != b.alive)
            return !a.alive;
        return a.instance < b.instance;
    });
    // Floor shadows are a separate pass: never paint a shadow over another body.
    for (std::size_t i = 0; i < count; ++i) {
        const auto& item = visible[i];
        if (item.travel) {
            composeTravelShadow(item.shadowRadius, item.lateral, item.depth, walls, frame);
            continue;
        }
        const auto& shadow = *item.shadow;
        for (std::size_t r = 0; r < shadow.rowCount; ++r) {
            const auto& row = shadowRows_[shadow.firstRow + r];
            const int left = static_cast<int>(
                std::round(viewWidth * 0.5f + focal * item.lateral / row.depth - row.width * 0.5f));
            for (int x = std::max(0, left); x < std::min(viewWidth, left + row.width); ++x) {
                if (row.depth >= walls[x])
                    continue;
                const unsigned inverse = 255 - shadowAlpha_[row.first + x - left];
                auto& dst = frame[row.y * viewWidth + x];
                dst = {static_cast<unsigned char>((dst.r * inverse + 127) / 255),
                       static_cast<unsigned char>((dst.g * inverse + 127) / 255),
                       static_cast<unsigned char>((dst.b * inverse + 127) / 255), 255};
            }
        }
    }
    for (std::size_t i = 0; i < count; ++i) {
        const auto& item = visible[i];
        if (item.travel) {
            composeTravel(*item.travel, item.lateral, item.depth, static_cast<std::uint16_t>(item.instance),
                          animation && animation->frames[item.instance].flash, walls, frame, picking);
            continue;
        }
        const auto& s = *item.sprite;
        for (int x = std::max(0, item.left + s.cropX);
             x < std::min(viewWidth, item.left + s.cropX + s.cropWidth); ++x) {
            if (item.depth >= walls[x])
                continue;
            for (int y = s.cropY; y < s.cropY + s.cropHeight; ++y) {
                Color src = pixels_[s.first + static_cast<std::size_t>(y - s.cropY) * s.cropWidth + x -
                                    item.left - s.cropX];
                if (!src.a)
                    continue;
                if (picking)
                    picking->write((s.top + y) * viewWidth + x, static_cast<std::uint16_t>(item.instance));
                if (animation && animation->frames[item.instance].flash)
                    src.r = src.g = src.b = 255;
                auto& dst = frame[(s.top + y) * viewWidth + x];
                const unsigned alpha = src.a, inverse = 255 - alpha;
                dst = {static_cast<unsigned char>((src.r * alpha + dst.r * inverse + 127) / 255),
                       static_cast<unsigned char>((src.g * alpha + dst.g * inverse + 127) / 255),
                       static_cast<unsigned char>((src.b * alpha + dst.b * inverse + 127) / 255), 255};
            }
        }
    }
}
} // namespace blobber

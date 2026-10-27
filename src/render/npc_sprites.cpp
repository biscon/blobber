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
    const float focal = ViewProjection::focalLength();
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
        const auto& type = types_.at(npc.type.index);
        const ActorPosition position{npc.cell.x + .25f + npc.subcell.x * .5f + ox,
                                     npc.cell.y + .25f + npc.subcell.y * .5f + oy};
        const auto projected = projectNpc(npc, position, player, phase, type.edgeHalfWidth);
        const auto row = projected.row;
        const auto depth = projected.depth, lateral = projected.lateral, subDepth = projected.subDepth;
        if (traveling ? (depth < .15f || depth > ViewProjection::visibleRows + .65f)
                      : (row < 0 || row > ViewProjection::visibleRows))
            continue;
        const auto side = npcSide(npc.facing, projected.cameraX - position.x, projected.cameraY - position.y);
        if (picking) {
            // Fixed front-idle geometry keeps badges still as silhouettes breathe or turn.
            // finish() publishes this position only if the body has visible pixels.
            const auto& idle = travelSprites_[type.travelFirst[0][0]];
            const float scale = focal / depth;
            picking->badgeAnchor(
                static_cast<std::uint16_t>(i), int(std::round(viewWidth * .5f + scale * lateral)),
                int(std::round(ViewProjection::horizon +
                               scale * (ViewProjection::eyeHeight - idle.worldHeight * idle.anchorY))));
        }
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
void PreparedNpcs::setAttachments(const std::vector<NpcAttachments>& metadata) {
    if (!metadata.empty() && metadata.size() != types_.size())
        throw std::invalid_argument("NPC attachment/type count differs");
    for (std::size_t i = 0; i < metadata.size(); ++i) {
        for (const auto& group : {metadata[i].release, metadata[i].torso})
            for (const auto& anchor : group)
                if (!std::isfinite(anchor.x) || !std::isfinite(anchor.y) || anchor.x < 0 || anchor.x > 1 ||
                    anchor.y < 0 || anchor.y > 1)
                    throw std::invalid_argument("Invalid NPC effect attachment");
        attachments_[i] = metadata[i];
    }
}
ProjectedAttachment PreparedNpcs::attachment(const NpcInstance& npc, ActorPosition position, bool release,
                                             Player player, int phase) const noexcept {
    if (npc.type.index >= types_.size())
        return {};
    const auto& type = types_[npc.type.index];
    const auto p = projectNpc(npc, position, player, phase, type.edgeHalfWidth);
    if (p.depth < .1f || p.depth > ViewProjection::visibleRows + 1)
        return {};
    const auto side =
        static_cast<std::size_t>(npcSide(npc.facing, p.cameraX - position.x, p.cameraY - position.y));
    auto clip = release ? static_cast<std::size_t>(NpcAnimation::RangedAttack) : 0;
    if (!type.count[clip][side])
        clip = 0;
    const auto index = release ? type.count[clip][side] / 2 : 0;
    const auto& raster = travelSprites_[type.travelFirst[clip][side] + index];
    const auto anchor =
        release ? attachments_[npc.type.index].release[side] : attachments_[npc.type.index].torso[side];
    const float scale = ViewProjection::focalLength() / p.depth;
    return {viewWidth * .5f + scale * (p.lateral + raster.worldWidth * (anchor.x - raster.anchorX)),
            ViewProjection::horizon +
                scale * (ViewProjection::eyeHeight + raster.worldHeight * (anchor.y - raster.anchorY)),
            p.depth, true};
}
} // namespace blobber

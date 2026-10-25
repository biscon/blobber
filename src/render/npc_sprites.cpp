#include "render/npc_sprites.hpp"
#include "render/lighting.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace blobber {
NpcSide npcSide(Heading facing, float x, float y) {
    const Cell f = forward(facing), r = right(facing);
    const float front = x * f.x + y * f.y, side = x * r.x + y * r.y;
    if (std::abs(front) >= std::abs(side))
        return front >= 0 ? NpcSide::Front : NpcSide::Back;
    return side >= 0 ? NpcSide::Right : NpcSide::Left;
}
namespace {
std::size_t frameCount(const NpcAppearance& a, std::size_t clip, std::size_t side) {
    return a.animations[0].timing.count ? a.animations[clip].views[side].size() : (clip == 0 ? 1 : 0);
}
const NpcImage& sourceFrame(const NpcAppearance& a, std::size_t clip, std::size_t side, std::size_t frame) {
    return a.animations[0].timing.count ? a.animations[clip].views[side][frame] : a.views[side];
}
} // namespace
NpcTimings npcTimings(const NpcAppearance& a) {
    NpcTimings result{};
    if (!a.animations[0].timing.count)
        result[0] = {{{1, 0, 0, 0}}, 1};
    else
        for (std::size_t c = 0; c < npcAnimationCount; ++c)
            result[c] = a.animations[c].timing;
    return result;
}
std::size_t validateNpcAppearance(const NpcAppearance& a) {
    std::size_t pixels = 0;
    if (a.alphaCutoff == 255)
        throw std::runtime_error("NPC alpha cutoff must be 0..254");
    if (!std::isfinite(a.width) || !std::isfinite(a.height) || a.width <= 0 || a.width > 0.5f ||
        a.height <= 0 || a.height > ViewProjection::wallHeight)
        throw std::runtime_error("NPC dimensions must be positive, width <= 0.5 and height <= 1");
    const bool animated = a.animations[0].timing.count != 0;
    validateNpcTimings(npcTimings(a));
    for (const auto& image : a.views)
        if (animated && (image.width || image.height || !image.pixels.empty()))
            throw std::runtime_error("NPC cannot mix static views and animations");
    for (std::size_t clip = 0; clip < npcAnimationCount; ++clip) {
        const auto& c = a.animations[clip];
        const float maxWidth = clip == static_cast<std::size_t>(NpcAnimation::Death) ? 1.0f : 0.5f;
        if (!std::isfinite(c.width) || c.width < 0 || c.width > maxWidth || (c.width && !c.timing.count))
            throw std::runtime_error("Invalid NPC clip canvas width");
        if (!animated && c.timing.count)
            throw std::runtime_error("Animated NPC requires idle clip");
        for (std::size_t side = 0; side < 4; ++side) {
            const auto count = frameCount(a, clip, side);
            if ((!animated && !c.views[side].empty()) ||
                (animated && count != c.timing.count && !(clip == 2 && side != 0 && count == 0)))
                throw std::runtime_error("NPC view frame counts must match clip timing");
            for (std::size_t frame = 0; frame < count; ++frame) {
                const auto& image = sourceFrame(a, clip, side, frame);
                if (image.width <= 0 || image.height <= 0 || image.width > npcMaxImageDimension ||
                    image.height > npcMaxImageDimension ||
                    image.pixels.size() != static_cast<std::size_t>(image.width) * image.height ||
                    !std::isfinite(image.anchorX) || !std::isfinite(image.anchorY) || image.anchorX < 0 ||
                    image.anchorX > 1 || image.anchorY <= 0 || image.anchorY > 1)
                    throw std::runtime_error("Invalid NPC image or anchor");
                // Only transparent padding may extend below the ground anchor.
                for (int y = 0; y < image.height; ++y)
                    if (y + 0.5f >= image.anchorY * image.height)
                        for (int x = 0; x < image.width; ++x)
                            if (image.pixels[static_cast<std::size_t>(y) * image.width + x].a > a.alphaCutoff)
                                throw std::runtime_error("NPC image has visible pixels below ground anchor");
                pixels += image.pixels.size();
                if (pixels > npcPixelBudget)
                    throw std::runtime_error("NPC source images exceed 128 MiB budget");
            }
        }
    }
    return pixels;
}
PreparedNpcs::PreparedNpcs(const std::vector<NpcAppearance>& appearances) {
    if (appearances.size() > NpcWorld::maxTypes)
        throw std::runtime_error("Too many NPC appearances");
    std::size_t sourcePixels = 0;
    for (const auto& a : appearances) {
        sourcePixels += validateNpcAppearance(a);
        if (sourcePixels > npcPixelBudget)
            throw std::runtime_error("NPC source images exceed 128 MiB budget");
    }
    types_.resize(appearances.size());
    const float focal = ViewProjection::focalLength();
    std::size_t count = 0;
    for (std::size_t type = 0; type < appearances.size(); ++type) {
        const auto& a = appearances[type];
        for (std::size_t clip = 0; clip < npcAnimationCount; ++clip) {
            for (std::size_t side = 0; side < 4; ++side) {
                const auto frames = frameCount(a, clip, side);
                types_[type].first[clip][side] = sprites_.size();
                types_[type].count[clip][side] = static_cast<std::uint8_t>(frames);
                for (std::size_t frame = 0; frame < frames; ++frame) {
                    const auto& image = sourceFrame(a, clip, side, frame);
                    for (int slot = 0; slot < depthCount; ++slot) {
                        const float depth =
                            slot / 2 + ((slot & 1) ? 0.25f : -0.25f) + ViewProjection::cameraSetback;
                        Sprite sprite;
                        const float canvasWidth =
                            a.animations[clip].width ? a.animations[clip].width : a.width;
                        const float w = focal * canvasWidth / depth, h = focal * a.height / depth;
                        const float ground =
                            ViewProjection::horizon + focal * ViewProjection::eyeHeight / depth;
                        const float top = ground - h * image.anchorY;
                        sprite.width = std::max(1, static_cast<int>(std::round(w)));
                        sprite.pivotX = sprite.width * image.anchorX;
                        sprite.top = std::clamp(static_cast<int>(std::ceil(top - 0.5f)), 0, viewHeight);
                        const int bottom =
                            std::clamp(static_cast<int>(std::ceil(top + h - 0.5f)), 0, viewHeight);
                        sprite.rows = bottom - sprite.top;
                        sprite.first = count;
                        count += static_cast<std::size_t>(sprite.width) * sprite.rows;
                        if (count > npcPixelBudget - sourcePixels)
                            throw std::runtime_error("NPC source and prepared images exceed 128 MiB budget");
                        sprites_.push_back(sprite);
                    }
                }
            }
        }
    }
    prepareShadows(appearances, (sourcePixels + count) * sizeof(Color));
    pixels_.resize(count);
    for (std::size_t type = 0; type < appearances.size(); ++type) {
        const auto& a = appearances[type];
        for (std::size_t clip = 0; clip < npcAnimationCount; ++clip) {
            for (std::size_t side = 0; side < 4; ++side) {
                for (std::size_t frame = 0; frame < frameCount(a, clip, side); ++frame) {
                    const auto& image = sourceFrame(a, clip, side, frame);
                    for (int slot = 0; slot < depthCount; ++slot) {
                        const float depth =
                            slot / 2 + ((slot & 1) ? 0.25f : -0.25f) + ViewProjection::cameraSetback;
                        const auto& sprite =
                            sprites_[types_[type].first[clip][side] + frame * depthCount + slot];
                        const float h = focal * a.height / depth;
                        const float top = ViewProjection::horizon +
                                          focal * ViewProjection::eyeHeight / depth - h * image.anchorY;
                        for (int y = 0; y < sprite.rows; ++y) {
                            const int sy =
                                std::clamp(static_cast<int>((sprite.top + y + 0.5f - top) / h * image.height),
                                           0, image.height - 1);
                            for (int x = 0; x < sprite.width; ++x) {
                                const int sx =
                                    std::min(image.width - 1,
                                             static_cast<int>((x + 0.5f) / sprite.width * image.width));
                                Color pixel = image.pixels[static_cast<std::size_t>(sy) * image.width + sx];
                                if (pixel.a <= a.alphaCutoff)
                                    pixel.a = 0;
                                pixels_[sprite.first + static_cast<std::size_t>(y) * sprite.width + x] =
                                    shadeScene(pixel, depth);
                            }
                        }
                    }
                }
            }
        }
    }
}
void PreparedNpcs::prepareShadows(const std::vector<NpcAppearance>& appearances, std::size_t usedPixelBytes) {
    shadows_.resize(appearances.size());
    const float focal = ViewProjection::focalLength();
    std::size_t alphaCount = 0;
    // At most maxTypes * depthCount * viewHeight rows. Metadata may grow only here.
    for (std::size_t type = 0; type < appearances.size(); ++type) {
        const float radius = appearances[type].width * 0.45f;
        for (int slot = 0; slot < depthCount; ++slot) {
            const float centerDepth =
                slot / 2 + ((slot & 1) ? 0.25f : -0.25f) + ViewProjection::cameraSetback;
            auto& shadow = shadows_[type][slot];
            shadow.firstRow = shadowRows_.size();
            for (int y = 0; y < viewHeight; ++y) {
                if (y + 0.5f <= ViewProjection::horizon)
                    continue;
                const float depth = focal * ViewProjection::eyeHeight / (y + 0.5f - ViewProjection::horizon);
                if (std::abs(depth - centerDepth) >= radius)
                    continue;
                const int width = std::max(1, static_cast<int>(std::ceil(2 * radius * focal / depth)));
                shadowRows_.push_back({y, width, depth, alphaCount});
                alphaCount += static_cast<std::size_t>(width);
                if (alphaCount > npcPixelBudget * sizeof(Color) - usedPixelBytes)
                    throw std::runtime_error("NPC images and shadows exceed 128 MiB budget");
            }
            shadow.rowCount = shadowRows_.size() - shadow.firstRow;
        }
    }
    shadowAlpha_.resize(alphaCount);
    for (std::size_t type = 0; type < appearances.size(); ++type) {
        const float radius = appearances[type].width * 0.45f;
        for (int slot = 0; slot < depthCount; ++slot) {
            const float centerDepth =
                slot / 2 + ((slot & 1) ? 0.25f : -0.25f) + ViewProjection::cameraSetback;
            const auto& shadow = shadows_[type][slot];
            for (std::size_t i = 0; i < shadow.rowCount; ++i) {
                const auto& row = shadowRows_[shadow.firstRow + i];
                const float z = (row.depth - centerDepth) / radius;
                const float fade =
                    1 - std::clamp((row.depth - (9.0f + ViewProjection::cameraSetback)) / 3.5f, 0.0f, 1.0f);
                for (int x = 0; x < row.width; ++x) {
                    const float dx = (x + 0.5f - row.width * 0.5f) * row.depth / (focal * radius);
                    const float falloff = std::max(0.0f, 1 - dx * dx - z * z);
                    shadowAlpha_[row.first + x] =
                        static_cast<unsigned char>(std::round(255 * 0.32f * falloff * falloff * fade));
                }
            }
        }
    }
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
        const int dx = npc.cell.x - player.cell.x, dy = npc.cell.y - player.cell.y;
        const int row = dx * f.x + dy * f.y;
        if (row < 0 || row > ViewProjection::visibleRows)
            continue;
        const float sx = npc.subcell.x * 0.5f - 0.25f, sy = npc.subcell.y * 0.5f - 0.25f;
        const float subDepth = sx * f.x + sy * f.y;
        const float depth = row + subDepth + ViewProjection::cameraSetback;
        const float lateral = dx * r.x + dy * r.y + sx * r.x + sy * r.y - offset;
        const auto side =
            npcSide(npc.facing, cameraX - (npc.cell.x + 0.5f + sx), cameraY - (npc.cell.y + 0.5f + sy));
        const int slot = row * 2 + (subDepth > 0 ? 1 : 0);
        const auto& type = types_.at(npc.type.index);
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
        visible[count++] = {&s, &shadows_[npc.type.index][slot], lateral, depth, left,
                            i,  actor.life == ActorLife::Alive};
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
        const auto& s = *item.sprite;
        for (int x = std::max(0, item.left); x < std::min(viewWidth, item.left + s.width); ++x) {
            if (item.depth >= walls[x])
                continue;
            for (int y = 0; y < s.rows; ++y) {
                Color src = pixels_[s.first + static_cast<std::size_t>(y) * s.width + x - item.left];
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

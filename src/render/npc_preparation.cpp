#include "render/lighting.hpp"
#include "render/npc_sprites.hpp"
#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>

namespace blobber {
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
namespace {
std::size_t validateAppearance(const NpcAppearance& a, bool requirePixels) {
    for (const auto& group : {a.attachments.release, a.attachments.torso})
        for (const auto& anchor : group)
            if (!std::isfinite(anchor.x) || !std::isfinite(anchor.y) || anchor.x < 0 || anchor.x > 1 ||
                anchor.y < 0 || anchor.y > 1)
                throw std::runtime_error("Invalid NPC effect attachment");
    std::size_t pixels = 0;
    if (a.sampling != ImageSampling::Nearest && a.sampling != ImageSampling::Area)
        throw std::runtime_error("Invalid NPC sampling profile");
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
        const float maxWidth = clip == static_cast<std::size_t>(NpcAnimation::Death)          ? 1.0f
                               : clip == static_cast<std::size_t>(NpcAnimation::RangedAttack) ? 0.6f
                                                                                              : 0.5f;
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
                if (!std::isfinite(image.anchorX) || !std::isfinite(image.anchorY) || image.anchorX < 0 ||
                    image.anchorX > 1 || image.anchorY <= 0 || image.anchorY > 1)
                    throw std::runtime_error("Invalid NPC image or anchor");
                if (!requirePixels)
                    continue;
                if (image.width <= 0 || image.height <= 0 || image.width > npcMaxImageDimension ||
                    image.height > npcMaxImageDimension ||
                    image.pixels.size() != static_cast<std::size_t>(image.width) * image.height)
                    throw std::runtime_error("Invalid NPC decoded image");
                // Only transparent padding may extend below the ground anchor.
                for (int y = 0; y < image.height; ++y)
                    if (y + 0.5f >= image.anchorY * image.height)
                        for (int x = 0; x < image.width; ++x)
                            if (image.pixels[static_cast<std::size_t>(y) * image.width + x].a > a.alphaCutoff)
                                throw std::runtime_error("NPC image has visible pixels below ground anchor");
                pixels += image.pixels.size();
                if (pixels > npcPixelBudget)
                    throw std::runtime_error("NPC source images exceed 512 MiB source budget");
            }
        }
    }
    return pixels;
}
} // namespace
std::size_t validateNpcAppearance(const NpcAppearance& a) {
    return validateAppearance(a, true);
}
void validateNpcAppearanceMetadata(const NpcAppearance& a) {
    (void)validateAppearance(a, false);
}
std::size_t PreparedNpcs::storageBytes() const noexcept {
    return sizeof(*this) + sprites_.capacity() * sizeof(Sprite) +
           travelSprites_.capacity() * sizeof(TravelSprite) + types_.capacity() * sizeof(Type) +
           pixels_.capacity() * sizeof(Color) + shadows_.capacity() * sizeof(std::array<Shadow, depthCount>) +
           shadowRows_.capacity() * sizeof(ShadowRow) + shadowAlpha_.capacity();
}
std::size_t PreparedNpcs::stationaryPixelBytes() const noexcept {
    std::size_t count = 0;
    for (const auto& s : sprites_)
        count += static_cast<std::size_t>(s.cropWidth) * s.cropHeight;
    return count * sizeof(Color);
}
std::size_t PreparedNpcs::travelPixelBytes() const noexcept {
    std::size_t count = 0;
    for (const auto& s : travelSprites_)
        count += static_cast<std::size_t>(s.cropWidth) * s.cropHeight;
    return count * sizeof(Color);
}
PreparedNpcs::PreparedNpcs(const std::vector<NpcAppearance>& appearances, PreparationLimits limits) {
    if (appearances.size() > NpcWorld::maxTypes)
        throw std::runtime_error("Too many NPC appearances");
    std::size_t sourceOwned = appearances.capacity() * sizeof(NpcAppearance), totalFrames = 0;
    for (const auto& a : appearances) {
        sourceBytes_ += validateNpcAppearance(a) * sizeof(Color);
        for (std::size_t clip = 0; clip < npcAnimationCount; ++clip)
            for (std::size_t side = 0; side < 4; ++side) {
                const auto frames = frameCount(a, clip, side);
                totalFrames += frames;
                sourceOwned += a.animations[clip].views[side].capacity() * sizeof(NpcImage);
                for (std::size_t f = 0; f < frames; ++f)
                    sourceOwned += sourceFrame(a, clip, side, f).pixels.capacity() * sizeof(Color);
            }
    }
    const auto metadata = sizeof(*this) + appearances.size() * sizeof(Type) +
                          totalFrames * (sizeof(TravelSprite) + depthCount * sizeof(Sprite));
    requirePreparationBytes(0, metadata, limits.residentBytes);
    requirePreparationBytes(sourceOwned, metadata, limits.loadingBytes);
    types_.resize(appearances.size());
    sprites_.reserve(totalFrames * depthCount);
    travelSprites_.reserve(totalFrames);
    const float focal = ViewProjection::focalLength();
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) {
            const float dx = (x + .5f) / 32 - 1, dy = (y + .5f) / 32 - 1;
            const float falloff = std::max(0.f, 1 - dx * dx - dy * dy);
            travelShadow_[y * 64 + x] =
                static_cast<unsigned char>(std::round(255 * .32f * falloff * falloff));
        }
    for (std::size_t type = 0; type < appearances.size(); ++type) {
        const auto& a = appearances[type];
        attachments_[type] = a.attachments;
        types_[type].shadowRadius = a.width * .45f;
        for (std::size_t clip = 0; clip < npcAnimationCount; ++clip)
            for (std::size_t side = 0; side < 4; ++side) {
                const auto frames = frameCount(a, clip, side);
                types_[type].first[clip][side] = sprites_.size();
                types_[type].travelFirst[clip][side] = travelSprites_.size();
                types_[type].count[clip][side] = static_cast<std::uint8_t>(frames);
                for (std::size_t f = 0; f < frames; ++f) {
                    const auto& image = sourceFrame(a, clip, side, f);
                    const float canvasWidth = a.animations[clip].width ? a.animations[clip].width : a.width;
                    types_[type].edgeHalfWidth = std::max(
                        types_[type].edgeHalfWidth, canvasWidth * std::max(image.anchorX, 1 - image.anchorX));
                    const float scale = std::min(1.f, 256.f / std::max(image.width, image.height));
                    const int tw = std::max(1, int(std::round(image.width * scale)));
                    const int th = std::max(1, int(std::round(image.height * scale)));
                    travelSprites_.push_back(
                        {tw, th, canvasWidth, a.height, image.anchorX, image.anchorY, 0});
                    for (int slot = 0; slot < depthCount; ++slot) {
                        const float depth =
                            slot / 2 + ((slot & 1) ? .25f : -.25f) + ViewProjection::cameraSetback;
                        const float h = focal * a.height / depth;
                        const float top = ViewProjection::horizon +
                                          focal * ViewProjection::eyeHeight / depth - h * image.anchorY;
                        Sprite sprite;
                        sprite.width = std::max(1, int(std::round(focal * canvasWidth / depth)));
                        sprite.pivotX = sprite.width * image.anchorX;
                        sprite.top = std::clamp(int(std::ceil(top - .5f)), 0, viewHeight);
                        sprite.rows = std::clamp(int(std::ceil(top + h - .5f)), 0, viewHeight) - sprite.top;
                        sprites_.push_back(sprite);
                    }
                }
            }
    }
    // First count exact nontransparent rectangles, then allocate once and fill.
    // Two sampling passes avoid retaining a dense bank or growing the pixel arena.
    std::size_t pixelCount = 0, maxScratch = 0;
    for (int pass = 0; pass < 2; ++pass) {
        for (std::size_t type = 0; type < appearances.size(); ++type) {
            const auto& a = appearances[type];
            for (std::size_t clip = 0; clip < npcAnimationCount; ++clip)
                for (std::size_t side = 0; side < 4; ++side)
                    for (std::size_t f = 0; f < frameCount(a, clip, side); ++f) {
                        const auto& image = sourceFrame(a, clip, side, f);
                        std::optional<AreaSampler> area;
                        if (a.sampling == ImageSampling::Area) {
                            const auto scratch = static_cast<std::size_t>(image.width + 1) *
                                                 (image.height + 1) * sizeof(double) * 4;
                            requirePreparationBytes(sourceOwned + storageBytes(), scratch,
                                                    limits.loadingBytes);
                            area.emplace(image.pixels.data(), image.width, image.height, image.width,
                                         a.alphaCutoff);
                            maxScratch = std::max(maxScratch, area->storageBytes());
                        }
                        loadingHighWater_ =
                            std::max(loadingHighWater_,
                                     sourceOwned + storageBytes() + (area ? area->storageBytes() : 0));
                        requirePreparationBytes(0, loadingHighWater_, limits.loadingBytes);
                        const auto raster = [&](auto& record, int width, int height, const auto& sample) {
                            if (pass == 0) {
                                int x0 = width, y0 = height, x1 = 0, y1 = 0;
                                for (int y = 0; y < height; ++y)
                                    for (int x = 0; x < width; ++x)
                                        if (sample(x, y).a > a.alphaCutoff) {
                                            x0 = std::min(x0, x);
                                            y0 = std::min(y0, y);
                                            x1 = std::max(x1, x + 1);
                                            y1 = std::max(y1, y + 1);
                                        }
                                record.first = pixelCount;
                                if (x1 > x0 && y1 > y0) {
                                    record.cropX = x0;
                                    record.cropY = y0;
                                    record.cropWidth = x1 - x0;
                                    record.cropHeight = y1 - y0;
                                }
                                const auto count =
                                    static_cast<std::size_t>(record.cropWidth) * record.cropHeight;
                                requirePreparationBytes(pixelCount, count,
                                                        limits.residentBytes / sizeof(Color));
                                pixelCount += count;
                            } else {
                                for (int y = 0; y < record.cropHeight; ++y)
                                    for (int x = 0; x < record.cropWidth; ++x) {
                                        auto pixel = sample(record.cropX + x, record.cropY + y);
                                        if (pixel.a <= a.alphaCutoff)
                                            pixel.a = 0;
                                        pixels_[record.first +
                                                static_cast<std::size_t>(y) * record.cropWidth + x] = pixel;
                                    }
                            }
                        };
                        auto& travel = travelSprites_[types_[type].travelFirst[clip][side] + f];
                        raster(travel, travel.width, travel.height, [&](int x, int y) {
                            if (area)
                                return area->sample(double(x) * image.width / travel.width,
                                                    double(y) * image.height / travel.height,
                                                    double(x + 1) * image.width / travel.width,
                                                    double(y + 1) * image.height / travel.height);
                            const int sx =
                                std::min(image.width - 1, int((x + .5f) * image.width / travel.width));
                            const int sy =
                                std::min(image.height - 1, int((y + .5f) * image.height / travel.height));
                            return image.pixels[static_cast<std::size_t>(sy) * image.width + sx];
                        });
                        for (int slot = 0; slot < depthCount; ++slot) {
                            const float depth =
                                slot / 2 + ((slot & 1) ? .25f : -.25f) + ViewProjection::cameraSetback;
                            auto& sprite = sprites_[types_[type].first[clip][side] + f * depthCount + slot];
                            const float h = focal * a.height / depth;
                            const float top = ViewProjection::horizon +
                                              focal * ViewProjection::eyeHeight / depth - h * image.anchorY;
                            raster(sprite, sprite.width, sprite.rows, [&](int x, int y) {
                                Color pixel;
                                if (area)
                                    pixel = area->sample(double(x) * image.width / sprite.width,
                                                         (sprite.top + y - top) / h * image.height,
                                                         double(x + 1) * image.width / sprite.width,
                                                         (sprite.top + y + 1 - top) / h * image.height);
                                else {
                                    const int sx = std::min(image.width - 1,
                                                            int((x + .5f) / sprite.width * image.width));
                                    const int sy =
                                        std::clamp(int((sprite.top + y + .5f - top) / h * image.height), 0,
                                                   image.height - 1);
                                    pixel = image.pixels[static_cast<std::size_t>(sy) * image.width + sx];
                                }
                                return shadeScene(pixel, depth);
                            });
                        }
                    }
        }
        if (pass == 0) {
            const auto pixelBytes = pixelCount * sizeof(Color);
            requirePreparationBytes(storageBytes(), pixelBytes, limits.residentBytes);
            requirePreparationBytes(sourceOwned + storageBytes() + maxScratch, pixelBytes,
                                    limits.loadingBytes);
            const auto available =
                std::min(limits.residentBytes - storageBytes() - pixelBytes,
                         limits.loadingBytes - sourceOwned - storageBytes() - maxScratch - pixelBytes);
            prepareShadows(appearances, available);
            pixels_.resize(pixelCount);
            requirePreparationBytes(0, storageBytes(), limits.residentBytes);
            requirePreparationBytes(sourceOwned + storageBytes(), maxScratch, limits.loadingBytes);
            loadingHighWater_ = std::max(loadingHighWater_, sourceOwned + storageBytes() + maxScratch);
        }
    }
}
void PreparedNpcs::prepareShadows(const std::vector<NpcAppearance>& appearances, std::size_t availableBytes) {
    const float focal = ViewProjection::focalLength();
    std::size_t plannedRows = 0, plannedAlpha = 0;
    for (const auto& appearance : appearances) {
        const float radius = appearance.width * .45f;
        for (int slot = 0; slot < depthCount; ++slot) {
            const float centerDepth = slot / 2 + ((slot & 1) ? .25f : -.25f) + ViewProjection::cameraSetback;
            for (int y = 0; y < viewHeight; ++y) {
                if (y + .5f <= ViewProjection::horizon)
                    continue;
                const float depth = focal * ViewProjection::eyeHeight / (y + .5f - ViewProjection::horizon);
                if (std::abs(depth - centerDepth) >= radius)
                    continue;
                ++plannedRows;
                plannedAlpha += std::max(1, int(std::ceil(2 * radius * focal / depth)));
            }
        }
    }
    requirePreparationBytes(plannedAlpha,
                            plannedRows * sizeof(ShadowRow) +
                                appearances.size() * sizeof(std::array<Shadow, depthCount>),
                            availableBytes);
    shadows_.resize(appearances.size());
    shadowRows_.reserve(plannedRows);
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
} // namespace blobber

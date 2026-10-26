#include "render/lighting.hpp"
#include "render/npc_sprites.hpp"
#include <algorithm>
#include <cmath>

namespace blobber {
void PreparedNpcs::composeTravel(const TravelSprite& s, float lateral, float depth, std::uint16_t instance,
                                 bool flash, const std::array<float, viewWidth>& walls, Frame& frame,
                                 PickingBuffer* picking) const {
    const float focal = ViewProjection::focalLength();
    const int width = std::max(1, int(std::round(focal * s.worldWidth / depth)));
    const float height = focal * s.worldHeight / depth;
    const float top =
        ViewProjection::horizon + focal * ViewProjection::eyeHeight / depth - height * s.anchorY;
    const int left = int(std::round(viewWidth * .5f + focal * lateral / depth - width * s.anchorX));
    const int firstY = std::clamp(int(std::ceil(top - .5f)), 0, viewHeight);
    const float ground = ViewProjection::horizon + focal * ViewProjection::eyeHeight / depth;
    const int lastY = std::clamp(int(std::ceil(std::min(top + height, ground) - .5f)), 0, viewHeight);
    std::array<Color, 256> lighting;
    for (int i = 0; i < 256; ++i) {
        const auto value = static_cast<unsigned char>(i);
        lighting[i] = shadeScene({value, value, value, 255}, depth);
    }
    for (int x = std::max(0, left); x < std::min(viewWidth, left + width); ++x) {
        if (depth >= walls[x])
            continue;
        const int sx = std::min(s.width - 1, int((x - left + .5f) * s.width / width));
        for (int y = firstY; y < lastY; ++y) {
            const int sy = std::clamp(int((y + .5f - top) * s.height / height), 0, s.height - 1);
            auto src = pixels_[s.first + static_cast<std::size_t>(sy) * s.width + sx];
            if (!src.a)
                continue;
            const auto pixel = y * viewWidth + x;
            if (picking)
                picking->write(pixel, instance);
            src = flash ? Color{255, 255, 255, src.a}
                        : Color{lighting[src.r].r, lighting[src.g].g, lighting[src.b].b, src.a};
            auto& dst = frame[pixel];
            const unsigned alpha = src.a, inverse = 255 - alpha;
            dst = {static_cast<unsigned char>((src.r * alpha + dst.r * inverse + 127) / 255),
                   static_cast<unsigned char>((src.g * alpha + dst.g * inverse + 127) / 255),
                   static_cast<unsigned char>((src.b * alpha + dst.b * inverse + 127) / 255), 255};
        }
    }
}
void PreparedNpcs::composeTravelShadow(float radius, float lateral, float centerDepth,
                                       const std::array<float, viewWidth>& walls, Frame& frame) const {
    const float focal = ViewProjection::focalLength();
    const int firstY =
        std::clamp(int(std::ceil(ViewProjection::horizon +
                                 focal * ViewProjection::eyeHeight / (centerDepth + radius) - .5f)),
                   0, viewHeight);
    for (int y = firstY; y < viewHeight; ++y) {
        const float depth = focal * ViewProjection::eyeHeight / (y + .5f - ViewProjection::horizon);
        const float z = (depth - centerDepth) / radius;
        if (z <= -1)
            break;
        if (z >= 1)
            continue;
        const int sy = std::clamp(int((z + 1) * 32), 0, 63);
        const int width = std::max(1, int(std::ceil(2 * radius * focal / depth)));
        const int left = int(std::round(viewWidth * .5f + focal * lateral / depth - width * .5f));
        const float fade = 1 - std::clamp((depth - (9.f + ViewProjection::cameraSetback)) / 3.5f, 0.f, 1.f);
        for (int x = std::max(0, left); x < std::min(viewWidth, left + width); ++x) {
            if (depth >= walls[x])
                continue;
            const float dx = (x - left + .5f - width * .5f) * depth / (focal * radius);
            const int sx = std::clamp(int((dx + 1) * 32), 0, 63);
            const unsigned inverse =
                255 - static_cast<unsigned>(std::round(travelShadow_[sy * 64 + sx] * fade));
            auto& dst = frame[y * viewWidth + x];
            dst = {static_cast<unsigned char>((dst.r * inverse + 127) / 255),
                   static_cast<unsigned char>((dst.g * inverse + 127) / 255),
                   static_cast<unsigned char>((dst.b * inverse + 127) / 255), 255};
        }
    }
}
} // namespace blobber

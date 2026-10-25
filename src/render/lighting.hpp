#pragma once
#include "render/projection.hpp"
#include <algorithm>
#include <raylib.h>
namespace blobber {
inline constexpr Color sceneDarkness{15, 18, 20, 255};
inline Color shadeScene(Color c, float depth, float face = 1.0f) {
    const float light = face / (1.0f + 0.115f * depth + 0.015f * depth * depth);
    const float fog = std::clamp((depth - (9.0f + ViewProjection::cameraSetback)) / 3.5f, 0.0f, 1.0f);
    auto channel = [light, fog](unsigned char value, unsigned char dark) {
        return static_cast<unsigned char>(std::clamp(value * light * (1 - fog) + dark * fog, 0.0f, 255.0f));
    };
    return {channel(c.r, sceneDarkness.r), channel(c.g, sceneDarkness.g), channel(c.b, sceneDarkness.b), c.a};
}
} // namespace blobber

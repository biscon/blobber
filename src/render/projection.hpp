#pragma once
#include "render/layout.hpp"

namespace blobber {
struct ViewProjection {
    static constexpr float horizontalFovDegrees = 84.0f, cameraSetback = 0.4f;
    // Keep 32 foreground floor rows visible against an adjacent wall at this FOV.
    static constexpr float horizon = viewHeight * 0.34f, eyeHeight = 0.5f, wallHeight = 1.0f;
    static constexpr int visibleRows = 12, strafeSteps = 8;
    static float focalLength();
};
struct FloorPoint {
    float lateral, forward, depth;
};
inline FloorPoint floorPoint(int x, int y, float offset = 0) noexcept {
    const float depth =
        ViewProjection::focalLength() * ViewProjection::eyeHeight / (y + 0.5f - ViewProjection::horizon);
    return {(x + 0.5f - viewWidth * 0.5f) * depth / ViewProjection::focalLength() + offset,
            depth - ViewProjection::cameraSetback, depth};
}
} // namespace blobber

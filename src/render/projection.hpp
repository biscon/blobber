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
} // namespace blobber

#pragma once
#include "render/world_projection_parameters.hpp"
#include "render/world_scene.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "raymath.h"
#pragma GCC diagnostic pop
#include <cmath>

namespace blobber {
// Shared by color/depth rendering; the horizon is intentionally off center.
inline Matrix worldProjection(float farClip, float horizon = world_render::viewHorizon) noexcept {
    const double half = world_render::nearPlane * std::tan(84 * world_render::pi / 360);
    return MatrixFrustum(-half, half,
                         -viewHeight * (1 - horizon) * world_render::nearPlane / world_render::focal(),
                         viewHeight * horizon * world_render::nearPlane / world_render::focal(),
                         world_render::nearPlane, farClip);
}
} // namespace blobber

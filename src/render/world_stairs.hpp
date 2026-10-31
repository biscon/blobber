#pragma once
#include "render/world_scene.hpp"
namespace blobber::world_render {
struct StairQuad {
    std::array<Vertex, 4> vertices;
    std::uint8_t material = 0;
};
struct StairGeometry {
    // Eight treads/risers, tunnel lining, exterior shell, front reveals and two end caps.
    std::array<StairQuad, 128> quads;
    unsigned count = 0;
};
// Preparation only. One-cell footprint, independent of collision and runtime animation.
StairGeometry stairGeometry(GeometryView, const FixtureDefinition&, Surfaces);
} // namespace blobber::world_render

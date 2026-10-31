#include "render/world_stairs.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace blobber::world_render {
StairGeometry stairGeometry(GeometryView geometry, const FixtureDefinition& stair, Surfaces surfaces) {
    StairGeometry result;
    const auto n = forward(stair.facing), r = right(stair.facing);
    // The visible descending flight must remain visible over the threshold with the fixed eye height/setback.
    // A deeper 45-degree flight would be entirely hidden behind the landing lip.
    const float sign = stair.kind == FixtureKind::Upstairs ? 1.f : -.25f;
    const auto surface = geometry.surface(stair.cell);
    const float base = elevation(surface.floor);
    const float clearance = surface.ceiling == noCeiling ? 1.f : elevation(surface.ceiling) - base;
    auto point = [&](float u, float y, float t) {
        return Vec{stair.cell.x + .5f + r.x * (u - .5f) + n.x * (t - .5f), base + y,
                   stair.cell.y + .5f + r.y * (u - .5f) + n.y * (t - .5f)};
    };
    auto face = [&](std::array<Vec, 4> p, Vec outward, unsigned material, bool dark = true,
                    float brightness = 1) {
        if (result.count == result.quads.size())
            throw std::runtime_error("Stair geometry capacity exceeded");
        const Vec a{p[1].x - p[0].x, p[1].y - p[0].y, p[1].z - p[0].z};
        const Vec b{p[2].x - p[0].x, p[2].y - p[0].y, p[2].z - p[0].z};
        Vec normal{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
        const float length = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (length == 0)
            throw std::runtime_error("Degenerate stair face");
        const float orientation =
            normal.x * outward.x + normal.y * outward.y + normal.z * outward.z < 0 ? -1.f : 1.f;
        if (orientation < 0)
            std::reverse(p.begin(), p.end());
        normal = {normal.x * orientation / length, normal.y * orientation / length,
                  normal.z * orientation / length};
        auto& quad = result.quads[result.count++];
        quad.material = std::uint8_t(material);
        for (unsigned k = 0; k < 4; ++k) {
            const float t = std::clamp(
                (p[k].x - stair.cell.x - .5f) * n.x + (p[k].z - stair.cell.y - .5f) * n.y + .5f, 0.f, 1.f);
            const float shade =
                dark ? std::min(1.f, brightness * std::pow(1 - t, sign > 0 ? 2.4f : 1.f)) : 1;
            quad.vertices[k] = {p[k], normal,
                                std::abs(normal.y) > .5f ? p[k].x
                                                         : (std::abs(normal.x) > .5f ? p[k].z : p[k].x),
                                std::abs(normal.y) > .5f ? p[k].z : 1 - p[k].y, shade};
        }
    };
    for (unsigned step = 0; step < 8; ++step) {
        const float t0 = float(step) / 8, t1 = float(step + 1) / 8;
        const float y0 = sign * t0, y1 = sign * t1;
        const float low = std::min(y0, y1), high = std::max(y0, y1) + clearance;
        const float tread = sign > 0 ? y1 : y0, riser = sign > 0 ? t0 : t1;
        face({point(.06f, y0, riser), point(.94f, y0, riser), point(.94f, y1, riser), point(.06f, y1, riser)},
             {float(sign > 0 ? -n.x : n.x), 0, float(sign > 0 ? -n.y : n.y)}, surfaces.stairWall);
        face({point(.06f, tread, t0), point(.94f, tread, t0), point(.94f, tread, t1), point(.06f, tread, t1)},
             {0, 1, 0}, surfaces.stairFloor);
        if (sign < 0) {
            // A small stone nosing catches light and makes descending tread edges readable from the landing.
            face({point(.06f, tread, t1 - .04f), point(.94f, tread, t1 - .04f),
                  point(.94f, tread + .015f, t1), point(.06f, tread + .015f, t1)},
                 {0, 1, 0}, surfaces.stairWall, true, 1.5f);
        }
        for (unsigned side = 0; side < 2; ++side) {
            const float inner = side ? .94f : .06f, outer = side ? 1.f : 0.f;
            const Vec inward{float(side ? -r.x : r.x), 0, float(side ? -r.y : r.y)};
            face({point(inner, low, t0), point(inner, low, t1), point(inner, high, t1),
                  point(inner, high, t0)},
                 inward, surfaces.stairWall);
            const float bottom = std::min(0.f, low), top = std::max(clearance, high) + .06f;
            face({point(outer, bottom, t0), point(outer, bottom, t1), point(outer, top, t1),
                  point(outer, top, t0)},
                 {-inward.x, 0, -inward.z}, surfaces.stairWall, false);
            face({point(inner, top, t0), point(outer, top, t0), point(outer, top, t1), point(inner, top, t1)},
                 {0, 1, 0}, surfaces.stairWall, false);
            if (!step)
                face({point(outer, 0, 0), point(inner, 0, 0), point(inner, top, 0), point(outer, top, 0)},
                     {float(-n.x), 0, float(-n.y)}, surfaces.stairWall, false);
        }
        face({point(.06f, y0 + clearance, t0), point(.94f, y0 + clearance, t0),
              point(.94f, y1 + clearance, t1), point(.06f, y1 + clearance, t1)},
             {0, -1, 0}, surfaces.ceiling);
        face({point(.06f, y0 + clearance + .06f, t0), point(.94f, y0 + clearance + .06f, t0),
              point(.94f, y1 + clearance + .06f, t1), point(.06f, y1 + clearance + .06f, t1)},
             {0, 1, 0}, surfaces.stairWall, false);
    }
    face({point(.06f, sign, .9999f), point(.94f, sign, .9999f), point(.94f, sign + clearance, .9999f),
          point(.06f, sign + clearance, .9999f)},
         {float(-n.x), 0, float(-n.y)}, surfaces.stairWall);
    // Force the terminal panel fully black, including its near-coplanar entrance-facing vertices.
    for (auto& v : result.quads[result.count - 1].vertices)
        v.shade = 0;
    face({point(0, std::min(0.f, sign), 1), point(1, std::min(0.f, sign), 1),
          point(1, std::max(clearance, sign + clearance) + .06f, 1),
          point(0, std::max(clearance, sign + clearance) + .06f, 1)},
         {float(n.x), 0, float(n.y)}, surfaces.stairWall, false);
    return result;
}
} // namespace blobber::world_render

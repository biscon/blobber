#include "render/world_doors.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace blobber::world_render {
std::array<float, 2> doorUvSpan(const DoorBox& b, unsigned textureWidth, unsigned textureHeight) noexcept {
    const float width = std::hypot(b.x.x, b.x.z), ratio = width * textureHeight / (b.y.y * textureWidth);
    return {std::min(1.f, ratio), std::min(1.f, 1.f / ratio)};
}
std::array<Vec, 8> corners(const DoorBox& b) noexcept {
    std::array<Vec, 8> p{};
    for (unsigned i = 0; i < 8; ++i)
        p[i] = {b.origin.x + (i & 1 ? b.x.x : 0) + (i & 2 ? b.y.x : 0) + (i & 4 ? b.z.x : 0),
                b.origin.y + (i & 1 ? b.x.y : 0) + (i & 2 ? b.y.y : 0) + (i & 4 ? b.z.y : 0),
                b.origin.z + (i & 1 ? b.x.z : 0) + (i & 2 ? b.y.z : 0) + (i & 4 ? b.z.z : 0)};
    return p;
}
std::array<Vec, 4> doorEmblemQuad(const DoorBox& b, bool back) noexcept {
    std::array<Vec, 4> result{};
    for (unsigned i = 0; i < result.size(); ++i) {
        float x = (i == 1 || i == 2) ? .725f : .275f;
        if (back)
            x = 1.f - x;
        const float y = i >= 2 ? .82f : .37f, z = back ? 1.02f : -.02f;
        result[i] = {b.origin.x + b.x.x * x + b.y.x * y + b.z.x * z,
                     b.origin.y + b.x.y * x + b.y.y * y + b.z.y * z,
                     b.origin.z + b.x.z * x + b.y.z * y + b.z.z * z};
    }
    return result;
}
unsigned doorBoxes(GeometryView g, const DoorSnapshot* doors, std::array<DoorBox, doorBoxCapacity>& out,
                   unsigned ordinary, unsigned locked, unsigned frame, unsigned edge, unsigned guardianEmblem,
                   unsigned guardianDoor) {
    if (doors && doors->world != g.world())
        throw std::invalid_argument("Foreign door snapshot");
    const auto* fs = g.fixtures();
    unsigned count = 0;
    if (!fs)
        return count;
    for (unsigned i = 0; i < fs->definition().count; ++i) {
        const auto& f = fs->definition().entries[i];
        if (f.kind != FixtureKind::Door && f.kind != FixtureKind::GuardianGate)
            continue;
        const auto r = right(f.facing), n = forward(f.facing);
        const auto surface = g.surface(f.cell);
        const float floor = elevation(surface.floor),
                    height = (surface.ceiling == noCeiling ? g.base().upper() : elevation(surface.ceiling)) -
                             floor;
        const Vec origin{f.cell.x + .5f - r.x * .5f - n.x * .5f, floor,
                         f.cell.y + .5f - r.y * .5f - n.y * .5f};
        auto add = [&](float x, float y, float z, float w, float h, float d, float angle, unsigned texture,
                       std::uint16_t id = PickingBuffer::empty) {
            const float c = std::cos(angle), s = std::sin(angle);
            out[count++] = {{origin.x + r.x * x + n.x * z, floor + y, origin.z + r.y * x + n.y * z},
                            {w * (r.x * c + n.x * s), 0, w * (r.y * c + n.y * s)},
                            {0, h, 0},
                            {d * (-r.x * s + n.x * c), 0, d * (-r.y * s + n.y * c)},
                            id,
                            std::uint16_t(texture),
                            std::uint16_t(edge),
                            id == PickingBuffer::empty};
        };
        const float width = .88f * .65f, leafHeight = (height - .08f) * .875f, jamb = (1.f - width) * .5f;
        add(0, 0, 0, jamb, height, .12f, 0, frame);
        add(1.f - jamb, 0, 0, jamb, height, .12f, 0, frame);
        add(jamb, leafHeight, 0, width, height - leafHeight, .12f, 0, frame);
        const bool opened = fs->open(i);
        const float progress = doors ? doors->progress(i) : 1.f;
        float t = opened ? progress : 1.f - progress;
        t = t * t * (3 - 2 * t);
        const float angle = t * pi * .5f;
        // Hinge through the leaf thickness, keeping the open slab clear of the adjoining wall.
        add(jamb + .03f - .03f * std::cos(angle), 0, .06f - .03f * std::sin(angle), width, leafHeight, .06f,
            angle, f.key || f.kind == FixtureKind::GuardianGate ? locked : ordinary,
            std::uint16_t(PickingBuffer::fixtureBase + i));
        out[count - 1].leaf = true;
        if (f.kind == FixtureKind::GuardianGate || i == guardianDoor)
            out[count - 1].emblemTexture = std::uint16_t(guardianEmblem);
    }
    return count;
}
} // namespace blobber::world_render

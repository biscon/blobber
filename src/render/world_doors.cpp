#include "render/world_doors.hpp"
#include <cmath>
#include <stdexcept>
namespace blobber::world_render {
std::array<Vec, 8> corners(const DoorBox& b) noexcept {
    std::array<Vec, 8> p{};
    for (unsigned i = 0; i < 8; ++i)
        p[i] = {b.origin.x + (i & 1 ? b.x.x : 0) + (i & 2 ? b.y.x : 0) + (i & 4 ? b.z.x : 0),
                b.origin.y + (i & 1 ? b.x.y : 0) + (i & 2 ? b.y.y : 0) + (i & 4 ? b.z.y : 0),
                b.origin.z + (i & 1 ? b.x.z : 0) + (i & 2 ? b.y.z : 0) + (i & 4 ? b.z.z : 0)};
    return p;
}
unsigned doorBoxes(GeometryView g, const DoorSnapshot* doors, std::array<DoorBox, doorBoxCapacity>& out,
                   unsigned ordinary, unsigned locked, unsigned frame) {
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
                            std::uint16_t(texture)};
        };
        add(0, 0, 0, .06f, height, .12f, 0, frame);
        add(.94f, 0, 0, .06f, height, .12f, 0, frame);
        add(.06f, height - .08f, 0, .88f, .08f, .12f, 0, frame);
        const bool completed = fs->completed(i);
        float t = completed ? (doors ? doors->progress(i) : 1.f) : 0.f;
        t = t * t * (3 - 2 * t);
        add(.06f, 0, .06f, .88f, height - .08f, .06f, t * pi * .5f,
            f.key || f.kind == FixtureKind::GuardianGate ? locked : ordinary,
            completed ? PickingBuffer::empty : std::uint16_t(PickingBuffer::fixtureBase + i));
    }
    return count;
}
} // namespace blobber::world_render

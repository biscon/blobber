#include "render/spell_lights.hpp"
#include <cmath>
namespace blobber::world_render {
SpellLightData prepareSpellLights(const FrameInput& in, const WorldFrame& world,
                                  const EffectFrame& effects) noexcept {
    SpellLightData out;
    out.count = std::min<unsigned>(effects.lightCount, spellLightCapacity);
    if (!out.count)
        return out;
    std::array<int, fixtureCapacity> doorIndex{};
    doorIndex.fill(-1);
    for (unsigned i = 0; i < world.doorCount; ++i) {
        const auto& d = world.doors[i];
        if (d.leaf && d.id >= PickingBuffer::fixtureBase &&
            d.id < PickingBuffer::fixtureBase + fixtureCapacity)
            doorIndex[d.id - PickingBuffer::fixtureBase] = int(i) - 3;
        const float xx = d.x.x * d.x.x + d.x.z * d.x.z, zz = d.z.x * d.z.x + d.z.z * d.z.z;
        const auto at = lightDoorsOffset + i * 3;
        out.texels[at] = {d.origin.x, d.origin.y, d.origin.z, d.y.y};
        out.texels[at + 1] = {d.x.x / xx, 0, d.x.z / xx, 0};
        out.texels[at + 2] = {d.z.x / zz, 0, d.z.z / zz, 0};
    }
    for (unsigned i = 0; i < out.count; ++i) {
        const auto& l = effects.lights[i];
        const int ox = int(std::floor(l.position.x)) - 2, oz = int(std::floor(l.position.z)) - 2;
        out.texels[i * 3] = {l.position.x, l.position.y, l.position.z, std::min(1.5f, l.radius)};
        out.texels[i * 3 + 1] = {l.color[0], l.color[1], l.color[2], l.strength};
        out.texels[i * 3 + 2] = {float(ox), float(oz), 0, 0};
        for (int z = 0; z < 5; ++z)
            for (int x = 0; x < 5; ++x) {
                const Cell cell{ox + x, oz + z};
                const auto s = in.geometry.surface(cell);
                int door = -1;
                if (const auto* fixtures = in.geometry.fixtures()) {
                    const int f = fixtures->at(cell);
                    if (f >= 0 && unsigned(f) < fixtureCapacity)
                        door = doorIndex[unsigned(f)];
                }
                const bool solid = door >= 0 ? in.geometry.base().solid(cell) : in.geometry.solid(cell);
                out.texels[lightCellsOffset + i * 25 + unsigned(z * 5 + x)] = {
                    elevation(s.floor), s.ceiling == noCeiling ? 10000.f : elevation(s.ceiling),
                    solid ? 1.f : 0.f, float(door + 1)};
            }
    }
    return out;
}
namespace {
bool slab(float at, float delta, float lo, float hi, float& enter, float& leave) {
    if (std::abs(delta) < 1e-7f)
        return at >= lo && at <= hi;
    const float a = (lo - at) / delta, b = (hi - at) / delta;
    enter = std::max(enter, std::min(a, b));
    leave = std::min(leave, std::max(a, b));
    return leave - enter > 1e-6f;
}
} // namespace
bool spellLightVisible(const SpellLightData& data, unsigned index, Vec receiver, Vec normal) noexcept {
    if (index >= data.count)
        return false;
    const auto& l = data.texels[index * 3];
    const Vec target{receiver.x + normal.x * .002f, receiver.y + normal.y * .002f,
                     receiver.z + normal.z * .002f};
    const Vec delta{target.x - l[0], target.y - l[1], target.z - l[2]};
    const auto& o = data.texels[index * 3 + 2];
    int x = int(std::floor(l[0])), z = int(std::floor(l[2]));
    const int sx = delta.x >= 0 ? 1 : -1, sz = delta.z >= 0 ? 1 : -1;
    const float dx = std::abs(delta.x) > 1e-7f ? std::abs(1 / delta.x) : 1e20f;
    const float dz = std::abs(delta.z) > 1e-7f ? std::abs(1 / delta.z) : 1e20f;
    float tx = std::abs(delta.x) > 1e-7f ? (float(x + (sx > 0)) - l[0]) / delta.x : 1e20f;
    float tz = std::abs(delta.z) > 1e-7f ? (float(z + (sz > 0)) - l[2]) / delta.z : 1e20f;
    float at = 0;
    for (unsigned step = 0; step < 8; ++step) {
        const int cx = x - int(o[0]), cz = z - int(o[1]);
        if (cx < 0 || cx >= 5 || cz < 0 || cz >= 5)
            return false;
        const auto& cell = data.texels[lightCellsOffset + index * 25 + unsigned(cz * 5 + cx)];
        const float end = std::min(1.f, std::min(tx, tz));
        const float y0 = l[1] + delta.y * at, y1 = l[1] + delta.y * end;
        if (cell[2] > 0 || std::min(y0, y1) < cell[0] - .0001f || std::max(y0, y1) > cell[1] + .0001f)
            return false;
        if (cell[3] > 0)
            for (unsigned box = 0; box < 4; ++box) {
                const unsigned b = lightDoorsOffset + (unsigned(cell[3]) - 1 + box) * 3;
                const auto& a = data.texels[b];
                const auto& u = data.texels[b + 1];
                const auto& v = data.texels[b + 2];
                const Vec d{l[0] - a[0], l[1] - a[1], l[2] - a[2]};
                float enter = .00001f, leave = .99999f;
                if (slab(d.x * u[0] + d.z * u[2], delta.x * u[0] + delta.z * u[2], 0, 1, enter, leave) &&
                    slab(d.y, delta.y, 0, a[3], enter, leave) &&
                    slab(d.x * v[0] + d.z * v[2], delta.x * v[0] + delta.z * v[2], 0, 1, enter, leave))
                    return false;
            }
        if (end >= 1.f)
            return true;
        // At exact corners, reject the other side as well as the cell visited next.
        if (std::abs(tx - tz) < 1e-6f) {
            const int otherZ = cz + sz;
            if (otherZ < 0 || otherZ >= 5)
                return false;
            const auto& side = data.texels[lightCellsOffset + index * 25 + unsigned(otherZ * 5 + cx)];
            if (side[2] > 0 || side[3] > 0 || y1 < side[0] - .0001f || y1 > side[1] + .0001f)
                return false;
        }
        // Equal crossings visit both adjacent cells (zero-length intermediate segment).
        if (tx <= tz) {
            at = tx;
            tx += dx;
            x += sx;
        } else {
            at = tz;
            tz += dz;
            z += sz;
        }
    }
    return false;
}
} // namespace blobber::world_render

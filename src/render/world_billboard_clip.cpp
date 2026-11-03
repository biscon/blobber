#include "render/world_visibility.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace blobber::world_render {
namespace {
constexpr float epsilon = 1e-6f;
struct Extents {
    float left, right, low, high;
};
Extents extents(const Billboard& b) noexcept {
    return {(b.crop.left - b.anchorX) * b.width, (b.crop.right - b.anchorX) * b.width,
            b.feet.y + (b.anchorY - b.crop.bottom) * b.height,
            b.feet.y + (b.anchorY - b.crop.top) * b.height};
}
bool slab(float at, float delta, float low, float high, float& enter, float& leave) noexcept {
    if (std::abs(delta) < epsilon)
        return at >= low && at <= high;
    const float a = (low - at) / delta, b = (high - at) / delta;
    enter = std::max(enter, std::min(a, b));
    leave = std::min(leave, std::max(a, b));
    return leave - enter > epsilon;
}
bool rectangle(const Billboard& b, const Extents& e, float left, float right, float low, float high,
               BillboardClipRect& out) noexcept {
    left = std::max(left, e.left);
    right = std::min(right, e.right);
    low = std::max(low, e.low);
    high = std::min(high, e.high);
    if (right - left <= epsilon || high - low <= epsilon)
        return false;
    const float width = (b.crop.right - b.crop.left) * b.width,
                height = (b.crop.bottom - b.crop.top) * b.height;
    out = {(left - e.left) / width, (e.high - high) / height, (right - e.left) / width,
           (e.high - low) / height};
    return true;
}
} // namespace

bool BillboardClips::contains(float u, float v) const noexcept {
    for (unsigned i = 0; i < count; ++i) {
        const auto& r = rectangles[i];
        if (u >= r.left && u <= r.right && v >= r.top && v <= r.bottom)
            return true;
    }
    return false;
}

void prepareTerrainClips(GeometryView geometry, const Camera& camera, const Billboard& b, TerrainClips& out) {
    out.count = 0;
    const auto e = extents(b);
    const float x0 = b.feet.x + camera.right.x * e.left, x1 = b.feet.x + camera.right.x * e.right,
                z0 = b.feet.z + camera.right.z * e.left, z1 = b.feet.z + camera.right.z * e.right;
    // Width <= 1 is validated by asset preparation. Ignore zero-area corner contacts.
    const auto add = [&](float left, float right, float low, float high) {
        BillboardClipRect mask;
        if (!rectangle(b, e, left, right, low, high, mask))
            return;
        if (out.count == out.rectangles.size())
            throw std::runtime_error("Billboard terrain clipping capacity exceeded");
        out.rectangles[out.count++] = mask;
    };
    for (int z = int(std::floor(std::min(z0, z1))); z <= int(std::floor(std::max(z0, z1))); ++z)
        for (int x = int(std::floor(std::min(x0, x1))); x <= int(std::floor(std::max(x0, x1))); ++x) {
            float left = e.left, right = e.right;
            if (!slab(b.feet.x, camera.right.x, float(x), float(x + 1), left, right) ||
                !slab(b.feet.z, camera.right.z, float(z), float(z + 1), left, right))
                continue;
            if (geometry.base().solid({x, z}))
                add(left, right, e.low, e.high);
            else {
                const auto surface = geometry.surface({x, z});
                add(left, right, e.low, elevation(surface.floor));
                if (surface.ceiling != noCeiling)
                    add(left, right, elevation(surface.ceiling), e.high);
            }
        }
}

void billboardClips(const WorldFrame& frame, const Billboard& b, BillboardClips& out) noexcept {
    out.count = 0;
    if (b.id >= NpcWorld::maxInstances)
        return;
    const auto& terrain = frame.terrainClips[b.id];
    for (unsigned i = 0; i < terrain.count; ++i)
        out.rectangles[out.count++] = terrain.rectangles[i];
    const auto e = extents(b);
    for (unsigned i = 0; i < frame.doorCount; ++i) {
        const auto& box = frame.doors[i];
        if (box.origin.y >= e.high || box.origin.y + box.y.y <= e.low)
            continue;
        float left = e.left, right = e.right;
        bool intersects = true;
        for (const auto axis : {box.x, box.z}) {
            const float length2 = axis.x * axis.x + axis.z * axis.z;
            const float at =
                ((b.feet.x - box.origin.x) * axis.x + (b.feet.z - box.origin.z) * axis.z) / length2;
            const float delta = (frame.camera.right.x * axis.x + frame.camera.right.z * axis.z) / length2;
            if (!slab(at, delta, 0, 1, left, right)) {
                intersects = false;
                break;
            }
        }
        if (intersects &&
            rectangle(b, e, left, right, box.origin.y, box.origin.y + box.y.y, out.rectangles[out.count]))
            ++out.count;
    }
}
} // namespace blobber::world_render

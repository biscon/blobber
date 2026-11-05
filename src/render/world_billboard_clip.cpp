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
// Conservative screen bounds; a box crossing the near plane is never culled.
bool overlapsProjection(const Camera& camera, const Billboard& body, const DoorBox& box) noexcept {
    float left = 1e30f, right = -1e30f, top = 1e30f, bottom = -1e30f;
    for (const auto& corner : corners(box)) {
        const auto p = project(camera, corner);
        if (p.depth <= nearPlane)
            return true;
        left = std::min(left, p.x);
        right = std::max(right, p.x);
        top = std::min(top, p.y);
        bottom = std::max(bottom, p.y);
    }
    const auto foot = project(camera, body.feet);
    if (foot.depth <= nearPlane)
        return true;
    const float scale = focal() / foot.depth;
    return right >= foot.x + (body.crop.left - body.anchorX) * body.width * scale - 1 &&
           left <= foot.x + (body.crop.right - body.anchorX) * body.width * scale + 1 &&
           bottom >= foot.y - (body.anchorY - body.crop.top) * body.height * scale - 1 &&
           top <= foot.y - (body.anchorY - body.crop.bottom) * body.height * scale + 1;
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
        if (box.leaf)
            continue;
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

void prepareLeafQueries(const WorldFrame& frame, const Billboard* body, LeafQueries& out,
                        bool cullToBody) noexcept {
    out.count = 0;
    for (unsigned i = 0; i < frame.doorCount; ++i) {
        const auto& box = frame.doors[i];
        if (!box.leaf || (body && cullToBody && !overlapsProjection(frame.camera, *body, box)))
            continue;
        const float xx = box.x.x * box.x.x + box.x.z * box.x.z;
        const float zz = box.z.x * box.z.x + box.z.z * box.z.z;
        LeafQuery q{{box.origin.x, box.origin.y, box.origin.z, box.y.y},
                    {box.x.x / xx, 0, box.x.z / xx, 0},
                    {box.z.x / zz, 0, box.z.z / zz, 0}};
        if (body) {
            const auto e = extents(*body);
            float left = e.left, right = e.right;
            // Resolve straddling of the extended broad slab, not the finite leaf
            // volume. A finite-volume switch pops a whole shoulder into view at
            // the free tip. Ray coverage below still uses the actual finite box.
            const float at =
                (body->feet.x - box.origin.x) * q.zAxis[0] + (body->feet.z - box.origin.z) * q.zAxis[2];
            const float delta = frame.camera.right.x * q.zAxis[0] + frame.camera.right.z * q.zAxis[2];
            const bool intersects = slab(at, delta, 0, 1, left, right);
            if (intersects) {
                const float anchor =
                    (body->feet.x - box.origin.x) * q.zAxis[0] + (body->feet.z - box.origin.z) * q.zAxis[2];
                const float camera = (frame.camera.position.x - box.origin.x) * q.zAxis[0] +
                                     (frame.camera.position.z - box.origin.z) * q.zAxis[2];
                const float tolerance = 1e-5f / std::sqrt(zz);
                const auto side = [tolerance](float p) {
                    return p < -tolerance ? -1 : p > 1 + tolerance ? 1 : 0;
                };
                const int cs = side(camera), as = side(anchor);
                if (cs)
                    q.xMode[3] = as && as == cs ? 1.f : 2.f;
            }
        }
        out.leaves[out.count++] = q;
    }
}

bool LeafQueries::hides(const Camera& camera, Vec ray, float depth,
                        const std::uint8_t* relations) const noexcept {
    for (unsigned i = 0; i < count; ++i) {
        const auto& q = leaves[i];
        const auto relation = relations ? float(relations[i]) : q.xMode[3];
        if (relation == 1)
            continue;
        float enter = nearPlane, leave = camera.farClip;
        const float dx = camera.position.x - q.originHeight[0], dy = camera.position.y - q.originHeight[1],
                    dz = camera.position.z - q.originHeight[2];
        if (!slab(dx * q.xMode[0] + dz * q.xMode[2], ray.x * q.xMode[0] + ray.z * q.xMode[2], 0, 1, enter,
                  leave) ||
            !slab(dy, ray.y, 0, q.originHeight[3], enter, leave) ||
            !slab(dx * q.zAxis[0] + dz * q.zAxis[2], ray.x * q.zAxis[0] + ray.z * q.zAxis[2], 0, 1, enter,
                  leave))
            continue;
        if (relation == 2 || enter <= depth + 1e-5f)
            return true;
    }
    return false;
}
} // namespace blobber::world_render

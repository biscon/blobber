#include "render/world_visibility.hpp"
#include <algorithm>
#include <cmath>
namespace blobber::world_render {
namespace {
constexpr float epsilon = 1e-5f;
float dot(Vec a, Vec b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vec sub(Vec a, Vec b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
struct ClipVertex {
    float x, y, z, u, v;
};
float plane(const ClipVertex& p, unsigned face, float farClip) {
    const float half = viewWidth * .5f / focal();
    switch (face) {
    case 0:
        return p.z - nearPlane;
    case 1:
        return farClip - p.z;
    case 2:
        return p.x + p.z * half;
    case 3:
        return p.z * half - p.x;
    case 4:
        return p.z * viewHeight * .34f / focal() - p.y;
    default:
        return p.y + p.z * viewHeight * .66f / focal();
    }
}
float edge(float ax, float ay, float bx, float by, float x, float y) {
    return (bx - ax) * (y - ay) - (by - ay) * (x - ax);
}
bool inclusive(float ax, float ay, float bx, float by) {
    return by < ay || (by == ay && bx > ax);
}
void triangle(WorldFrame& out, const std::array<Vertex, 3>& vertices, std::uint16_t id,
              const world_assets::CpuAssets* assets = nullptr, unsigned texture = 0, unsigned threshold = 1,
              bool cutoutOpaque = false, const BillboardClips* clips = nullptr,
              const LeafQueries* leaves = nullptr) {
    const unsigned char* mask = nullptr;
    if (assets) {
        if (texture >= assets->masks.size() || assets->masks[texture].empty())
            throw std::logic_error("World alpha mask was not acquired at preparation");
        mask = assets->masks[texture].data();
    }
    std::array<ClipVertex, 16> a{}, b{};
    unsigned count = 3;
    for (unsigned i = 0; i < 3; ++i) {
        const auto delta = sub(vertices[i].position, out.camera.position);
        a[i] = {dot(delta, out.camera.right), delta.y, dot(delta, out.camera.forward), vertices[i].u,
                vertices[i].v};
    }
    for (unsigned face = 0; face < 6 && count; ++face) {
        unsigned n = 0;
        for (unsigned i = 0; i < count; ++i) {
            const auto p = a[i], q = a[(i + 1) % count];
            const float dp = plane(p, face, out.camera.farClip), dq = plane(q, face, out.camera.farClip);
            if (dp >= 0)
                b[n++] = p;
            if ((dp >= 0) != (dq >= 0)) {
                const float t = dp / (dp - dq);
                b[n++] = {p.x + (q.x - p.x) * t, p.y + (q.y - p.y) * t, p.z + (q.z - p.z) * t,
                          p.u + (q.u - p.u) * t, p.v + (q.v - p.v) * t};
            }
        }
        a = b;
        count = n;
    }
    for (unsigned k = 1; k + 1 < count; ++k) {
        std::array<ClipVertex, 3> t{a[0], a[k], a[k + 1]};
        std::array<float, 3> x{}, y{}, iz{};
        for (unsigned i = 0; i < 3; ++i) {
            iz[i] = 1 / t[i].z;
            x[i] = viewWidth * .5f + focal() * t[i].x * iz[i];
            y[i] = viewHeight * .34f - focal() * t[i].y * iz[i];
        }
        float area = edge(x[0], y[0], x[1], y[1], x[2], y[2]);
        if (std::abs(area) < 1e-10f)
            continue;
        if (area < 0) {
            std::swap(t[1], t[2]);
            std::swap(x[1], x[2]);
            std::swap(y[1], y[2]);
            std::swap(iz[1], iz[2]);
            area = -area;
        }
        const int left =
            std::clamp(int(std::ceil(*std::min_element(x.begin(), x.end()) - .5f)), 0, viewWidth);
        const int right =
            std::clamp(int(std::floor(*std::max_element(x.begin(), x.end()) - .5f)) + 1, 0, viewWidth);
        const int top =
            std::clamp(int(std::ceil(*std::min_element(y.begin(), y.end()) - .5f)), 0, viewHeight);
        const int bottom =
            std::clamp(int(std::floor(*std::max_element(y.begin(), y.end()) - .5f)) + 1, 0, viewHeight);
        const std::array<bool, 3> inc{inclusive(x[1], y[1], x[2], y[2]), inclusive(x[2], y[2], x[0], y[0]),
                                      inclusive(x[0], y[0], x[1], y[1])};
        for (int py = top; py < bottom; ++py) {
            float e0 = edge(x[1], y[1], x[2], y[2], left + .5f, py + .5f);
            float e1 = edge(x[2], y[2], x[0], y[0], left + .5f, py + .5f);
            float e2 = edge(x[0], y[0], x[1], y[1], left + .5f, py + .5f);
            for (int px = left; px < right; ++px, e0 += y[1] - y[2], e1 += y[2] - y[0], e2 += y[0] - y[1]) {
                if (e0 < 0 || (e0 == 0 && !inc[0]) || e1 < 0 || (e1 == 0 && !inc[1]) || e2 < 0 ||
                    (e2 == 0 && !inc[2]))
                    continue;
                const float z = area / (e0 * iz[0] + e1 * iz[1] + e2 * iz[2]);
                const auto pixel = std::size_t(py) * viewWidth + px;
                if (leaves) {
                    const float lateral = (px + .5f - viewWidth * .5f) / focal();
                    const Vec ray{out.camera.forward.x + out.camera.right.x * lateral,
                                  (viewHeight * .34f - py - .5f) / focal(),
                                  out.camera.forward.z + out.camera.right.z * lateral};
                    if (leaves->hides(out.camera, ray, z))
                        continue;
                }
                if (assets) {
                    if (z >= out.picking.opaqueDepth(pixel) - epsilon)
                        continue;
                    const auto old = out.picking.slot(px, py);
                    const bool covered = z > out.picking.depth(pixel) + epsilon ||
                                         (std::abs(z - out.picking.depth(pixel)) <= epsilon &&
                                          old < NpcWorld::maxInstances && id >= NpcWorld::maxInstances);
                    const bool candidate = !cutoutOpaque && id < NpcWorld::maxInstances;
                    if (covered && (!candidate || out.picking.exposed(id)))
                        continue;
                    const float u =
                        (e0 * t[0].u * iz[0] + e1 * t[1].u * iz[1] + e2 * t[2].u * iz[2]) * z / area;
                    const float v =
                        (e0 * t[0].v * iz[0] + e1 * t[1].v * iz[1] + e2 * t[2].v * iz[2]) * z / area;
                    if (clips && clips->contains(u, v))
                        continue;
                    const auto& tex = assets->publication.textures[texture];
                    const unsigned sx =
                        unsigned(std::clamp(int(std::floor(u * tex.width)), 0, int(tex.width) - 1));
                    const unsigned sy =
                        unsigned(std::clamp(int(std::floor(v * tex.height)), 0, int(tex.height) - 1));
                    if (mask[std::size_t(sy) * tex.width + sx] < threshold)
                        continue;
                    if (candidate)
                        out.picking.expose(id);
                    if (covered)
                        continue;
                } else {
                    out.picking.recordOpaqueDepth(pixel, z);
                    if (z >= out.picking.depth(pixel))
                        continue;
                }
                out.picking.writeDepth(pixel, id, z, !assets || cutoutOpaque);
            }
        }
    }
}
void quad(WorldFrame& out, const std::array<Vec, 4>& p, std::uint16_t id,
          const world_assets::CpuAssets* assets = nullptr, unsigned texture = 0, unsigned threshold = 1,
          const BillboardClips* clips = nullptr, const LeafQueries* leaves = nullptr) {
    const std::array<Vertex, 4> v{{{p[0], {}, 0, 1}, {p[1], {}, 1, 1}, {p[2], {}, 1, 0}, {p[3], {}, 0, 0}}};
    triangle(out, {v[0], v[1], v[2]}, id, assets, texture, threshold, false, clips, leaves);
    triangle(out, {v[0], v[2], v[3]}, id, assets, texture, threshold, false, clips, leaves);
}

} // namespace
void VisibilityPipeline::rasterForDiagnostics() {
    if (!ready_ || frame().rasterized)
        return;
    auto& out = frames_[current_];
    out.picking.clearRaster();
    raster(out, true);
    out.rasterized = true;
    // Moving interaction remains unavailable. Diagnostics consume only raster IDs/depth.
}
void VisibilityPipeline::raster(WorldFrame& out, bool rebuildOpaque) {
    if (rebuildOpaque) {
        DrawList draws;
        cull(scene_, out.camera, draws);
        for (unsigned r = 0; r < draws.count; ++r) {
            const auto& range = scene_.ranges[draws.ranges[r]];
            if (range.variant != noContainer &&
                out.containerOpened[range.id - PickingBuffer::containerBase] != bool(range.variant))
                continue;
            for (unsigned i = range.first; i < range.first + range.count; i += 3) {
                const std::array<Vertex, 3> v{scene_.vertices[scene_.indices[i]],
                                              scene_.vertices[scene_.indices[i + 1]],
                                              scene_.vertices[scene_.indices[i + 2]]};
                if (dot(v[0].normal, sub(out.camera.position, v[0].position)) <= 0)
                    continue;
                if (range.cutout)
                    triangle(out, v, range.id, &assets_, pack_.materials[range.material],
                             world_assets::meshAlphaThreshold, true);
                else
                    triangle(out, v, range.id);
            }
        }
        for (const auto& decal : scene_.decals) {
            const auto texture = decal.texture != UINT16_MAX ? decal.texture
                                 : decal.motif               ? pack_.vine
                                                             : pack_.crest;
            const auto& p = decal.vertices;
            triangle(out, {p[0], p[1], p[2]}, PickingBuffer::empty, &assets_, texture, 16, true);
            triangle(out, {p[0], p[2], p[3]}, PickingBuffer::empty, &assets_, texture, 16, true);
        }
        constexpr unsigned faces[6][4]{{0, 1, 3, 2}, {4, 6, 7, 5}, {0, 2, 6, 4},
                                       {1, 5, 7, 3}, {2, 3, 7, 6}, {0, 4, 5, 1}};
        for (unsigned i = 0; i < out.doorCount; ++i) {
            if (out.doors[i].leaf)
                continue;
            const auto p = corners(out.doors[i]);
            for (const auto& f : faces)
                quad(out, {p[f[0]], p[f[1]], p[f[2]], p[f[3]]}, out.doors[i].id);
        }
        opaqueCache_.copyRaster(out.picking);
    } else
        out.picking.copyRaster(opaqueCache_);
    for (std::size_t p = 0; p < viewWidth * viewHeight; ++p)
        out.bodyOpaqueDepth[p] = out.picking.opaqueDepth(p);
    LeafQueries leaves;
    prepareLeafQueries(out, nullptr, leaves);
    for (unsigned i = 0; i < out.count; ++i) {
        const auto& b = out.billboards[i];
        if (b.id >= NpcWorld::maxInstances)
            quad(out, corners(out.camera, b), b.id, &assets_, b.texture, 16, nullptr, &leaves);
    }
    compositionScratch_.copyRaster(out.picking);
    constexpr unsigned faces[6][4]{{0, 1, 3, 2}, {4, 6, 7, 5}, {0, 2, 6, 4},
                                   {1, 5, 7, 3}, {2, 3, 7, 6}, {0, 4, 5, 1}};
    for (unsigned i = 0; i < out.doorCount; ++i) {
        if (!out.doors[i].leaf)
            continue;
        const auto p = corners(out.doors[i]);
        for (const auto& f : faces)
            quad(out, {p[f[0]], p[f[1]], p[f[2]], p[f[3]]}, out.doors[i].id);
    }
    out.picking.swapRaster(compositionScratch_);
    BillboardClips clips;
    for (unsigned i = 0; i < out.count; ++i) {
        const auto& b = out.billboards[i];
        if (b.id >= NpcWorld::maxInstances)
            continue;
        billboardClips(out, b, clips);
        prepareLeafQueries(out, &b, leaves, true);
        quad(out, corners(out.camera, b), b.id, &assets_, b.texture, 1, &clips, &leaves);
    }
    out.picking.compositeBodies(compositionScratch_);
}
void VisibilityPipeline::finishPicking(WorldFrame& out, const FrameInput& in, bool rebuildOpaque) {
    out.picking.clearRaster();
    raster(out, rebuildOpaque);
    out.picking.finish(in.actors);
    bool lootPixels = in.loot && in.loot->containerCount;
    for (unsigned i = 0; in.loot && !lootPixels && i < out.count; ++i)
        lootPixels = out.billboards[i].id >= NpcWorld::maxInstances &&
                     out.billboards[i].id < PickingBuffer::fixtureBase;
    if (lootPixels) {
        for (int y = 0; y < viewHeight; ++y) {
            for (int x = 0; x < viewWidth; ++x) {
                if (auto id = out.picking.pickLoot(x, y))
                    out.visibleLoot[id->slot] = *id;
                else if (auto chest = out.picking.pickContainer(x, y))
                    out.visibleContainers[chest->slot] = *chest;
            }
        }
    }
    if (in.loot)
        for (unsigned i = 0; i < lootCapacity; ++i) {
            const auto& stack = in.loot->stacks[i];
            if (stack.active && stack.container != noContainer && out.visibleContainers[stack.container] &&
                out.containerOpened[stack.container])
                out.visibleLoot[i] = {in.loot->world, std::uint16_t(i), stack.generation};
        }
}
} // namespace blobber::world_render

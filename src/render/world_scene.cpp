#include "render/world_scene.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <stdexcept>
namespace blobber::world_render {
float focal() noexcept {
    return viewWidth * .5f / std::tan(84 * pi / 360);
}
Camera camera(const MovementState& s) noexcept {
    const auto& t = s.transition;
    const auto a = t.active() ? t.from : s.player, b = t.active() ? t.to : s.player;
    const float p = t.active() ? t.progress() : 1, e = p * p * (3 - 2 * p);
    float yaw = float(a.heading) * pi / 2, delta = float(b.heading) * pi / 2 - yaw;
    if (delta > pi)
        delta -= 2 * pi;
    if (delta < -pi)
        delta += 2 * pi;
    yaw += delta * e;
    const Vec f{std::sin(yaw), 0, -std::cos(yaw)}, r{std::cos(yaw), 0, std::sin(yaw)};
    return {{a.cell.x + .5f + (b.cell.x - a.cell.x) * e - .4f * f.x, .5f,
             a.cell.y + .5f + (b.cell.y - a.cell.y) * e - .4f * f.z},
            f,
            r,
            yaw};
}
ScreenPoint project(const Camera& c, Vec p) noexcept {
    const float dx = p.x - c.position.x, dz = p.z - c.position.z;
    const float d = dx * c.forward.x + dz * c.forward.z;
    return {viewWidth * .5f + focal() * (dx * c.right.x + dz * c.right.z) / d,
            viewHeight * .34f - focal() * (p.y - c.position.y) / d, d};
}
bool sameCamera(const Camera& a, const Camera& b) noexcept {
    return a.position.x == b.position.x && a.position.y == b.position.y && a.position.z == b.position.z &&
           a.yaw == b.yaw;
}
std::size_t Scene::bytes() const noexcept {
    return sizeof(*this) + vertices.capacity() * sizeof(Vertex) + indices.capacity() * 4 +
           ranges.capacity() * sizeof(Range) + chunks.capacity() * sizeof(Chunk);
}
std::size_t Scene::meshBytes() const noexcept {
    return vertices.size() * sizeof(Vertex) + indices.size() * 4;
}
namespace {
struct Face {
    std::array<Vertex, 4> v;
    std::uint8_t material;
};
template <class F>
void faces(GeometryView m, int cx, int cz, Surfaces defaults, const std::vector<Surfaces>& overrides,
           F emit) {
    auto add = [&](std::array<Vec, 4> p, Vec n, unsigned material, bool wall) {
        Face q{};
        q.material = std::uint8_t(material);
        for (unsigned i = 0; i < 4; ++i)
            q.v[i] = {p[i], n, wall ? (n.x != 0 ? p[i].z : p[i].x) : p[i].x, wall ? 1 - p[i].y : p[i].z};
        emit(q);
    };
    for (int z = cz; z < std::min(cz + 8, m.height()); ++z)
        for (int x = cx; x < std::min(cx + 8, m.width()); ++x) {
            if (m.solid({x, z}))
                continue;
            const auto s = overrides.empty() ? defaults : overrides[std::size_t(z) * m.width() + x];
            const float a = float(x), b = float(z);
            add({Vec{a, 0, b}, Vec{a, 0, b + 1}, Vec{a + 1, 0, b + 1}, Vec{a + 1, 0, b}}, {0, 1, 0}, s.floor,
                false);
            add({Vec{a, 1, b}, Vec{a + 1, 1, b}, Vec{a + 1, 1, b + 1}, Vec{a, 1, b + 1}}, {0, -1, 0},
                s.ceiling, false);
            if (m.solid({x, z - 1}))
                add({Vec{a, 0, b}, Vec{a + 1, 0, b}, Vec{a + 1, 1, b}, Vec{a, 1, b}}, {0, 0, 1}, s.wall,
                    true);
            if (m.solid({x, z + 1}))
                add({Vec{a + 1, 0, b + 1}, Vec{a, 0, b + 1}, Vec{a, 1, b + 1}, Vec{a + 1, 1, b + 1}},
                    {0, 0, -1}, s.wall, true);
            if (m.solid({x - 1, z}))
                add({Vec{a, 0, b + 1}, Vec{a, 0, b}, Vec{a, 1, b}, Vec{a, 1, b + 1}}, {1, 0, 0}, s.wall,
                    true);
            if (m.solid({x + 1, z}))
                add({Vec{a + 1, 0, b}, Vec{a + 1, 0, b + 1}, Vec{a + 1, 1, b + 1}, Vec{a + 1, 1, b}},
                    {-1, 0, 0}, s.wall, true);
        }
}
} // namespace
Scene prepare(GeometryView m, Surfaces defaults, const std::vector<Surfaces>& overrides, Limits limits) {
    if (!overrides.empty() && overrides.size() != std::size_t(m.width()) * m.height())
        throw std::invalid_argument("World material grid dimensions differ");
    auto valid = [](Surfaces s) { return s.wall < 32 && s.floor < 32 && s.ceiling < 32; };
    if (!valid(defaults) || !std::all_of(overrides.begin(), overrides.end(), valid))
        throw std::invalid_argument("World material index exceeds 32");
    limits.quads = std::min(limits.quads, std::size_t(262144));
    limits.ranges = std::min(limits.ranges, std::size_t(65536));
    limits.cpuBytes = std::min(limits.cpuBytes, std::size_t(64u * 1024 * 1024));
    Scene s;
    s.width = m.width();
    s.height = m.height();
    s.columns = (m.width() + 7) / 8;
    s.rows = (m.height() + 7) / 8;
    std::size_t quads = 0, ranges = 0;
    for (int z = 0; z < m.height(); z += 8)
        for (int x = 0; x < m.width(); x += 8) {
            std::array<bool, 32> used{};
            faces(m, x, z, defaults, overrides, [&](const Face& f) {
                ++quads;
                used[f.material] = true;
            });
            ranges += std::count(used.begin(), used.end(), true);
            if (quads > limits.quads || ranges > limits.ranges)
                throw std::runtime_error("World geometry capacity exceeded");
        }
    const std::size_t required =
        sizeof(s) + quads * 152 + ranges * sizeof(Range) + std::size_t(s.columns) * s.rows * sizeof(Chunk);
    if (required > limits.cpuBytes)
        throw std::runtime_error("World CPU scene budget exceeded");
    s.vertices.reserve(quads * 4);
    s.indices.reserve(quads * 6);
    s.ranges.reserve(ranges);
    s.chunks.resize(std::size_t(s.columns) * s.rows);
    for (int z = 0; z < m.height(); z += 8)
        for (int x = 0; x < m.width(); x += 8) {
            std::array<Face, 384> local{};
            unsigned count = 0;
            faces(m, x, z, defaults, overrides, [&](const Face& f) { local[count++] = f; });
            std::sort(local.begin(), local.begin() + count,
                      [](const Face& a, const Face& b) { return a.material < b.material; });
            auto& chunk = s.chunks[(z / 8) * s.columns + x / 8];
            chunk.first = std::uint32_t(s.ranges.size());
            for (unsigned i = 0; i < count; ++i) {
                if (!i || local[i].material != local[i - 1].material) {
                    s.ranges.push_back({std::uint32_t(s.indices.size()), 0, local[i].material});
                    ++chunk.count;
                }
                const auto first = std::uint32_t(s.vertices.size());
                s.vertices.insert(s.vertices.end(), local[i].v.begin(), local[i].v.end());
                for (unsigned k : {0u, 1u, 2u, 0u, 2u, 3u})
                    s.indices.push_back(first + k);
                s.ranges.back().count += 6;
            }
        }
    if (s.bytes() > limits.cpuBytes)
        throw std::runtime_error("World CPU capacity budget exceeded");
    static std::atomic<std::uint64_t> next{1};
    s.identity = next.fetch_add(1);
    return s;
}
bool visible(const Camera& c, int x, int z, int width, int height) noexcept {
    std::array<bool, 6> inside{};
    for (int dx : {0, width})
        for (int dz : {0, height})
            for (int y : {0, 1}) {
                const float rx = x + dx - c.position.x, rz = z + dz - c.position.z;
                const float depth = rx * c.forward.x + rz * c.forward.z,
                            side = rx * c.right.x + rz * c.right.z;
                const float up = y - c.position.y, half = depth * viewWidth * .5f / focal();
                constexpr float eps = 1e-4f;
                inside[0] |= depth >= nearPlane - eps;
                inside[1] |= depth <= farPlane + eps;
                inside[2] |= side >= -half - eps;
                inside[3] |= side <= half + eps;
                inside[4] |= up <= depth * viewHeight * .34f / focal() + eps;
                inside[5] |= up >= -depth * viewHeight * .66f / focal() - eps;
            }
    return std::all_of(inside.begin(), inside.end(), [](bool v) { return v; });
}
void cull(const Scene& s, const Camera& c, DrawList& out) noexcept {
    out.count = out.tested = 0;
    const int left = std::max(0, int(std::floor((c.position.x - 24) / 8))),
              right = std::min(s.columns - 1, int(std::floor((c.position.x + 24) / 8)));
    const int top = std::max(0, int(std::floor((c.position.z - 24) / 8))),
              bottom = std::min(s.rows - 1, int(std::floor((c.position.z + 24) / 8)));
    std::array<unsigned, 49> chunks{};
    unsigned n = 0;
    for (int z = top; z <= bottom; ++z)
        for (int x = left; x <= right; ++x) {
            ++out.tested;
            if (visible(c, x * 8, z * 8, std::min(8, s.width - x * 8), std::min(8, s.height - z * 8)))
                chunks[n++] = z * s.columns + x;
        }
    for (unsigned mat = 0; mat < 32; ++mat)
        for (unsigned i = 0; i < n; ++i) {
            const auto& chunk = s.chunks[chunks[i]];
            for (unsigned k = 0; k < chunk.count; ++k)
                if (s.ranges[chunk.first + k].material == mat)
                    out.ranges[out.count++] = chunk.first + k;
        }
}
} // namespace blobber::world_render

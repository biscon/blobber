#include "render/world_scene.hpp"
#include "core/preparation.hpp"
#include "render/picking.hpp"
#include "render/world_stairs.hpp"
#include "world/view.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <stdexcept>
namespace blobber::world_render {
float focal() noexcept {
    return viewWidth * .5f / partyViewHalfTangent;
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
    return {{a.cell.x + .5f + (b.cell.x - a.cell.x) * e - partyCameraSetback * f.x, .5f,
             a.cell.y + .5f + (b.cell.y - a.cell.y) * e - partyCameraSetback * f.z},
            f,
            r,
            yaw};
}
Camera camera(const MovementState& s, GeometryView g) noexcept {
    auto c = camera(s);
    const auto& t = s.transition;
    const auto a = t.active() ? t.from : s.player, b = t.active() ? t.to : s.player;
    const float p = t.active() ? t.progress() : 1, e = p * p * (3 - 2 * p);
    c.position.y += elevation(g.surface(a.cell).floor) * (1 - e) + elevation(g.surface(b.cell).floor) * e;
    return c;
}
float daylight(const MovementState& s, GeometryView g) noexcept {
    const auto& t = s.transition;
    const float p = t.active() ? t.progress() : 1, e = p * p * (3 - 2 * p);
    const auto a = t.active() ? t.from : s.player, b = t.active() ? t.to : s.player;
    return float(g.surface(a.cell).environment == Environment::Daylight) * (1 - e) +
           float(g.surface(b.cell).environment == Environment::Daylight) * e;
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
           ranges.capacity() * sizeof(Range) + chunks.capacity() * sizeof(Chunk) +
           decals.capacity() * sizeof(Decal);
}
std::size_t Scene::meshBytes() const noexcept {
    return vertices.size() * sizeof(Vertex) + indices.size() * 4;
}
namespace {
struct Face {
    std::array<Vertex, 4> v;
    std::uint8_t material;
    std::uint16_t id = UINT16_MAX;
    bool painted = false;
    std::uint8_t variant = noContainer;
};
template <class F>
void faces(GeometryView m, int cx, int cz, Surfaces defaults, const std::vector<Surfaces>& overrides, F emit,
           const world_assets::CpuAssets* assets, const world_assets::Pack& pack,
           const std::array<world_assets::StaticModelId, buildingCapacity>& buildings) {
    auto add = [&](std::array<Vec, 4> p, Vec n, unsigned material, bool wall) {
        Face q{};
        q.material = std::uint8_t(material);
        for (unsigned i = 0; i < 4; ++i)
            q.v[i] = {p[i], n, wall ? (n.x != 0 ? p[i].z : p[i].x) : p[i].x, wall ? 1 - p[i].y : p[i].z};
        emit(q);
    };
    for (int z = cz; z < std::min(cz + 8, m.height()); ++z)
        for (int x = cx; x < std::min(cx + 8, m.width()); ++x) {
            if (m.base().solid({x, z}))
                continue;
            const auto h = m.surface({x, z});
            auto s = overrides.empty() ? Surfaces{h.wallMaterial, h.floorMaterial, h.ceilingMaterial}
                                       : overrides[std::size_t(z) * m.width() + x];
            if (overrides.empty() && (defaults.wall != 0 || defaults.floor != 1 || defaults.ceiling != 2))
                s = defaults;
            const float a = float(x), b = float(z), lo = elevation(h.floor),
                        hi = h.ceiling == noCeiling ? m.base().upper() : elevation(h.ceiling);
            const auto* fixtureOwner = m.fixtures();
            const int fixtureSlot = fixtureOwner ? fixtureOwner->at({x, z}) : -1;
            if (fixtureSlot >= 0 &&
                stairFixture(fixtureOwner->definition().entries[unsigned(fixtureSlot)].kind)) {
                const auto stairs =
                    stairGeometry(m, fixtureOwner->definition().entries[unsigned(fixtureSlot)],
                                  {s.wall, s.floor, s.ceiling, defaults.stairWall, defaults.stairFloor});
                for (unsigned i = 0; i < stairs.count; ++i)
                    emit(Face{stairs.quads[i].vertices, stairs.quads[i].material,
                              std::uint16_t(PickingBuffer::fixtureBase + fixtureSlot)});
                continue;
            }
            add({Vec{a, lo, b}, Vec{a, lo, b + 1}, Vec{a + 1, lo, b + 1}, Vec{a + 1, lo, b}}, {0, 1, 0},
                s.floor, false);
            if (h.ceiling != noCeiling)
                add({Vec{a, hi, b}, Vec{a + 1, hi, b}, Vec{a + 1, hi, b + 1}, Vec{a, hi, b + 1}}, {0, -1, 0},
                    s.ceiling, false);
            for (unsigned d = 0; d < 4; ++d) {
                const auto f = forward(Heading(d)), r = right(Heading(d));
                const Cell next = Cell{x, z} + f;
                const auto n = m.surface(next);
                auto wall = [&](float low, float high) {
                    if (high <= low)
                        return;
                    const float px = a + .5f + f.x * .5f, pz = b + .5f + f.y * .5f;
                    add({Vec{px - r.x * .5f, low, pz - r.y * .5f}, Vec{px + r.x * .5f, low, pz + r.y * .5f},
                         Vec{px + r.x * .5f, high, pz + r.y * .5f},
                         Vec{px - r.x * .5f, high, pz - r.y * .5f}},
                        {float(-f.x), 0, float(-f.y)}, s.wall, true);
                };
                if (m.base().solid(next)) {
                    if (!m.base().buildingOwned(next))
                        wall(lo, hi);
                } else {
                    wall(lo, std::min(hi, elevation(n.floor)));
                    if (n.ceiling != noCeiling)
                        wall(std::max(lo, elevation(n.ceiling)), hi);
                }
            }
        }
    const auto& placed = m.base().definition().buildings;
    for (unsigned i = 0; i < placed.size(); ++i) {
        const auto& b = placed[i];
        if (b.anchor.x < cx || b.anchor.x >= cx + 8 || b.anchor.y < cz || b.anchor.y >= cz + 8)
            continue;
        const auto r = right(b.facing), f = forward(b.facing);
        for (auto part : pack.buildings[buildings[i].index].parts) {
            const auto& mesh = assets->mesh(part);
            const auto material =
                std::find(pack.materials.begin(), pack.materials.begin() + pack.materialCount, mesh.texture) -
                pack.materials.begin();
            for (unsigned n = 0; n < mesh.vertices.size(); n += 3) {
                Face q{};
                q.material = std::uint8_t(material);
                q.painted = true;
                for (unsigned k = 0; k < 3; ++k) {
                    const auto& v = mesh.vertices[n + k];
                    q.v[k] = {{b.anchor.x + .5f + r.x * v[0] - f.x * v[2],
                               elevation(m.surface(b.anchor).floor) + v[1],
                               b.anchor.y + .5f + r.y * v[0] - f.y * v[2]},
                              {r.x * v[3] - f.x * v[5], v[4], r.y * v[3] - f.y * v[5]},
                              v[6],
                              v[7]};
                }
                q.v[3] = q.v[2];
                emit(q);
            }
        }
    }
    auto block = [&](float x, float z, float base, float width, float depth, float height, Cell r, Cell f,
                     unsigned material) {
        std::array<Vec, 4> bottom{}, top{};
        for (unsigned k = 0; k < 4; ++k) {
            const float u = (k == 0 || k == 3) ? -.5f : .5f, v = k < 2 ? -.5f : .5f;
            bottom[k] = {x + r.x * u * width + f.x * v * depth, base, z + r.y * u * width + f.y * v * depth};
            top[k] = bottom[k];
            top[k].y += height;
        }
        for (unsigned k = 0; k < 4; ++k) {
            const unsigned n = (k + 1) % 4;
            const float dx = bottom[n].x - bottom[k].x, dz = bottom[n].z - bottom[k].z,
                        len = std::sqrt(dx * dx + dz * dz);
            add({bottom[k], bottom[n], top[n], top[k]}, {-dz / len, 0, dx / len}, material, true);
        }
        add({top[0], top[1], top[2], top[3]}, {0, 1, 0}, material, false);
    };
    for (const auto& d : m.base().definition().decorations) {
        const bool local = d.cell.x >= cx && d.cell.x < cx + 8 && d.cell.y >= cz && d.cell.y < cz + 8;
        if (local && d.kind == DecorationKind::DoorSurround) {
            const auto f = forward(d.face), r = right(d.face);
            const float x = d.cell.x + .5f, z = d.cell.y + .5f, base = elevation(m.surface(d.cell).floor);
            for (int side : {-1, 1})
                block(x + r.x * .475f * side, z + r.y * .475f * side, base, .05f, .08f, .96f, r, f,
                      d.material);
            block(x, z, base + .96f, 1.f, .08f, .04f, r, f, d.material);
            continue;
        }
        if (local && (d.kind == DecorationKind::BonesA || d.kind == DecorationKind::BonesB)) {
            // Small chips lift the floor art's silhouette without camera-facing cards.
            const float x = d.cell.x + .5f, z = d.cell.y + .5f,
                        base = elevation(m.surface(d.cell).floor) + .006f;
            block(x - .11f, z + .07f, base, .19f, .035f, .022f, {1, 0}, {0, -1}, d.material);
            block(x + .12f, z - .10f, base, .04f, .16f, .018f, {1, 0}, {0, -1}, d.material);
        }
        if (d.cutout() || d.cell.x < cx || d.cell.x >= cx + 8 || d.cell.y < cz || d.cell.y >= cz + 8)
            continue;
        const auto f = forward(d.face), r = right(d.face);
        if (d.model()) {
            if (!assets)
                throw std::runtime_error("Props require prepared mesh assets");
            const unsigned first = d.kind == DecorationKind::Chest  ? 0
                                   : d.kind == DecorationKind::Cage ? 2
                                                                    : 3;
            const unsigned last = d.kind == DecorationKind::Chest ? 2 : first + 1;
            float wallward = d.kind == DecorationKind::Rack ? .5f : 0.f;
            if (d.kind == DecorationKind::Chest) {
                float back = 0;
                for (unsigned variant = 0; variant < 2; ++variant) {
                    if (pack.models[variant] >= assets->publication.meshes.size())
                        throw std::runtime_error("Missing chest mesh binding");
                    for (const auto& v : assets->mesh(pack.models[variant]).vertices)
                        back = std::max(back, -.5f * v[2]);
                }
                // One transform for both states: leave clearance for the open lid.
                wallward = std::max(0.f, .5f - back - .0125f);
            }
            const float cx = d.cell.x + .5f + f.x * wallward;
            const float cz = d.cell.y + .5f + f.y * wallward;
            for (unsigned model = first; model < last; ++model) {
                if (pack.models[model] >= assets->publication.meshes.size())
                    throw std::runtime_error("Missing prop mesh binding");
                const auto& mesh = assets->mesh(pack.models[model]);
                const float scale = d.kind == DecorationKind::Chest ? .5f : 1.f;
                unsigned material = 0;
                while (material < pack.materialCount && pack.materials[material] != mesh.texture)
                    ++material;
                if (material == pack.materialCount)
                    throw std::runtime_error("Missing prop atlas binding");
                for (unsigned n = 0; n < mesh.vertices.size(); n += 3) {
                    Face q{};
                    q.material = std::uint8_t(material);
                    q.painted = true;
                    if (d.kind == DecorationKind::Chest) {
                        q.id = std::uint16_t(PickingBuffer::containerBase + d.container);
                        q.variant = std::uint8_t(model);
                    }
                    for (unsigned k = 0; k < 3; ++k) {
                        const auto& a = mesh.vertices[n + k];
                        q.v[k] = {{cx + scale * (r.x * a[0] - f.x * a[2]),
                                   elevation(m.surface(d.cell).floor) + scale * a[1],
                                   cz + scale * (r.y * a[0] - f.y * a[2])},
                                  {r.x * a[3] - f.x * a[5], a[4], r.y * a[3] - f.y * a[5]},
                                  a[6],
                                  a[7]};
                    }
                    q.v[3] = q.v[2];
                    emit(q);
                }
            }
            continue;
        }
        const float px = d.cell.x + .5f + f.x * (.5f - d.depth * .5f) + r.x * (d.offset - .5f),
                    pz = d.cell.y + .5f + f.y * (.5f - d.depth * .5f) + r.y * (d.offset - .5f),
                    base = elevation(m.surface(d.cell).floor);
        const unsigned sides = d.kind == DecorationKind::Urn ? 8 : 4;
        std::array<Vec, 8> bottom{}, top{};
        for (unsigned k = 0; k < sides; ++k) {
            float u, v;
            if (sides == 4) {
                u = (k == 0 || k == 3) ? -.5f : .5f;
                v = k < 2 ? -.5f : .5f;
            } else {
                const float angle = 2 * pi * k / sides;
                u = .5f * std::cos(angle);
                v = .5f * std::sin(angle);
            }
            bottom[k] = {px + r.x * u * d.width + f.x * v * d.depth, base,
                         pz + r.y * u * d.width + f.y * v * d.depth};
            top[k] = bottom[k];
            top[k].y += d.height;
        }
        const Vec center{px, base + d.height, pz};
        for (unsigned k = 0; k < sides; ++k) {
            const unsigned n = (k + 1) % sides;
            const float dx = bottom[n].x - bottom[k].x, dz = bottom[n].z - bottom[k].z,
                        len = std::sqrt(dx * dx + dz * dz);
            add({bottom[k], bottom[n], top[n], top[k]}, {-dz / len, 0, dx / len}, d.material, true);
            add({center, top[k], top[n], center}, {0, 1, 0}, d.material, false);
        }
    }
}
} // namespace
Scene prepare(GeometryView m, Surfaces defaults, const std::vector<Surfaces>& overrides, Limits limits,
              const world_assets::CpuAssets* assets) {
    if (!overrides.empty() && overrides.size() != std::size_t(m.width()) * m.height())
        throw std::invalid_argument("World material grid dimensions differ");
    auto valid = [](Surfaces s) {
        return s.wall < 32 && s.floor < 32 && s.ceiling < 32 && s.stairWall < 32 && s.stairFloor < 32;
    };
    if (!valid(defaults) || !std::all_of(overrides.begin(), overrides.end(), valid))
        throw std::invalid_argument("World material index exceeds 32");
    limits.quads = std::min(limits.quads, std::size_t(262144));
    limits.ranges = std::min(limits.ranges, std::size_t(65536));
    limits.cpuBytes = std::min(limits.cpuBytes, std::size_t(64u * 1024 * 1024));
    // Bounded preparation lease; transformed geometry owns its vertices after return.
    const auto lease = assets ? assets->acquire(assets->requirements(m.base().definition().assetPack))
                              : world_assets::AssetLease{};
    Scene s;
    if (!m.base().definition().buildings.empty())
        s.cullRadius = buildingCullRadius;
    const auto pack = assets ? assets->pack(m.base().definition().assetPack) : world_assets::Pack{};
    std::array<world_assets::StaticModelId, buildingCapacity> buildings{};
    for (unsigned i = 0; i < m.base().definition().buildings.size(); ++i) {
        if (!assets)
            throw std::invalid_argument("Buildings require prepared mesh assets");
        buildings[i] = pack.building(m.base().definition().buildings[i].model);
    }
    s.map = &m.base();
    for (auto h : m.base().definition().surfaces)
        s.outdoor |= h.ceiling == noCeiling;
    for (const auto& d : m.base().definition().decorations)
        if (d.cutout()) {
            const auto f = forward(d.face), r = right(d.face);
            const float x = d.cell.x + .5f + f.x * .499f + r.x * (d.offset - .5f),
                        z = d.cell.y + .5f + f.y * .499f + r.y * (d.offset - .5f),
                        y = elevation(m.surface(d.cell).floor) + d.elevation;
            const float w = d.width * .5f, h = d.height * .5f;
            const Vec normal{float(-f.x), 0, float(-f.y)};
            const auto before = s.decals.size();
            s.decals.push_back({{{{{x - r.x * w, y - h, z - r.y * w}, normal, 0, 1},
                                  {{x + r.x * w, y - h, z + r.y * w}, normal, 1, 1},
                                  {{x + r.x * w, y + h, z + r.y * w}, normal, 1, 0},
                                  {{x - r.x * w, y + h, z - r.y * w}, normal, 0, 0}}},
                                unsigned(d.kind)});
            auto& decal = s.decals[before];
            if (unsigned(d.kind) >= unsigned(DecorationKind::DampA)) {
                if (!assets || pack.decorations[unsigned(d.kind)] == UINT16_MAX)
                    throw std::runtime_error("Missing decoration artwork");
                decal.texture = pack.decorations[unsigned(d.kind)];
            }
            if (d.kind == DecorationKind::DampA || d.kind == DecorationKind::DampB) {
                const auto& texture = assets->publication.textures[decal.texture];
                const auto& bounds = assets->bounds[decal.texture];
                // Anchor visible ink, not the transparent canvas, at floor level.
                const float bottom = y + h - d.height * float(bounds.bottom) / texture.height;
                const float shift = elevation(m.surface(d.cell).floor) - bottom;
                for (auto& v : decal.vertices)
                    v.position.y += shift;
            }
            if (d.floorCutout()) {
                const float yy = elevation(m.surface(d.cell).floor) + d.elevation;
                const float xx = d.cell.x + .5f, zz = d.cell.y + .5f;
                decal.vertices = {{{{xx - w, yy, zz + h}, {0, 1, 0}, 0, 1},
                                   {{xx + w, yy, zz + h}, {0, 1, 0}, 1, 1},
                                   {{xx + w, yy, zz - h}, {0, 1, 0}, 1, 0},
                                   {{xx - w, yy, zz - h}, {0, 1, 0}, 0, 0}}};
            } else if (d.web()) {
                // From a point on the backing wall to a point on its adjoining wall.
                const auto side = right(d.face);
                const float centerX = d.cell.x + .5f, centerZ = d.cell.y + .5f;
                const float ax = centerX + f.x * .498f - side.x * .20f,
                            az = centerZ + f.y * .498f - side.y * .20f;
                const float bx = centerX + side.x * .498f - f.x * .20f,
                            bz = centerZ + side.y * .498f - f.y * .20f;
                for (unsigned k = 0; k < 4; ++k) {
                    decal.vertices[k].position.x = (k == 0 || k == 3) ? ax : bx;
                    decal.vertices[k].position.z = (k == 0 || k == 3) ? az : bz;
                }
            }
        }
    s.width = m.width();
    s.height = m.height();
    s.columns = (m.width() + 7) / 8;
    s.rows = (m.height() + 7) / 8;
    std::size_t quads = 0, ranges = 0, maxLocal = 0;
    for (int z = 0; z < m.height(); z += 8)
        for (int x = 0; x < m.width(); x += 8) {
            preparationPoint("Preparing dungeon");
            std::array<bool, 32 * (fixtureCapacity + containerCapacity + 1) * 4> used{};
            std::size_t localCount = 0;
            faces(
                m, x, z, defaults, overrides,
                [&](const Face& f) {
                    ++quads;
                    ++localCount;
                    used[f.material +
                         32 * ((f.id == UINT16_MAX ? 0u : unsigned(f.id - PickingBuffer::fixtureBase) + 1) *
                                   4 +
                               unsigned(f.painted) * 2 + (f.variant == 1 ? 1 : 0))] = true;
                },
                assets, pack, buildings);
            maxLocal = std::max(maxLocal, localCount);
            ranges += std::count(used.begin(), used.end(), true);
            if (quads > limits.quads || ranges > limits.ranges)
                throw std::runtime_error("World geometry capacity exceeded");
        }
    const std::size_t required = sizeof(s) + s.decals.capacity() * sizeof(Decal) +
                                 quads * (4 * sizeof(Vertex) + 6 * sizeof(std::uint32_t)) +
                                 ranges * sizeof(Range) + std::size_t(s.columns) * s.rows * sizeof(Chunk);
    if (required > limits.cpuBytes)
        throw std::runtime_error("World CPU scene budget exceeded");
    s.vertices.reserve(quads * 4);
    s.indices.reserve(quads * 6);
    s.ranges.reserve(ranges);
    s.chunks.resize(std::size_t(s.columns) * s.rows);
    std::vector<Face> local(maxLocal);
    for (int z = 0; z < m.height(); z += 8)
        for (int x = 0; x < m.width(); x += 8) {
            preparationPoint("Preparing dungeon");
            unsigned count = 0;
            faces(
                m, x, z, defaults, overrides, [&](const Face& f) { local[count++] = f; }, assets, pack,
                buildings);
            std::sort(local.begin(), local.begin() + count, [](const Face& a, const Face& b) {
                return a.material != b.material ? a.material < b.material
                       : a.id != b.id           ? a.id < b.id
                       : a.painted != b.painted ? a.painted < b.painted
                                                : a.variant < b.variant;
            });
            auto& chunk = s.chunks[(z / 8) * s.columns + x / 8];
            chunk.low = m.base().upper();
            chunk.high = m.base().lower();
            int left = x, top = z, right = std::min(x + 8, m.width()), bottom = std::min(z + 8, m.height());
            for (unsigned i = 0; i < count; ++i)
                for (const auto& v : local[i].v) {
                    chunk.low = std::min(chunk.low, v.position.y);
                    chunk.high = std::max(chunk.high, v.position.y);
                    left = std::min(left, int(std::floor(v.position.x)));
                    right = std::max(right, int(std::ceil(v.position.x)));
                    top = std::min(top, int(std::floor(v.position.z)));
                    bottom = std::max(bottom, int(std::ceil(v.position.z)));
                }
            chunk.left = left;
            chunk.top = top;
            chunk.width = right - left;
            chunk.depth = bottom - top;
            for (const auto& d : s.decals)
                for (const auto& v : d.vertices)
                    if (v.position.x >= x && v.position.x <= x + 8 && v.position.z >= z &&
                        v.position.z <= z + 8) {
                        chunk.low = std::min(chunk.low, v.position.y);
                        chunk.high = std::max(chunk.high, v.position.y);
                    }
            if (chunk.low > chunk.high) {
                chunk.low = m.base().lower();
                chunk.high = m.base().upper();
            }
            chunk.first = std::uint32_t(s.ranges.size());
            for (unsigned i = 0; i < count; ++i) {
                if (!i ||
                    (local[i].material != local[i - 1].material || local[i].id != local[i - 1].id ||
                     local[i].painted != local[i - 1].painted || local[i].variant != local[i - 1].variant)) {
                    s.ranges.push_back({std::uint32_t(s.indices.size()), 0, local[i].material, local[i].id,
                                        local[i].painted, local[i].variant});
                    ++chunk.count;
                }
                const auto first = std::uint32_t(s.vertices.size());
                s.vertices.insert(s.vertices.end(), local[i].v.begin(), local[i].v.end());
                for (unsigned k : {0u, 1u, 2u, 0u, 2u, 3u})
                    s.indices.push_back(first + k);
                s.ranges.back().count += 6;
            }
        }
    // Static ranges have at most two paint modes per material in each candidate
    // chunk. Fixture/container variants use the separately bounded global slots.
    for (const auto& chunk : s.chunks) {
        unsigned staticRanges = 0;
        for (unsigned k = 0; k < chunk.count; ++k)
            staticRanges += s.ranges[chunk.first + k].id == UINT16_MAX;
        if (staticRanges > 2 * materialCapacity)
            throw std::runtime_error("Static chunk draw capacity exceeded");
    }
    if (s.bytes() > limits.cpuBytes)
        throw std::runtime_error("World CPU capacity budget exceeded");
    static std::atomic<std::uint64_t> next{1};
    s.identity = next.fetch_add(1);
    return s;
}
bool visible(const Camera& c, int x, int z, int width, int height, float low, float high) noexcept {
    std::array<bool, 6> inside{};
    for (int dx : {0, width})
        for (int dz : {0, height})
            for (float y : {low, high}) {
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
    const int left = std::max(0, int(std::floor((c.position.x - s.cullRadius) / 8))),
              right = std::min(s.columns - 1, int(std::floor((c.position.x + s.cullRadius) / 8)));
    const int top = std::max(0, int(std::floor((c.position.z - s.cullRadius) / 8))),
              bottom = std::min(s.rows - 1, int(std::floor((c.position.z + s.cullRadius) / 8)));
    std::array<unsigned, buildingCullChunks> chunks{};
    unsigned n = 0;
    for (int z = top; z <= bottom; ++z)
        for (int x = left; x <= right; ++x) {
            ++out.tested;
            const auto& chunk = s.chunks[z * s.columns + x];
            if (visible(c, chunk.left, chunk.top, chunk.width, chunk.depth, chunk.low, chunk.high))
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

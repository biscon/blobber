#include "render/world_visibility.hpp"
#include "game/combat.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
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
float plane(const ClipVertex& p, unsigned face) {
    const float half = viewWidth * .5f / focal();
    switch (face) {
    case 0:
        return p.z - nearPlane;
    case 1:
        return farPlane - p.z;
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
              bool cutoutOpaque = false) {
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
            const float dp = plane(p, face), dq = plane(q, face);
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
                if (assets) {
                    if (z >= out.picking.opaqueDepth(pixel) - epsilon ||
                        z > out.picking.depth(pixel) + epsilon)
                        continue;
                    const auto old = out.picking.slot(px, py);
                    if (std::abs(z - out.picking.depth(pixel)) <= epsilon && old < NpcWorld::maxInstances &&
                        id >= NpcWorld::maxInstances)
                        continue;
                    const float u =
                        (e0 * t[0].u * iz[0] + e1 * t[1].u * iz[1] + e2 * t[2].u * iz[2]) * z / area;
                    const float v =
                        (e0 * t[0].v * iz[0] + e1 * t[1].v * iz[1] + e2 * t[2].v * iz[2]) * z / area;
                    const auto& tex = assets->publication.textures[texture];
                    const unsigned sx =
                        unsigned(std::clamp(int(std::floor(u * tex.width)), 0, int(tex.width) - 1));
                    const unsigned sy =
                        unsigned(std::clamp(int(std::floor(v * tex.height)), 0, int(tex.height) - 1));
                    if (assets->masks[texture][std::size_t(sy) * tex.width + sx] < threshold)
                        continue;
                } else if (z >= out.picking.depth(pixel))
                    continue;
                out.picking.writeDepth(pixel, id, z, !assets || cutoutOpaque);
            }
        }
    }
}
void quad(WorldFrame& out, const std::array<Vec, 4>& p, std::uint16_t id,
          const world_assets::CpuAssets* assets = nullptr, unsigned texture = 0, unsigned threshold = 1) {
    const std::array<Vertex, 4> v{{{p[0], {}, 0, 1}, {p[1], {}, 1, 1}, {p[2], {}, 1, 0}, {p[3], {}, 0, 0}}};
    triangle(out, {v[0], v[1], v[2]}, id, assets, texture, threshold);
    triangle(out, {v[0], v[2], v[3]}, id, assets, texture, threshold);
}
template <class F> void cube(Cell c, F emit) {
    const float x = float(c.x), z = float(c.y);
    emit(std::array<Vec, 4>{{{x, 0, z}, {x + 1, 0, z}, {x + 1, 1, z}, {x, 1, z}}}, Vec{0, 0, -1});
    emit(std::array<Vec, 4>{{{x + 1, 0, z + 1}, {x, 0, z + 1}, {x, 1, z + 1}, {x + 1, 1, z + 1}}},
         Vec{0, 0, 1});
    emit(std::array<Vec, 4>{{{x, 0, z + 1}, {x, 0, z}, {x, 1, z}, {x, 1, z + 1}}}, Vec{-1, 0, 0});
    emit(std::array<Vec, 4>{{{x + 1, 0, z}, {x + 1, 0, z + 1}, {x + 1, 1, z + 1}, {x + 1, 1, z}}},
         Vec{1, 0, 0});
    emit(std::array<Vec, 4>{{{x, 1, z}, {x + 1, 1, z}, {x + 1, 1, z + 1}, {x, 1, z + 1}}}, Vec{0, 1, 0});
    emit(std::array<Vec, 4>{{{x, 0, z + 1}, {x + 1, 0, z + 1}, {x + 1, 0, z}, {x, 0, z}}}, Vec{0, -1, 0});
}
unsigned sideFor(Heading facing, float x, float z) {
    const auto f = forward(facing), r = right(facing);
    const float front = x * f.x + z * f.y, side = x * r.x + z * r.y;
    if (std::abs(front) >= std::abs(side))
        return front >= 0 ? 0u : 1u;
    return side >= 0 ? 3u : 2u;
}
Vec playerCenter(const MovementState& s) {
    const auto& t = s.transition;
    const float p = t.active() ? t.progress() : 1, e = p * p * (3 - 2 * p);
    const auto a = t.active() ? t.from : s.player, b = t.active() ? t.to : s.player;
    return {a.cell.x + .5f + (b.cell.x - a.cell.x) * e, 0, a.cell.y + .5f + (b.cell.y - a.cell.y) * e};
}
float fit(float lateral, float depth, float halfWidth) {
    const float f = depth - .4f;
    if (f > .25f && f < 1.75f) {
        const float limit = std::max(0.f, (viewWidth * .5f - 2) * depth / focal() - halfWidth);
        const float blend =
            std::min({std::clamp((f - .25f) * 2, 0.f, 1.f), std::clamp((1.75f - f) * 2, 0.f, 1.f),
                      std::clamp((1.75f - std::abs(lateral)) * 2, 0.f, 1.f)});
        if (std::abs(lateral) > .25f) {
            const float scale = std::clamp((limit - .25f), 0.f, 1.f);
            const float fitted = .25f + (std::abs(lateral) - .25f) * scale;
            lateral = std::copysign(std::abs(lateral) + (fitted - std::abs(lateral)) * blend, lateral);
        }
    }
    return lateral;
}
} // namespace
VisibilityPipeline::VisibilityPipeline(const Scene& scene, const world_assets::CpuAssets& assets)
    : scene_(scene), assets_(assets) {
    unsigned materials = 0;
    for (const auto& texture : assets.publication.textures)
        materials += !texture.clamp;
    for (const auto& range : scene.ranges)
        if (range.material >= materials)
            throw std::invalid_argument("Scene references an unavailable world material");
    for (const auto& decal : scene.decals)
        (void)assets.named(decal.motif ? "decoration/vine" : "decoration/crest");
    icons_.fill(UINT16_MAX);
    constexpr const char* names[]{"loot/bag",         "fixture/key-bronze", "fixture/key-silver",
                                  "fixture/upstairs", "fixture/downstairs", "fixture/endpoint",
                                  "fixture/threshold"};
    for (unsigned i = 0; i < 7; ++i)
        for (unsigned t = 0; t < assets.publication.textures.size(); ++t)
            if (assets.publication.textures[t].name == names[i])
                icons_[i] = t;
    if (std::any_of(icons_.begin(), icons_.end(), [](unsigned texture) { return texture == UINT16_MAX; }))
        throw std::runtime_error(
            "Missing prepared world interaction sprites; rebuild blobber_bake_world_assets");
    if (bytes() > viewScratchBudget)
        throw std::runtime_error("World visibility scratch budget exceeded");
}
std::size_t VisibilityPipeline::bytes() const noexcept {
    return sizeof(*this) + 3u * viewWidth * viewHeight * (sizeof(std::uint16_t) + 2 * sizeof(float));
}
bool VisibilityPipeline::lootVisible(LootId id) const noexcept {
    return ready_ && !frame().moving && id.slot < lootCapacity && frame().visibleLoot[id.slot] == id;
}
bool VisibilityPipeline::update(const FrameInput& in) {
    if ((scene_.map && scene_.map != &in.geometry.base()) || (map_ && map_ != &in.geometry.base()))
        throw std::invalid_argument("World visibility belongs to another map; prepare a new view");
    if (in.geometry.width() != scene_.width || in.geometry.height() != scene_.height)
        throw std::invalid_argument("World visibility geometry dimensions differ");
    if (in.actors && &in.actors->map().base() != &in.geometry.base())
        throw std::invalid_argument("World actors belong to another map");
    const auto* actors = in.actors ? &in.actors->snapshot() : nullptr;
    const auto* anim = in.animation;
    if (actors && (actors->count > NpcWorld::maxInstances ||
                   actors->typeCount != assets_.publication.appearances.size()))
        throw std::invalid_argument("World appearance/actor counts differ");
    if (anim && (!actors || anim->count != actors->count || anim->world != actors->world ||
                 anim->actorRevision != actors->revision))
        throw std::invalid_argument("World animation belongs to another actor snapshot");
    if (anim)
        for (unsigned i = 0; i < anim->count; ++i)
            if (!std::isfinite(anim->frames[i].elevation) || anim->frames[i].elevation < -8 ||
                anim->frames[i].elevation > 8 || !std::isfinite(anim->frames[i].offsetX) ||
                !std::isfinite(anim->frames[i].offsetY) ||
                std::abs(anim->frames[i].offsetX) > Map::maxDimension ||
                std::abs(anim->frames[i].offsetY) > Map::maxDimension)
                throw std::invalid_argument("Invalid world animation travel offset");
    if (in.doors && in.doors->world != in.geometry.world())
        throw std::invalid_argument("Foreign door snapshot");
    const auto* fixtures = in.geometry.fixtures();
    if (fixtures && fixtures->definition().count > fixtureCapacity)
        throw std::invalid_argument("Too many world fixtures");
    const auto c = camera(in.movement, in.geometry);
    const std::array<std::uint64_t, 10> stamp{in.geometry.world(),
                                              in.geometry.revision(),
                                              fixtures ? fixtures->revision() : 0,
                                              actors ? actors->world : 0,
                                              actors ? actors->revision : 0,
                                              anim ? anim->revision : 0,
                                              in.loot ? in.loot->world : 0,
                                              in.loot ? in.loot->revision : 0,
                                              in.doors ? in.doors->revision : 0,
                                              in.doors && in.doors->active() ? in.doors->now : 0};
    if (ready_ && stamp == stamp_ && sameCamera(c, frame().camera) &&
        frame().moving == (in.movement.transition.active() || (in.doors && in.doors->active())))
        return false;
    auto& out = frames_[1 - current_];
    out.picking.clear();
    out.visibleLoot = {};
    out.count = out.proxyCount = 0;
    out.camera = c;
    out.moving = in.movement.transition.active() || (in.doors && in.doors->active());
    const bool opaqueChanged = !ready_ || !sameCamera(c, frame().camera) || stamp[0] != stamp_[0] ||
                               stamp[1] != stamp_[1] || stamp[2] != stamp_[2] || stamp[8] != stamp_[8] ||
                               stamp[9] != stamp_[9];
    build(out, in);
    raster(out, opaqueChanged);
    out.picking.finish(in.actors);
    if (in.loot)
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x)
                if (auto id = out.picking.pickLoot(x, y))
                    out.visibleLoot[id->slot] = *id;
    out.revision = frame().revision + 1;
    map_ = &in.geometry.base();
    current_ = 1 - current_;
    stamp_ = stamp;
    ready_ = true;
    return true;
}
void VisibilityPipeline::build(WorldFrame& out, const FrameInput& in) {
    const auto center = playerCenter(in.movement);
    unsigned ordinary = 0, locked = 0;
    if (in.geometry.fixtures()) {
        ordinary = assets_.named("door");
        locked = assets_.named("door-locked");
    }
    out.doorCount = doorBoxes(in.geometry, in.doors, out.doors, ordinary, locked, 0);
    const auto append = [&](Billboard b) {
        if (out.count == billboardCapacity)
            throw std::runtime_error("World billboard capacity exceeded");
        out.billboards[out.count++] = b;
    };
    if (in.actors)
        for (unsigned i = 0; i < in.actors->snapshot().count; ++i) {
            const auto& actor = in.actors->snapshot().actors[i];
            const auto selection = in.animation ? in.animation->frames[i] : NpcFrameSelection{};
            if (actor.life == ActorLife::Removed || !selection.visible ||
                (!in.animation && actor.life != ActorLife::Alive))
                continue;
            const auto& npc = actor.npc;
            if (npc.type.index >= assets_.publication.appearances.size())
                throw std::invalid_argument("Invalid world appearance index");
            const auto& a = assets_.publication.appearances[npc.type.index];
            const Vec foot{npc.cell.x + .25f + npc.subcell.x * .5f + selection.offsetX,
                           in.animation ? selection.elevation : actorElevation(actor, in.geometry, in.now),
                           npc.cell.y + .25f + npc.subcell.y * .5f + selection.offsetY};
            const float travelX = foot.x - (npc.cell.x + .25f + npc.subcell.x * .5f),
                        travelZ = foot.z - (npc.cell.y + .25f + npc.subcell.y * .5f);
            const unsigned side = unsigned(sideFor(npc.facing, center.x - .5f - npc.cell.x - travelX,
                                                   center.z - .5f - npc.cell.y - travelZ));
            unsigned clip = unsigned(in.animation ? selection.clip : npc.animation),
                     f = in.animation ? selection.active : 0;
            if (clip >= npcAnimationCount || !a.clips[clip].timing.count)
                throw std::invalid_argument("Absent world animation clip");
            if (a.clips[clip].textures[side][0] == UINT16_MAX && clip == unsigned(NpcAnimation::Attack)) {
                clip = 0;
                f = selection.idle;
            }
            const auto& cl = a.clips[clip];
            if (f >= cl.timing.count || cl.textures[side][f] == UINT16_MAX)
                throw std::invalid_argument("Absent world animation direction/frame");
            const auto texture = cl.textures[side][f];
            const auto anchor = assets_.publication.anchors[texture];
            const float depth = dot(sub(foot, out.camera.position), out.camera.forward);
            if (depth < nearPlane || depth > farPlane)
                continue;
            const float lateral = dot(sub(foot, out.camera.position), out.camera.right);
            const float fitted = fit(lateral, depth, assets_.edgeHalfWidths[npc.type.index]);
            const Vec feet{foot.x + out.camera.right.x * (fitted - lateral), 0,
                           foot.z + out.camera.right.z * (fitted - lateral)};
            Billboard b{feet,
                        cl.width ? cl.width : a.width,
                        a.height,
                        anchor.x,
                        anchor.y,
                        depth,
                        a.width * .45f,
                        texture,
                        std::uint16_t(i),
                        actor.life == ActorLife::Alive,
                        selection.flash};
            b.release = a.attachments.release[side];
            b.torso = a.attachments.torso[side];
            append(b);
            auto canonical = a.clips[0].textures[0][0];
            float top = 0;
            if (clip == unsigned(NpcAnimation::Prone) && a.clips[clip].textures[side][0] != UINT16_MAX) {
                canonical = a.clips[clip].textures[side][0];
                top = float(assets_.bounds[canonical].top) / assets_.publication.textures[canonical].height;
            }
            const auto badge = project(
                out.camera,
                {feet.x, feet.y + a.height * (assets_.publication.anchors[canonical].y - top), feet.z});
            out.picking.badgeAnchor(std::uint16_t(i), int(std::round(badge.x)), int(std::round(badge.y)),
                                    depth);
        }
    if (in.loot) {
        out.picking.lootWorld(in.loot->world);
        unsigned count = 0;
        for (unsigned i = 0; i < lootCapacity; ++i)
            if (in.loot->stacks[i].active)
                lootOrder_[count++] = std::uint16_t(i);
        std::sort(lootOrder_.begin(), lootOrder_.begin() + count, [&](auto a, auto b) {
            const auto x = in.loot->stacks[a].cell, y = in.loot->stacks[b].cell;
            return x.y != y.y ? x.y < y.y : x.x != y.x ? x.x < y.x : a < b;
        });
        for (unsigned j = 0; j < count; ++j) {
            const auto i = lootOrder_[j];
            const auto& stack = in.loot->stacks[i];
            if (j && stack.cell == in.loot->stacks[lootOrder_[j - 1]].cell)
                continue;
            Vec feet{stack.cell.x + .5f, elevation(in.geometry.surface(stack.cell).floor),
                     stack.cell.y + .5f};
            float depth = dot(sub(feet, out.camera.position), out.camera.forward);
            if (depth < nearPlane || depth > farPlane)
                continue;
            if (icons_[0] == UINT16_MAX)
                throw std::invalid_argument("Missing prepared loot/bag");
            float lateral = dot(sub(feet, out.camera.position), out.camera.right);
            const float forwardPosition = depth - .4f;
            if (forwardPosition > .25f && forwardPosition < 1.75f && std::abs(lateral) > .5f &&
                std::abs(lateral) < 1.75f) {
                const float limit = (viewWidth * .5f - 32) * depth / focal();
                const float blend = std::min({std::clamp((forwardPosition - .25f) * 2, 0.f, 1.f),
                                              std::clamp((1.75f - forwardPosition) * 2, 0.f, 1.f),
                                              std::clamp((1.75f - std::abs(lateral)) * 2, 0.f, 1.f)});
                const float fitted =
                    lateral + (std::copysign(std::min(std::abs(lateral), limit), lateral) - lateral) * blend;
                feet.x += out.camera.right.x * (fitted - lateral);
                feet.z += out.camera.right.z * (fitted - lateral);
            }
            const float size = std::clamp(focal() * .25f / depth, 4.f, 64.f) * depth / focal();
            const auto p = project(out.camera, feet);
            // Retain the current-cell bag's four-pixel margin above the viewport foot.
            feet.y += std::max(0.f, p.y - (viewHeight - 4)) * depth / focal();
            append({feet, size, size, .5f, 1, depth, 0, std::uint16_t(icons_[0]),
                    std::uint16_t(NpcWorld::maxInstances + i)});
            out.picking.lootGeneration(std::uint16_t(i), stack.generation);
            const auto anchor = project(out.camera, feet);
            out.picking.lootAnchor(std::uint16_t(i), std::clamp(int(std::round(anchor.x)), 0, viewWidth - 1),
                                   std::clamp(int(std::round(anchor.y)), 0, viewHeight - 1));
        }
    }
    if (const auto* f = in.geometry.fixtures()) {
        out.picking.fixtureWorld(f->world());
        for (unsigned i = 0; i < f->definition().count; ++i) {
            const auto& entry = f->definition().entries[i];
            if (entry.kind == FixtureKind::Door || entry.kind == FixtureKind::GuardianGate)
                continue;
            if (entry.kind == FixtureKind::Key && f->completed(i))
                continue;
            const unsigned icon = entry.kind == FixtureKind::Key          ? (entry.key == 2 ? 1 : 2)
                                  : entry.kind == FixtureKind::Upstairs   ? 3
                                  : entry.kind == FixtureKind::Downstairs ? 4
                                  : entry.kind == FixtureKind::Endpoint   ? 5
                                                                          : 6;
            if (icons_[icon] == UINT16_MAX)
                throw std::invalid_argument("Missing prepared fixture marker");
            Vec feet{entry.cell.x + .5f, elevation(in.geometry.surface(entry.cell).floor),
                     entry.cell.y + .5f};
            const float depth = dot(sub(feet, out.camera.position), out.camera.forward);
            if (depth < nearPlane || depth > farPlane)
                continue;
            const float size = std::clamp(focal() * .25f / depth, 4.f, 64.f) * depth / focal();
            const auto p = project(out.camera, feet);
            feet.y += std::max(0.f, p.y - (viewHeight - 4)) * depth / focal();
            append({feet, size, size, .5f, 1, depth, 0, std::uint16_t(icons_[icon]),
                    std::uint16_t(PickingBuffer::fixtureBase + i)});
        }
    }
    std::sort(out.billboards.begin(), out.billboards.begin() + out.count, [](const auto& a, const auto& b) {
        if (a.depth != b.depth)
            return a.depth > b.depth;
        if ((a.id < NpcWorld::maxInstances) != (b.id < NpcWorld::maxInstances))
            return a.id >= NpcWorld::maxInstances;
        if (a.alive != b.alive)
            return !a.alive;
        return a.id < b.id;
    });
}
void VisibilityPipeline::raster(WorldFrame& out, bool rebuildOpaque) {
    if (rebuildOpaque) {
        DrawList draws;
        cull(scene_, out.camera, draws);
        for (unsigned r = 0; r < draws.count; ++r) {
            const auto& range = scene_.ranges[draws.ranges[r]];
            for (unsigned i = range.first; i < range.first + range.count; i += 3) {
                const std::array<Vertex, 3> v{scene_.vertices[scene_.indices[i]],
                                              scene_.vertices[scene_.indices[i + 1]],
                                              scene_.vertices[scene_.indices[i + 2]]};
                if (dot(v[0].normal, sub(out.camera.position, v[0].position)) <= 0)
                    continue;
                triangle(out, v, PickingBuffer::empty);
            }
        }
        for (const auto& decal : scene_.decals) {
            const auto texture = assets_.named(decal.motif ? "decoration/vine" : "decoration/crest");
            const auto& p = decal.vertices;
            triangle(out, {p[0], p[1], p[2]}, PickingBuffer::empty, &assets_, texture, 16, true);
            triangle(out, {p[0], p[2], p[3]}, PickingBuffer::empty, &assets_, texture, 16, true);
        }
        constexpr unsigned faces[6][4]{{0, 1, 3, 2}, {4, 6, 7, 5}, {0, 2, 6, 4},
                                       {1, 5, 7, 3}, {2, 3, 7, 6}, {0, 4, 5, 1}};
        for (unsigned i = 0; i < out.doorCount; ++i) {
            const auto p = corners(out.doors[i]);
            for (const auto& f : faces)
                quad(out, {p[f[0]], p[f[1]], p[f[2]], p[f[3]]}, out.doors[i].id);
        }
        opaqueCache_.copyRaster(out.picking);
    } else
        out.picking.copyRaster(opaqueCache_);
    for (unsigned i = 0; i < out.count; ++i) {
        const auto& b = out.billboards[i];
        quad(out, corners(out.camera, b), b.id, &assets_, b.texture, b.id < NpcWorld::maxInstances ? 1 : 16);
    }
}
std::optional<Cell> VisibilityPipeline::groundCell(const FrameInput& in, int x, int y) const noexcept {
    if (!ready_ || map_ != &in.geometry.base() || (in.doors && in.doors->active()) ||
        in.movement.transition.active() || x < 0 || x >= viewWidth || y < 0 || y >= viewHeight)
        return {};
    if (in.geometry.world() != stamp_[0] || in.geometry.revision() != stamp_[1] ||
        (in.doors ? in.doors->revision : 0) != stamp_[8])
        return {};
    const auto c = camera(in.movement, in.geometry);
    if (!sameCamera(c, frame().camera))
        return {};
    const float lateral = (x + .5f - viewWidth * .5f) / focal(), vy = (viewHeight * .34f - y - .5f) / focal();
    if (vy >= 0)
        return {};
    const float vx = c.forward.x + c.right.x * lateral, vz = c.forward.z + c.right.z * lateral;
    float start = nearPlane;
    for (unsigned visit = 0; visit < 64 && start < farPlane; ++visit) {
        const float probe = start + 1e-5f;
        const Cell cell{int(std::floor(c.position.x + vx * probe)),
                        int(std::floor(c.position.z + vz * probe))};
        float end = farPlane;
        if (std::abs(vx) > 1e-8f)
            end = std::min(end, ((vx > 0 ? cell.x + 1 : cell.x) - c.position.x) / vx);
        if (std::abs(vz) > 1e-8f)
            end = std::min(end, ((vz > 0 ? cell.y + 1 : cell.y) - c.position.z) / vz);
        if (in.geometry.base().solid(cell))
            return {};
        const float d = (elevation(in.geometry.surface(cell).floor) - c.position.y) / vy;
        if (d >= start - epsilon && d <= end + epsilon && d >= nearPlane && d <= farPlane) {
            if (d > frame().picking.opaqueDepth(std::size_t(y) * viewWidth + x) + epsilon)
                return {};
            return spellCell(in.geometry, in.movement.player, cell) ? std::optional<Cell>{cell}
                                                                    : std::nullopt;
        }
        if (end <= start)
            return {};
        start = end;
    }
    return {};
}
std::array<Vec, 4> corners(const Camera& c, const Billboard& b) noexcept {
    const float left = -b.anchorX * b.width, right = (1 - b.anchorX) * b.width,
                bottom = (b.anchorY - 1) * b.height, top = b.anchorY * b.height;
    return {{{b.feet.x + c.right.x * left, b.feet.y + bottom, b.feet.z + c.right.z * left},
             {b.feet.x + c.right.x * right, b.feet.y + bottom, b.feet.z + c.right.z * right},
             {b.feet.x + c.right.x * right, b.feet.y + top, b.feet.z + c.right.z * right},
             {b.feet.x + c.right.x * left, b.feet.y + top, b.feet.z + c.right.z * left}}};
}
Attachment attachment(const Camera& c, const Billboard& b, NpcAttachment a) noexcept {
    return {{b.feet.x + c.right.x * b.width * (a.x - b.anchorX), b.feet.y + b.height * (b.anchorY - a.y),
             b.feet.z + c.right.z * b.width * (a.x - b.anchorX)},
            b.depth,
            b.depth >= nearPlane && b.depth <= farPlane};
}
Scene billboardMesh() {
    Scene s;
    s.vertices = {{{-.5f, 0, 0}, {0, 0, 1}, 0, 1},
                  {{.5f, 0, 0}, {0, 0, 1}, 1, 1},
                  {{.5f, 1, 0}, {0, 0, 1}, 1, 0},
                  {{-.5f, 1, 0}, {0, 0, 1}, 0, 0}};
    s.indices = {0, 1, 2, 0, 2, 3};
    return s;
}
Scene proxyMesh() {
    Scene s;
    cube({0, 0}, [&](const auto& p, Vec n) {
        const auto first = std::uint32_t(s.vertices.size());
        for (unsigned i = 0; i < 4; ++i)
            s.vertices.push_back({p[i], n, float(i == 1 || i == 2), float(i < 2)});
        for (unsigned k : {0u, 1u, 2u, 0u, 2u, 3u})
            s.indices.push_back(first + k);
    });
    return s;
}
} // namespace blobber::world_render

#include "render/world_visibility.hpp"
#include "game/combat.hpp"
#include "render/world_projection_parameters.hpp"
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
} // namespace
VisibilityPipeline::VisibilityPipeline(const Scene& scene, const world_assets::CpuAssets& assets)
    : scene_(scene), assets_(assets) {
    pack_ = assets_.pack(scene_.map ? scene_.map->definition().assetPack : "dungeon");
    for (const auto& range : scene.ranges)
        if (range.material >= pack_.materialCount)
            throw std::invalid_argument("Scene references an unavailable pack material");
    icons_.fill(UINT16_MAX);
    constexpr const char* names[]{"loot/bag", "fixture/key-bronze", "fixture/key-silver", "fixture/endpoint",
                                  "fixture/threshold"};
    for (unsigned i = 0; i < icons_.size(); ++i)
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
    return sizeof(*this) + pack_.dynamicBytes() +
           4u * viewWidth * viewHeight * (sizeof(std::uint16_t) + 2 * sizeof(float)) +
           2u * viewWidth * viewHeight * sizeof(float);
}
bool VisibilityPipeline::lootVisible(LootId id) const noexcept {
    return ready_ && !frame().moving && id.slot < lootCapacity && frame().visibleLoot[id.slot] == id;
}
bool VisibilityPipeline::update(const FrameInput& in, bool rasterMoving) {
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
    const auto c = camera(in.movement, in.geometry, scene_.farClip);
    const std::array<std::uint64_t, 11> stamp{in.geometry.world(),
                                              in.geometry.revision(),
                                              fixtures ? fixtures->revision() : 0,
                                              actors ? actors->world : 0,
                                              actors ? actors->revision : 0,
                                              anim ? anim->revision : 0,
                                              in.loot ? in.loot->world : 0,
                                              in.loot ? in.loot->revision : 0,
                                              in.doors ? in.doors->revision : 0,
                                              in.doors && in.doors->active() ? in.doors->now : 0,
                                              in.guardianDoor};
    if (ready_ && stamp == stamp_ && sameCamera(c, frame().camera) &&
        frame().moving == (in.movement.transition.active() || (in.doors && in.doors->active())) &&
        (!rasterMoving || !frame().moving || frame().rasterized))
        return false;
    auto& out = frames_[1 - current_];
    out.picking.clearMetadata();
    out.visibleLoot = {};
    out.visibleContainers = {};
    out.count = out.proxyCount = 0;
    out.camera = c;
    out.moving = in.movement.transition.active() || (in.doors && in.doors->active());
    out.rasterized = !out.moving || rasterMoving;
    const bool opaqueChanged = !ready_ || !frame().rasterized || !sameCamera(c, frame().camera) ||
                               stamp[0] != stamp_[0] || stamp[1] != stamp_[1] || stamp[2] != stamp_[2] ||
                               stamp[8] != stamp_[8] || stamp[9] != stamp_[9] || stamp[7] != stamp_[7];
    build(out, in);
    if (out.rasterized)
        finishPicking(out, in, opaqueChanged);
    out.revision = frame().revision + 1;
    map_ = &in.geometry.base();
    current_ = 1 - current_;
    stamp_ = stamp;
    ready_ = true;
    return true;
}
void VisibilityPipeline::build(WorldFrame& out, const FrameInput& in) {
    out.bodyIndex.fill(-1);
    out.containerOpened = {};
    if (in.loot)
        for (unsigned i = 0; i < in.loot->containerCount; ++i)
            out.containerOpened[i] = in.loot->containers[i].opened;
    out.crestTexture = pack_.crest;
    out.vineTexture = pack_.vine;
    const auto center = playerCenter(in.movement);
    out.doorCount = doorBoxes(in.geometry, in.doors, out.doors, pack_.door, pack_.lockedDoor,
                              pack_.materialCount ? pack_.materials[0] : 0, pack_.doorEdge,
                              pack_.guardianEmblem, in.guardianDoor);
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
            // A cosmetic melee lunge must not cross a directional-art sector.
            // Combat holds facing/view through the attack; keep its starting anchor
            // for art selection while geometry and picking follow the animated feet.
            const auto facingPosition =
                actor.life == ActorLife::Alive && actor.hasAttack && selection.clip == NpcAnimation::Attack
                    ? actorPosition(actor, actor.attackTimeMs)
                    : ActorPosition{foot.x, foot.z};
            const float travelX = facingPosition.x - (npc.cell.x + .25f + npc.subcell.x * .5f),
                        travelZ = facingPosition.y - (npc.cell.y + .25f + npc.subcell.y * .5f);
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
            if (depth < nearPlane || depth > out.camera.farClip)
                continue;
            const Vec feet = foot;
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
            b.bodyBounds = a.attachments.bodyBounds[side];
            b.bodyCanvasWidth = a.clips[0].width ? a.clips[0].width : a.width;
            const auto& stored = assets_.publication.textures[texture];
            b.crop = {float(stored.canvas.x) / stored.canvas.width,
                      float(stored.canvas.y) / stored.canvas.height,
                      float(stored.canvas.x + stored.width) / stored.canvas.width,
                      float(stored.canvas.y + stored.height) / stored.canvas.height};
            prepareTerrainClips(in.geometry, out.camera, b, out.terrainClips[i]);
            append(b);
            auto canonical = a.clips[0].textures[0][0];
            if (clip == unsigned(NpcAnimation::Prone) && a.clips[clip].textures[side][0] != UINT16_MAX) {
                canonical = a.clips[clip].textures[side][0];
            }
            if (assets_.masks[canonical].empty() || assets_.masks[texture].empty())
                throw std::logic_error("Actor artwork was not acquired at preparation");
            const auto& canonicalTexture = assets_.publication.textures[canonical];
            const float top = float(canonicalTexture.canvas.y + assets_.bounds[canonical].top) /
                              canonicalTexture.canvas.height;
            const auto badge = project(
                out.camera,
                {feet.x, feet.y + b.height * (assets_.publication.anchors[canonical].y - top), feet.z});
            out.picking.badgeAnchor(std::uint16_t(i), int(std::round(badge.x)), int(std::round(badge.y)),
                                    depth);
        }
    if (in.loot) {
        out.picking.lootWorld(in.loot->world);
        unsigned count = 0;
        for (unsigned i = 0; i < lootCapacity; ++i)
            if (in.loot->stacks[i].active && in.loot->stacks[i].container == noContainer)
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
            if (depth < nearPlane || depth > out.camera.farClip)
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
            const float size = (2.f / 3.f) * std::clamp(focal() * .25f / depth, 4.f, 64.f) * depth / focal();
            const auto p = project(out.camera, feet);
            // Retain the current-cell bag's four-pixel margin above the viewport foot.
            feet.y += std::max(0.f, p.y - (viewHeight - 4)) * depth / focal();
            append({feet, size, size, .5f, 1, depth, size * .42f, std::uint16_t(icons_[0]),
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
            if (entry.kind == FixtureKind::Door || entry.kind == FixtureKind::GuardianGate ||
                stairFixture(entry.kind))
                continue;
            if (entry.kind == FixtureKind::Key && f->completed(i))
                continue;
            const unsigned icon = entry.kind == FixtureKind::Key        ? (entry.key == 2 ? 1 : 2)
                                  : entry.kind == FixtureKind::Endpoint ? 3
                                                                        : 4;
            if (icons_[icon] == UINT16_MAX)
                throw std::invalid_argument("Missing prepared fixture marker");
            Vec feet{entry.cell.x + .5f, elevation(in.geometry.surface(entry.cell).floor),
                     entry.cell.y + .5f};
            const float depth = dot(sub(feet, out.camera.position), out.camera.forward);
            if (depth < nearPlane || depth > out.camera.farClip)
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
    for (unsigned i = 0; i < out.count; ++i)
        if (out.billboards[i].id < NpcWorld::maxInstances)
            out.bodyIndex[out.billboards[i].id] = int(i);
    prepareLeafQueries(out, nullptr, out.leaves);
    LeafQueries bodyLeaves;
    for (unsigned i = 0; i < out.count; ++i) {
        const auto& body = out.billboards[i];
        if (body.id >= NpcWorld::maxInstances)
            continue;
        prepareLeafQueries(out, &body, bodyLeaves);
        for (unsigned leaf = 0; leaf < bodyLeaves.count; ++leaf)
            out.bodyLeafRelations[body.id][leaf] = std::uint8_t(bodyLeaves.leaves[leaf].xMode[3]);
    }
    out.picking.bodyVisibility(&out, [](const void* context, std::uint16_t owner, int x, int y, float depth) {
        const auto& frame = *static_cast<const WorldFrame*>(context);
        if (depth >= frame.bodyOpaqueDepth[y * viewWidth + x] || frame.bodyIndex[owner] < 0)
            return false;
        const float lateral = (x + .5f - viewWidth * .5f) / focal();
        if (frame.leaves.hides(frame.camera,
                               {frame.camera.forward.x + frame.camera.right.x * lateral,
                                (viewHeight * viewHorizon - y - .5f) / focal(),
                                frame.camera.forward.z + frame.camera.right.z * lateral},
                               depth, frame.bodyLeafRelations[owner].data()))
            return false;
        const auto visible = frame.picking.slot(x, y);
        if (visible == owner || frame.picking.depth(y * viewWidth + x) > depth)
            return true;
        // A leaf underneath an owner-bound badge has already been evaluated with
        // that owner's relation. Other bodies and nonbody sprites still occlude.
        for (unsigned i = 0; i < frame.doorCount; ++i)
            if (frame.doors[i].leaf && frame.doors[i].id == visible)
                return true;
        return false;
    });
}
std::optional<Cell> VisibilityPipeline::groundCell(const FrameInput& in, int x, int y) const noexcept {
    if (!ready_ || map_ != &in.geometry.base() || (in.doors && in.doors->active()) ||
        in.movement.transition.active() || x < 0 || x >= viewWidth || y < 0 || y >= viewHeight)
        return {};
    if (in.geometry.world() != stamp_[0] || in.geometry.revision() != stamp_[1] ||
        (in.doors ? in.doors->revision : 0) != stamp_[8])
        return {};
    const auto c = camera(in.movement, in.geometry, scene_.farClip);
    if (!sameCamera(c, frame().camera))
        return {};
    const float lateral = (x + .5f - viewWidth * .5f) / focal(),
                vy = (viewHeight * viewHorizon - y - .5f) / focal();
    if (vy >= 0)
        return {};
    const float vx = c.forward.x + c.right.x * lateral, vz = c.forward.z + c.right.z * lateral;
    float start = nearPlane;
    for (unsigned visit = 0; visit < 128 && start < c.farClip; ++visit) {
        const float probe = start + 1e-5f;
        const Cell cell{int(std::floor(c.position.x + vx * probe)),
                        int(std::floor(c.position.z + vz * probe))};
        float end = c.farClip;
        if (std::abs(vx) > 1e-8f)
            end = std::min(end, ((vx > 0 ? cell.x + 1 : cell.x) - c.position.x) / vx);
        if (std::abs(vz) > 1e-8f)
            end = std::min(end, ((vz > 0 ? cell.y + 1 : cell.y) - c.position.z) / vz);
        if (in.geometry.base().solid(cell))
            return {};
        const float d = (elevation(in.geometry.surface(cell).floor) - c.position.y) / vy;
        if (d >= start - epsilon && d <= end + epsilon && d >= nearPlane && d <= c.farClip) {
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
    const float left = (b.crop.left - b.anchorX) * b.width, right = (b.crop.right - b.anchorX) * b.width,
                bottom = (b.anchorY - b.crop.bottom) * b.height, top = (b.anchorY - b.crop.top) * b.height;
    return {{{b.feet.x + c.right.x * left, b.feet.y + bottom, b.feet.z + c.right.z * left},
             {b.feet.x + c.right.x * right, b.feet.y + bottom, b.feet.z + c.right.z * right},
             {b.feet.x + c.right.x * right, b.feet.y + top, b.feet.z + c.right.z * right},
             {b.feet.x + c.right.x * left, b.feet.y + top, b.feet.z + c.right.z * left}}};
}
Attachment attachment(const Camera& c, const Billboard& b, NpcAttachment a) noexcept {
    return {{b.feet.x + c.right.x * b.width * (a.x - b.anchorX), b.feet.y + b.height * (b.anchorY - a.y),
             b.feet.z + c.right.z * b.width * (a.x - b.anchorX)},
            b.depth,
            b.depth >= nearPlane && b.depth <= c.farClip};
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
    unsigned face = 0;
    cube({0, 0}, [&](const auto& p, Vec n) {
        const auto first = std::uint32_t(s.vertices.size());
        for (unsigned i = 0; i < 4; ++i)
            s.vertices.push_back({p[i], n, float((i == 1 || i == 2) != (face == 1)), float(i < 2)});
        for (unsigned k : {0u, 1u, 2u, 0u, 2u, 3u})
            s.indices.push_back(first + k);
        ++face;
    });
    return s;
}
} // namespace blobber::world_render

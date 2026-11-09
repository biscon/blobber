#include "render/world_effects.hpp"
#include <cmath>
#include <stdexcept>
namespace blobber::world_render {
static_assert(sizeof(EffectsPipeline) + sizeof(std::array<std::array<float, 4>, 1024>) <= 512u * 1024);
namespace {
float depth(const Camera& c, Vec p) {
    return (p.x - c.position.x) * c.forward.x + (p.z - c.position.z) * c.forward.z;
}
const AttackVisual* weaponAttack(const FrameInput& in, unsigned caster) noexcept {
    if (!in.attacks)
        return nullptr;
    const auto& shot = in.attacks->attacks[NpcWorld::maxInstances + caster];
    if (shot.spell != rules::SpellKind::SpiritualWeapon || shot.incoming ||
        shot.stage == AttackStage::Empty || shot.stage == AttackStage::Canceled || in.now < shot.startedMs ||
        in.now >= shot.impactMs + 240)
        return nullptr;
    return &shot;
}
Vec partyAnchor(const AttackVisual& shot, Player player, GeometryView geometry) {
    const auto f = forward(player.heading), r = right(player.heading);
    const float lane = (shot.formationSlot % 2 ? 1.f : -1.f) * .15f;
    return {player.cell.x + .5f + f.x * .1f + r.x * lane,
            elevation(geometry.surface(player.cell).floor) + (shot.formationSlot < 2 ? .2f : .17f),
            player.cell.y + .5f + f.y * .1f + r.y * lane};
}
} // namespace
Vec spiritualWeaponPosition(GeometryView geometry, Cell cell, std::uint64_t now) noexcept {
    const float phase = float((now / 40) % 60) * (6.28318530718f / 60.f);
    return {cell.x + .5f, elevation(geometry.surface(cell).floor) + .5f + .025f * std::sin(phase),
            cell.y + .5f};
}
std::optional<Vec> spiritualWeaponPosition(const FrameInput& in, Cell cell) noexcept {
    auto position = spiritualWeaponPosition(in.geometry, cell, in.now);
    const auto& t = in.movement.transition;
    if (t.active() && !(t.from.cell == t.to.cell)) {
        const auto destination = followingWeaponCell(in.geometry, cell, t.from.cell, t.to.cell);
        // A blocked follower retreats behind the party instead of passing through the camera.
        if (cell == t.to.cell && destination == t.from.cell)
            return {};
        const auto target = spiritualWeaponPosition(in.geometry, destination, in.now);
        const float p = t.progress(), e = p * p * (3 - 2 * p);
        position = {position.x + (target.x - position.x) * e, position.y + (target.y - position.y) * e,
                    position.z + (target.z - position.z) * e};
    } else if (cell == in.movement.player.cell)
        return {};
    return position;
}
EffectsPipeline::EffectsPipeline(const world_assets::CpuAssets& assets) : spells_(assets) {
    constexpr const char* names[]{"effect/arrow",   "effect/tip",    "effect/feathers", "effect/slash",
                                  "effect/glint",   "effect/grease", "effect/web",      "effect/weapon",
                                  "effect/preview", "effect/white"};
    for (unsigned i = 0; i < assets.publication.textures.size(); ++i)
        if (assets.publication.textures[i].name == "effect/hearth-flame")
            flame_ = i;
    for (unsigned i = 0; i < textures_.size(); ++i)
        textures_[i] = assets.named(names[i]);
}
bool EffectsPipeline::update(const FrameInput& in, const WorldFrame& world) {
    if (in.attacks && (!in.actors || in.attacks->world != in.actors->snapshot().world))
        throw std::invalid_argument("Foreign world attack snapshot");
    if (in.spells && (!in.actors || in.spells->world != in.actors->snapshot().world))
        throw std::invalid_argument("Foreign world spell snapshot");
    if (in.conditions && (!in.actors || in.conditions->world != in.actors->snapshot().world))
        throw std::invalid_argument("Foreign world condition snapshot");
    if (in.support && (!in.actors || in.support->world != in.actors->snapshot().world))
        throw std::invalid_argument("Foreign world support snapshot");
    const bool swipeChanged = swipes_.update(in, world);
    const bool spellChanged = spells_.update(in, world);
    const bool changed = shots_.update(in.attacks, in.now) || swipeChanged || spellChanged;
    SpellArea area = in.area ? *in.area : SpellArea{};
    if (world.moving)
        area.eligible.fill(false);
    if (world.moving)
        area.footprint.mask = 0;
    const auto& definition = in.geometry.base().definition();
    std::array<const AmbientFire*, ambientFireCapacity + 1> fires{};
    std::array<bool, ambientFireCapacity + 1> visibleFires{};
    unsigned fireCount = 0;
    if (definition.hearth)
        fires[fireCount++] = &*definition.hearth;
    for (const auto& fire : definition.ambientFires)
        fires[fireCount++] = &fire;
    bool visibleFire = false;
    for (unsigned i = 0; i < fireCount; ++i) {
        const auto& fire = *fires[i];
        const Vec anchor{fire.x, fire.y, fire.z};
        const float d = depth(world.camera, anchor);
        const float side = (anchor.x - world.camera.position.x) * world.camera.right.x +
                           (anchor.z - world.camera.position.z) * world.camera.right.z;
        visibleFires[i] = d > nearPlane - .6f && d < world.camera.farClip + .6f &&
                          std::abs(side) <= std::max(0.f, d) * (viewWidth * .5f / focal()) + .6f;
        visibleFire |= visibleFires[i];
    }
    const auto tick = visibleFire ? in.now / 50 + 1 : 0;
    bool visibleWeapon = false, attackingWeapon = false;
    for (unsigned caster = 0; caster < area.ground.weapons.size(); ++caster) {
        const auto& weapon = area.ground.weapons[caster];
        if (!weapon.active)
            continue;
        const auto position = spiritualWeaponPosition(in, weapon.cell);
        if (!position)
            continue;
        const auto p = *position;
        const auto d = depth(world.camera, p);
        const float side = (p.x - world.camera.position.x) * world.camera.right.x +
                           (p.z - world.camera.position.z) * world.camera.right.z;
        const bool visible = d >= nearPlane && d <= world.camera.farClip &&
                             std::abs(side) <= d * (viewWidth * .5f / focal()) + .325f;
        visibleWeapon |= visible;
        attackingWeapon |= visible && weaponAttack(in, caster);
    }
    const auto weaponTick =
        visibleWeapon ? (in.now / (attackingWeapon ? 20 : 40) + 1) * 2 + attackingWeapon : 0;
    if (fireCount && flame_ == UINT16_MAX)
        throw std::invalid_argument("Hearth flame artwork missing");
    if (fireTick_ == tick && weaponTick_ == weaponTick && !changed && visibility_ == world.revision &&
        area.footprint == area_.footprint && area.cells == area_.cells && area.kind == area_.kind &&
        area.eligible == area_.eligible && area.ground.revision == area_.ground.revision)
        return false;
    fireTick_ = tick;
    weaponTick_ = weaponTick;
    area_ = area;
    visibility_ = world.revision;
    frame_.count = 0;
    frame_.lightCount = 0;
    frame_.frost.fill(0);
    frame_.guiding.fill(0);
    frame_.swipes = swipes_.frame();
    ++frame_.revision;
    auto add = [&](EffectPacket p) {
        if (frame_.count == effectCapacity)
            throw std::logic_error("World effect capacity exceeded");
        frame_.packets[frame_.count++] = p;
    };
    auto ground = [&](Cell tile, unsigned texture, std::array<float, 4> tint, float height) {
        const auto cell = dungeonCell(tile);
        height += elevation(in.geometry.surface(cell).floor);
        const float x = float(tile.x) * .5f, z = float(tile.y) * .5f;
        EffectPacket packet;
        packet.points = {
            {{x, height, z + .5f}, {x + .5f, height, z + .5f}, {x + .5f, height, z}, {x, height, z}}};
        packet.tint = tint;
        packet.depth = depth(world.camera, {x + .25f, 0, z + .25f});
        packet.texture = std::uint16_t(textures_[texture]);
        packet.ground = true;
        add(packet);
    };
    for (const auto& patch : area.ground.areas)
        if (patch.active)
            for (unsigned bit = 0; bit < 64; ++bit)
                if (patch.footprint.mask & (UINT64_C(1) << bit))
                    ground(patch.footprint.tile(bit), patch.web ? 6 : 5, {1, 1, 1, 1}, .0015f);
    const auto color =
        area.kind == rules::SpellKind::Sleep         ? std::array<float, 4>{.545f, .49f, .87f, 1}
        : area.kind == rules::SpellKind::ColorSpray  ? std::array<float, 4>{.855f, .404f, .769f, 1}
        : area.kind == rules::SpellKind::Thunderwave ? std::array<float, 4>{.298f, .651f, .867f, 1}
                                                     : std::array<float, 4>{.949f, .525f, .106f, 1};
    for (unsigned bit = 0; bit < 64; ++bit)
        if (area.footprint.mask & (UINT64_C(1) << bit))
            ground(area.footprint.tile(bit), 8, color, .0025f);
    auto billboard = [&](Vec center, float w, float h, unsigned texture, float angle = 0.f,
                         std::array<float, 4> tint = {1, 1, 1, 1}) {
        const float c = std::cos(angle), s = std::sin(angle);
        EffectPacket p;
        p.tint = tint;
        p.texture = std::uint16_t(textures_[texture]);
        p.depth = depth(world.camera, center);
        if (p.depth < nearPlane || p.depth > world.camera.farClip)
            return;
        constexpr float xs[]{-.5f, .5f, .5f, -.5f}, ys[]{-.5f, -.5f, .5f, .5f};
        for (unsigned i = 0; i < 4; ++i) {
            const float x = xs[i] * w * c - ys[i] * h * s, y = xs[i] * w * s + ys[i] * h * c;
            p.points[i] = {center.x + world.camera.right.x * x, center.y + y,
                           center.z + world.camera.right.z * x};
        }
        add(p);
    };
    for (unsigned fireIndex = 0; fireIndex < fireCount; ++fireIndex) {
        if (!visibleFires[fireIndex])
            continue;
        const auto* fire = fires[fireIndex];
        // Fixed analytic particles need no spawning, RNG state, allocation or save data.
        const float t = float((in.now / 50) % 24000) * .05f + float(fireIndex) * .73f;
        for (unsigned i = 0; i < 24; ++i) {
            const bool ember = i >= 16;
            const float phase = std::fmod(t * (ember ? .48f : .8f) + float(i) * .618034f, 1.f);
            const float lane = float(i % 5) * .065f - .13f;
            Vec center{fire->x + .035f * std::sin(t * 2.3f + float(i)),
                       fire->y + (ember ? .08f + phase * .52f : .07f + phase * .20f),
                       fire->z + lane + .025f * std::sin(t * 3.f + float(i) * 2.f)};
            const float h = ember ? .025f : .22f + .12f * (1.f - phase);
            const unsigned before = frame_.count;
            billboard(center, ember ? .012f : .13f, h, 9, .08f * std::sin(t + float(i)),
                      {1, 1, 1, (ember ? .75f : .8f) * (1.f - phase * phase)});
            if (frame_.count != before)
                frame_.packets[before].texture = std::uint16_t(flame_);
        }
    }
    for (unsigned caster = 0; caster < area.ground.weapons.size(); ++caster) {
        const auto& weapon = area.ground.weapons[caster];
        if (!weapon.active)
            continue;
        const auto position = spiritualWeaponPosition(in, weapon.cell);
        if (!position)
            continue;
        auto p = *position;
        float angle = 0;
        if (const auto* shot = weaponAttack(in, caster)) {
            const float t = in.now < shot->impactMs
                                ? float(in.now - shot->startedMs) /
                                      float(std::max<std::uint64_t>(1, shot->impactMs - shot->startedMs))
                                : 1.f - float(in.now - shot->impactMs) / 240.f;
            const float travel = t * t * (3.f - 2.f * t);
            // The simulation has already moved the weapon into the destination cell.
            // This short local strike uses its recorded subcell, including misses/deaths.
            const Vec target{shot->targetPosition.x,
                             elevation(in.geometry.surface(shot->npcOrigin.cell).floor) + .32f,
                             shot->targetPosition.y};
            const float side =
                (target.x - p.x) * world.camera.right.x + (target.z - p.z) * world.camera.right.z;
            angle = (side < 0 ? .45f : -.45f) * travel;
            p = {p.x + (target.x - p.x) * travel, p.y + (target.y - p.y) * travel,
                 p.z + (target.z - p.z) * travel};
        }
        billboard(p, .65f, .65f, 7, angle);
    }
    std::array<const Billboard*, NpcWorld::maxInstances> bodies{};
    for (unsigned i = 0; i < world.count; ++i) {
        const auto& b = world.billboards[i];
        if (b.id < bodies.size())
            bodies[b.id] = &b;
    }
    for (const auto& shot : shots_) {
        // Spell/support players own all spell artwork. The shared glint remains
        // an ordinary arrow impact, never an alternate spell renderer.
        if (shot.spell != rules::SpellKind::Count)
            continue;
        const Billboard* body =
            in.actors && shot.enemy.world == in.actors->snapshot().world && shot.enemy.slot < bodies.size()
                ? bodies[shot.enemy.slot]
                : nullptr;
        Vec enemy{shot.targetPosition.x, elevation(in.geometry.surface(shot.npcOrigin.cell).floor) + .5f,
                  shot.targetPosition.y};
        if (shot.incoming)
            enemy = {shot.sourcePosition.x, elevation(in.geometry.surface(shot.npcOrigin.cell).floor) + .5f,
                     shot.sourcePosition.y};
        if (body) {
            const auto a = attachment(world.camera, *body, shot.incoming ? body->release : body->torso);
            if (a.valid)
                enemy = a.position;
        }
        const Vec party =
            partyAnchor(shot, shot.incoming ? in.movement.player : shot.partyOrigin, in.geometry);
        const Vec from = shot.incoming ? enemy : party, to = shot.incoming ? party : enemy;
        const bool ending = in.now >= shot.impactMs;
        if (ending && (!shot.hit || shot.targetGone))
            continue;
        const float t = std::clamp(float(in.now - shot.releaseMs) /
                                       float(std::max<std::uint64_t>(1, shot.impactMs - shot.releaseMs)),
                                   0.f, 1.f);
        Vec target = to;
        if (!shot.hit) {
            const float miss = shot.incoming ? .12f : .045f;
            target.x += world.camera.right.x * (shot.id.serial % 2 ? miss : -miss);
            target.z += world.camera.right.z * (shot.id.serial % 2 ? miss : -miss);
            target.y -= shot.incoming ? .08f : .01f;
        }
        Vec point{from.x + (target.x - from.x) * t, from.y + (target.y - from.y) * t + .32f * t * (1 - t),
                  from.z + (target.z - from.z) * t};
        const auto a = project(world.camera, from), b = project(world.camera, target);
        unsigned texture = 0;
        float size = .13f;
        if (ending) {
            point = to;
            texture = shot.ranged ? 4 : 3;
            size = (shot.ranged ? 12.f : 28.f) * depth(world.camera, to) / focal();
        } else if (std::abs(b.x - a.x) < 25 && std::abs(b.y - a.y) < 35)
            texture = shot.incoming ? 1 : 2;
        const float fade = ending ? 1.f - float(in.now - shot.impactMs) / 120.f : 1.f;
        const auto tint = texture == 4 ? std::array<float, 4>{1, 231.f / 255, 155.f / 255, fade}
                                       : std::array<float, 4>{1, 1, 1, fade};
        if (ending) {
            // The body owns depth at this exact attachment plane. Pull only the
            // cosmetic impact forward slightly, retaining ordinary wall/body tests.
            point.x -= world.camera.forward.x * .002f;
            point.z -= world.camera.forward.z * .002f;
        }
        const unsigned copies = std::clamp<unsigned>(shot.darts, 1, 3);
        for (unsigned dart = 0; dart < copies; ++dart) {
            auto center = point;
            float offset = (int(dart) * 2 - int(copies) + 1) * 5.f;
            offset *= depth(world.camera, point) / focal();
            center.x += world.camera.right.x * offset;
            center.z += world.camera.right.z * offset;
            billboard(center, size, size, texture, std::atan2(a.y - b.y, b.x - a.x), tint);
        }
    }
    spells_.append(in, world, frame_);
    // Stable bounded insertion sort: ground layers retain submission order, airborne packets far-first.
    for (unsigned i = 1; i < frame_.count; ++i) {
        const auto v = frame_.packets[i];
        unsigned j = i;
        while (j && !v.ground && !frame_.packets[j - 1].ground && v.depth > frame_.packets[j - 1].depth) {
            frame_.packets[j] = frame_.packets[j - 1];
            --j;
        }
        frame_.packets[j] = v;
    }
    return true;
}
} // namespace blobber::world_render

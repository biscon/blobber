#include "render/world_effects.hpp"
#include <cmath>
#include <stdexcept>
namespace blobber::world_render {
namespace {
float depth(const Camera& c, Vec p) {
    return (p.x - c.position.x) * c.forward.x + (p.z - c.position.z) * c.forward.z;
}
Vec partyAnchor(const AttackVisual& shot, Player player, GeometryView geometry) {
    const auto f = forward(player.heading), r = right(player.heading);
    const float lane = (shot.formationSlot % 2 ? 1.f : -1.f) * .15f;
    return {player.cell.x + .5f + f.x * .1f + r.x * lane,
            elevation(geometry.surface(player.cell).floor) + (shot.formationSlot < 2 ? .2f : .17f),
            player.cell.y + .5f + f.y * .1f + r.y * lane};
}
std::array<float, 4> spellTint(rules::SpellKind kind, float alpha) {
    using rules::SpellKind;
    switch (kind) {
    case SpellKind::FireBolt:
    case SpellKind::BurningHands:
    case SpellKind::ScorchingRay:
        return {1, 114.f / 255, 38.f / 255, alpha};
    case SpellKind::PoisonSpray:
    case SpellKind::AcidSplash:
        return {118.f / 255, 242.f / 255, 60.f / 255, alpha};
    case SpellKind::Sleep:
    case SpellKind::ColorSpray:
    case SpellKind::InflictWounds:
    case SpellKind::Bane:
        return {179.f / 255, 75.f / 255, 215.f / 255, alpha};
    case SpellKind::RayOfFrost:
    case SpellKind::ShockingGrasp:
    case SpellKind::Thunderwave:
    case SpellKind::MagicMissile:
        return {110.f / 255, 174.f / 255, 1, alpha};
    case SpellKind::Count:
        return {1, 231.f / 255, 155.f / 255, alpha};
    default:
        return {1, 232.f / 255, 150.f / 255, alpha};
    }
}
} // namespace
EffectsPipeline::EffectsPipeline(const world_assets::CpuAssets& assets) {
    constexpr const char* names[]{"effect/arrow",   "effect/tip",    "effect/feathers", "effect/slash",
                                  "effect/glint",   "effect/grease", "effect/web",      "effect/weapon",
                                  "effect/preview", "effect/white"};
    for (unsigned i = 0; i < textures_.size(); ++i)
        textures_[i] = assets.named(names[i]);
}
bool EffectsPipeline::update(const FrameInput& in, const WorldFrame& world) {
    if (in.attacks && (!in.actors || in.attacks->world != in.actors->snapshot().world))
        throw std::invalid_argument("Foreign world attack snapshot");
    const bool changed = shots_.update(in.attacks, in.now);
    SpellArea area = in.area ? *in.area : SpellArea{};
    if (world.moving)
        area.eligible.fill(false);
    if (!changed && visibility_ == world.revision && area.cells == area_.cells && area.kind == area_.kind &&
        area.eligible == area_.eligible && area.ground.revision == area_.ground.revision)
        return false;
    area_ = area;
    visibility_ = world.revision;
    frame_.count = 0;
    ++frame_.revision;
    auto add = [&](EffectPacket p) {
        if (frame_.count == effectCapacity)
            throw std::logic_error("World effect capacity exceeded");
        frame_.packets[frame_.count++] = p;
    };
    auto ground = [&](Cell cell, unsigned texture, std::array<float, 4> tint, float height) {
        height += elevation(in.geometry.surface(cell).floor);
        const float x = float(cell.x), z = float(cell.y);
        add({{{{x, height, z + 1}, {x + 1, height, z + 1}, {x + 1, height, z}, {x, height, z}}},
             tint,
             depth(world.camera, {x + .5f, 0, z + .5f}),
             std::uint16_t(textures_[texture]),
             true});
    };
    for (const auto& patch : area.ground.areas)
        if (patch.active) {
            ground(patch.cell, patch.web ? 6 : 5, {1, 1, 1, 1}, .0015f);
            if (patch.web && patch.burnMs)
                ground(patch.cell, 9, {1, .412f, .118f, .255f}, .002f);
        }
    const auto color =
        area.kind == rules::SpellKind::Sleep         ? std::array<float, 4>{.545f, .49f, .87f, 1}
        : area.kind == rules::SpellKind::ColorSpray  ? std::array<float, 4>{.855f, .404f, .769f, 1}
        : area.kind == rules::SpellKind::Thunderwave ? std::array<float, 4>{.298f, .651f, .867f, 1}
                                                     : std::array<float, 4>{.949f, .525f, .106f, 1};
    for (unsigned i = 0; i < area.cells.size(); ++i)
        if (area.eligible[i])
            ground(area.cells[i], 8, color, .0025f);
    auto billboard = [&](Vec center, float w, float h, unsigned texture, float angle = 0.f,
                         std::array<float, 4> tint = {1, 1, 1, 1}) {
        const float c = std::cos(angle), s = std::sin(angle);
        EffectPacket p;
        p.tint = tint;
        p.texture = std::uint16_t(textures_[texture]);
        p.depth = depth(world.camera, center);
        if (p.depth < nearPlane || p.depth > farPlane)
            return;
        constexpr float xs[]{-.5f, .5f, .5f, -.5f}, ys[]{-.5f, -.5f, .5f, .5f};
        for (unsigned i = 0; i < 4; ++i) {
            const float x = xs[i] * w * c - ys[i] * h * s, y = xs[i] * w * s + ys[i] * h * c;
            p.points[i] = {center.x + world.camera.right.x * x, center.y + y,
                           center.z + world.camera.right.z * x};
        }
        add(p);
    };
    for (const auto& weapon : area.ground.weapons)
        if (weapon.active)
            billboard({weapon.cell.x + .5f, elevation(in.geometry.surface(weapon.cell).floor) + .5f,
                       weapon.cell.y + .5f},
                      .65f, .65f, 7);
    std::array<const Billboard*, NpcWorld::maxInstances> bodies{};
    for (unsigned i = 0; i < world.count; ++i) {
        const auto& b = world.billboards[i];
        if (b.id < bodies.size())
            bodies[b.id] = &b;
    }
    for (const auto& shot : shots_) {
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
        if (shot.spell != rules::SpellKind::Count) {
            texture = 4;
            size = ending ? 28.f * depth(world.camera, to) / focal()
                          : std::max(.182f, 7.f * depth(world.camera, point) / focal());
        }
        const float fade = ending ? 1.f - float(in.now - shot.impactMs) / 120.f : 1.f;
        const auto tint = texture == 4 ? spellTint(shot.spell, fade) : std::array<float, 4>{1, 1, 1, fade};
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
            if (shot.spell == rules::SpellKind::ScorchingRay)
                offset += (int(shot.id.serial % 3) - 1) * 6.f;
            offset *= depth(world.camera, point) / focal();
            center.x += world.camera.right.x * offset;
            center.z += world.camera.right.z * offset;
            billboard(center, size, size, texture, std::atan2(a.y - b.y, b.x - a.x), tint);
        }
    }
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

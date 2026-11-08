#include "render/spell_effects.hpp"
#include "render/world_effects.hpp"
#include <cmath>
namespace blobber::world_render {
namespace {
using Kind = rules::SpellKind;
using Clip = world_assets::EffectClipKind;
bool offensive(Kind k) {
    return elementalSpell(k) && k != Kind::FireBolt && k != Kind::RayOfFrost && k != Kind::BurningHands;
}
struct Style {
    Clip flight, impact;
    std::array<float, 3> color;
    float light;
};
Style style(Kind k) {
    switch (k) {
    case Kind::MagicMissile:
        return {Clip::ForceFlight, Clip::ForceImpact, {.60f, .30f, 1}, .35f};
    case Kind::ScorchingRay:
        return {Clip::RayFlight, Clip::RayImpact, {1, .30f, .05f}, .65f};
    case Kind::GuidingBolt:
        return {Clip::RadiantFlight, Clip::RadiantImpact, {1, .75f, .25f}, .60f};
    case Kind::SacredFlame:
        return {Clip::SacredFlame, Clip::SacredFlame, {1, .75f, .25f}, .45f};
    case Kind::ShockingGrasp:
        return {Clip::Electricity, Clip::Electricity, {.3f, .6f, 1}, .45f};
    case Kind::InflictWounds:
        return {Clip::Necrotic, Clip::Necrotic, {.5f, .1f, .7f}, 0};
    case Kind::AcidSplash:
        return {Clip::Acid, Clip::Acid, {.6f, .7f, .1f}, 0};
    case Kind::PoisonSpray:
        return {Clip::Poison, Clip::Poison, {.3f, .5f, .1f}, 0};
    case Kind::Thunderwave:
        return {Clip::Thunderwave, Clip::Thunderwave, {.6f, .7f, .8f}, 0};
    default:
        return {Clip::Shatter, Clip::Shatter, {.6f, .7f, .8f}, 0};
    }
}
// Short-lived stack adapter over fixed frame storage. No resource ownership or allocation.
struct Painter {
    const world_assets::EffectClips& clips;
    const WorldFrame& world;
    EffectFrame& frame;
    SpellEffectDiagnostics& diagnostics;
    void emit(Vec center, float width, float height, Clip kind, unsigned age, float alpha, bool glow = false,
              float angle = 0, const SpellCastVisual* area = nullptr) {
        const auto& camera = world.camera;
        const float z = (center.x - camera.position.x) * camera.forward.x +
                        (center.z - camera.position.z) * camera.forward.z;
        if (z < nearPlane || z > camera.farClip || alpha <= 0)
            return;
        if (frame.count == frame.packets.size()) {
            ++diagnostics.packets;
            return;
        }
        const auto& clip = clips[unsigned(kind)];
        if (!clip.count || !clip.frameMs)
            return;
        EffectPacket p;
        p.texture = glow ? clip.emission : clip.body;
        p.depth = z;
        p.additive = glow;
        p.tint = {1, 1, 1, alpha};
        if (kind == Clip::Poison)
            p.tint = {.28f, 1, .22f, alpha};
        p.uv = clip.frames[std::min(clip.count - 1, age / clip.frameMs)];
        const float c = std::cos(angle), s = std::sin(angle);
        constexpr float xs[]{-.5f, .5f, .5f, -.5f}, ys[]{-.5f, -.5f, .5f, .5f};
        for (unsigned i = 0; i < 4; ++i) {
            const float x = xs[i] * width * c - ys[i] * height * s,
                        y = xs[i] * width * s + ys[i] * height * c;
            p.points[i] = {center.x + camera.right.x * x, center.y + y, center.z + camera.right.z * x};
        }
        if (area)
            p.subcells = area->footprint;
        frame.packets[frame.count++] = p;
    }
    void light(Vec position, Style st, float alpha, unsigned priority) {
        if (st.light <= 0 || alpha <= 0)
            return;
        LocalLight light{position, st.color, 1.5f, st.light * alpha, priority};
        if (frame.lightCount < frame.lights.size())
            frame.lights[frame.lightCount++] = light;
        else {
            unsigned worst = 0;
            for (unsigned i = 1; i < frame.lightCount; ++i)
                if (frame.lights[i].priority < frame.lights[worst].priority)
                    worst = i;
            if (priority > frame.lights[worst].priority)
                frame.lights[worst] = light;
            ++diagnostics.lights;
        }
    }
};
} // namespace
void SpellEffectsPlayer::appendOffensive(const FrameInput& in, const WorldFrame& world, EffectFrame& frame,
                                         unsigned pass, unsigned& particles) noexcept {
    Painter paint{clips_, world, frame, diagnostics_};
    std::array<const Billboard*, NpcWorld::maxInstances> bodies{};
    for (unsigned i = 0; i < world.count; ++i)
        if (world.billboards[i].id < bodies.size())
            bodies[world.billboards[i].id] = &world.billboards[i];
    frame.guidingTexture = clips_[unsigned(Clip::GuidingCoating)].body;
    frame.guidingUv = clips_[unsigned(Clip::GuidingCoating)].frames[2];
    for (unsigned i = 0; i < bodies.size(); ++i)
        if (!pass && guiding_[i] && bodies[i] && bodies[i]->alive)
            frame.guiding[i] = .22f;
    auto bodyFor = [&](const SpellTargetVisual& target) -> const Billboard* {
        return target.actor.world == world_ && target.actor.slot < bodies.size() ? bodies[target.actor.slot]
                                                                                 : nullptr;
    };
    auto point = [&](const SpellTargetVisual& target, bool follow) {
        if (follow)
            if (const auto* b = bodyFor(target)) {
                const auto a = attachment(world.camera, *b, b->torso);
                if (a.valid)
                    return a.position;
            }
        return Vec{target.position.x, target.floor + .45f, target.position.y};
    };
    // All primary world bodies and contacts precede optional trails.
    {
        for (const auto& record : casts_) {
            const auto& c = record.cue;
            if (!c.id.serial || !offensive(c.spell))
                continue;
            const auto st = style(c.spell);
            const auto f = forward(c.origin.heading), r = right(c.origin.heading);
            const float floor = elevation(in.geometry.surface(c.origin.cell).floor);
            const Vec from{c.origin.cell.x + .5f + f.x * .12f + r.x * (c.formation % 2 ? .12f : -.12f),
                           floor + .18f,
                           c.origin.cell.y + .5f + f.y * .12f + r.y * (c.formation % 2 ? .12f : -.12f)};
            if (c.spell == Kind::Thunderwave || c.spell == Kind::Shatter) {
                if (pass || in.now < c.impactMs || in.now - c.impactMs >= 640)
                    continue;
                const unsigned age = unsigned(in.now - c.impactMs);
                const float t = std::min(1.f, float(age) / 240), fade = std::min(1.f, float(640 - age) / 240);
                if (c.spell == Kind::Thunderwave) {
                    const Vec source{c.origin.cell.x + .5f + f.x * .5f, floor + .18f,
                                     c.origin.cell.y + .5f + f.y * .5f};
                    for (unsigned layer = 0; layer < 3; ++layer) {
                        const float reach = .125f + t * (.125f + .25f * layer);
                        paint.emit({source.x + f.x * reach, floor + .27f, source.z + f.y * reach},
                                   .4f + 2.7f * t, .55f, st.impact, age, fade * .5f, false, 0, &c);
                    }
                } else {
                    // One cast-centred burst with layered depth; fragments are clipped to the exact
                    // footprint.
                    const float base = elevation(in.geometry.surface(c.center).floor);
                    for (int layer = -1; layer <= 1; ++layer)
                        paint.emit({c.footprint.origin.x * .5f + 1.f + f.x * layer * .45f, base + .40f,
                                    c.footprint.origin.y * .5f + 1.f + f.y * layer * .45f},
                                   .4f + 2.6f * t, .35f + .65f * t, st.impact, age, fade * .65f, false, 0,
                                   &c);
                }
                continue;
            }
            for (unsigned i = 0; i < c.deliveryCount; ++i) {
                const auto& d = c.deliveries[i];
                if (d.stage == AttackStage::Canceled || in.now < d.releaseMs || in.now >= d.impactMs ||
                    c.spell == Kind::InflictWounds || c.spell == Kind::ShockingGrasp)
                    continue;
                auto to = point(c.targets[i], true);
                if (!c.targets[i].hit) {
                    to.x += r.x * .12f;
                    to.z += r.y * .12f;
                    to.y -= .03f;
                }
                const float t =
                    float(in.now - d.releaseMs) / float(std::max<std::uint64_t>(1, d.impactMs - d.releaseMs));
                const unsigned copies = c.spell == Kind::MagicMissile ? std::clamp(d.darts, 1u, 3u) : 1;
                for (unsigned dart = 0; dart < copies; ++dart) {
                    const float lane = float((c.spell == Kind::MagicMissile ? dart : i)) - 1.f;
                    auto sample = [&](float u) {
                        const float bow = std::sin(u * 3.14159265f);
                        const float curve = (c.spell == Kind::MagicMissile ? .30f : .055f) * lane * bow;
                        return Vec{from.x + (to.x - from.x) * u + r.x * curve,
                                   from.y + (to.y - from.y) * u +
                                       (c.spell == Kind::MagicMissile ? .12f + .08f * float(dart) : .025f) *
                                           bow,
                                   from.z + (to.z - from.z) * u + r.y * curve};
                    };
                    const auto p = sample(t), previous = sample(std::max(0.f, t - .03f));
                    const auto a = project(world.camera, previous), b = project(world.camera, p);
                    const float angle = std::atan2(a.y - b.y, b.x - a.x);
                    const auto age = unsigned((in.now - d.releaseMs) % 640);
                    const float width = c.spell == Kind::ScorchingRay ? .46f : .31f;
                    if (!pass) {
                        paint.emit(p, width, c.spell == Kind::ScorchingRay ? .085f : .14f, st.flight, age, 1,
                                   false, angle);
                        paint.emit(p, width, .12f, st.flight, age, .3f, true, angle);
                        paint.light(p, st, 1, 3);
                    } else
                        for (unsigned n = 1; n <= 6; ++n) {
                            if (t < float(n) * .028f)
                                break;
                            if (particles == spellParticleCapacity) {
                                ++diagnostics_.particles;
                                break;
                            }
                            ++particles;
                            paint.emit(sample(t - float(n) * .028f), width * (1 - float(n) / 8), .035f,
                                       st.flight, age, .35f * (1 - float(n) / 7), true, angle);
                        }
                }
            }
        }
        for (const auto& rem : remnants_) {
            if (!rem.id.serial || !offensive(rem.spell) || in.now < rem.impactMs || !bodyFor(rem.target))
                continue;
            const unsigned age = unsigned(in.now - rem.impactMs);
            const bool damage = rem.target.hit && rem.target.damage > 0;
            const unsigned end = damage ? 640u : 240u;
            if (age >= end)
                continue;
            const auto st = style(rem.spell);
            auto p = point(rem.target, damage);
            if (!rem.target.hit) {
                p.x += world.camera.right.x * .13f;
                p.z += world.camera.right.z * .13f;
            }
            p.x -= world.camera.forward.x * .003f;
            p.z -= world.camera.forward.z * .003f;
            const float remaining = 1 - float(age) / end;
            // Poison is a dense cloud even on a save, but negated contact still ends sooner.
            const float fade = rem.spell == Kind::PoisonSpray
                                   ? std::min(1.f, remaining * 2) * (damage ? 1.f : .8f)
                                   : remaining * (rem.target.saved ? .55f : 1.f) * (damage ? 1.f : .35f);
            if (pass) {
                if (rem.spell == Kind::AcidSplash && damage && age < 480 &&
                    particles < spellParticleCapacity) {
                    ++particles;
                    p.y -= float(age) * .0004f;
                    paint.emit(p, .32f, .35f, Clip::AcidFlecks, age, fade);
                }
                continue;
            }
            const bool area = rem.spell == Kind::Thunderwave || rem.spell == Kind::Shatter;
            if (area)
                continue; // The single cast release already owns area contact artwork.
            const float size = rem.spell == Kind::PoisonSpray   ? .65f
                               : rem.spell == Kind::SacredFlame ? .52f
                                                                : .45f;
            if (rem.spell == Kind::SacredFlame)
                p.y += .18f * (1 - std::min(1.f, float(age) / 240));
            if (rem.spell == Kind::InflictWounds) {
                const float contraction = 1 - .6f * float(age) / end;
                paint.emit(p, size * contraction, size * contraction, st.impact, age, fade);
            } else {
                paint.emit(p, size, rem.spell == Kind::SacredFlame ? size * 1.5f : size, st.impact, age,
                           fade);
                if (st.light > 0)
                    paint.emit(p, size, size, st.impact, age, fade * .24f, true);
            }
            paint.light(p, st, fade, 2);
            if (rem.spell == Kind::ScorchingRay && damage && age >= 160) {
                paint.emit(p, .28f, .48f, Clip::FireRemnant, age, fade * .40f);
            }
        }
    }
}
} // namespace blobber::world_render

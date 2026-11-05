#include "render/spell_effects.hpp"
#include "render/world_effects.hpp"
#include <cmath>
namespace blobber::world_render {
namespace {
using Kind = rules::SpellKind;
using Clip = world_assets::EffectClipKind;
float distance(const Camera& camera, Vec p) noexcept {
    return (p.x - camera.position.x) * camera.forward.x + (p.z - camera.position.z) * camera.forward.z;
}
float noise(std::uint64_t serial, unsigned i) noexcept {
    auto x = serial ^ (std::uint64_t(i + 1) * 0x9e3779b97f4a7c15ull);
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ull;
    x ^= x >> 27;
    return float((x >> 16) & 65535) / 65535.f;
}
unsigned duration(Kind spell) noexcept {
    return spell == Kind::FireBolt || spell == Kind::BurningHands ? 800u : 640u;
}
bool fresh(std::uint64_t now, std::uint64_t start, unsigned ms) noexcept {
    return now >= start && now - start < ms;
}
} // namespace
bool SpellEffectsPlayer::update(const FrameInput& in, const WorldFrame&) noexcept {
    const auto world = in.actors ? in.actors->snapshot().world : 0;
    const auto epoch = in.spells ? in.spells->epoch : 0;
    bool changed = world != world_ || epoch != epoch_;
    if (changed) {
        casts_ = {};
        remnants_ = {};
        slow_.reset();
        guiding_.reset();
        seen_ = 0;
        revision_ = UINT64_MAX;
        world_ = world;
        epoch_ = epoch;
        active_ = false;
    }
    if (in.spells && in.spells->world == world_ && in.spells->revision != revision_) {
        std::uint64_t seen = seen_;
        for (const auto& cue : in.spells->casts) {
            if (!cue.id.serial || cue.id.world != world_)
                continue;
            seen = std::max(seen, cue.id.serial);
            Cast* slot = nullptr;
            for (auto& c : casts_)
                if (c.cue.id.serial == cue.id.serial) {
                    slot = &c;
                    break;
                }
            if (!slot && cue.id.serial > seen_ &&
                (cue.stage == AttackStage::Flight || cue.stage == AttackStage::Windup ||
                 fresh(in.now, cue.impactMs, 1000))) {
                for (auto& c : casts_)
                    if (!c.cue.id.serial) {
                        slot = &c;
                        break;
                    }
                if (!slot) {
                    slot = &casts_[0];
                    for (auto& c : casts_) {
                        const bool completed = c.cue.stage == AttackStage::Resolved;
                        const bool previousCompleted = slot->cue.stage == AttackStage::Resolved;
                        if ((completed && !previousCompleted) ||
                            (completed == previousCompleted && c.cue.id.serial < slot->cue.id.serial))
                            slot = &c;
                    }
                    ++diagnostics_.casts;
                }
                *slot = {};
            }
            if (!slot)
                continue;
            slot->cue = cue;
            for (unsigned i = 0; i < std::min<unsigned>(cue.count, cue.targets.size()); ++i) {
                const auto stage = cue.deliveryCount ? cue.deliveries[i].stage : cue.stage;
                if ((slot->resolved & (1ull << i)) || stage != AttackStage::Resolved)
                    continue;
                slot->resolved |= 1ull << i;
                const auto when = cue.deliveryCount ? cue.deliveries[i].impactMs : cue.impactMs;
                const auto& t = cue.targets[i];
                if (t.gone || !fresh(in.now, when, duration(cue.spell)))
                    continue;
                // SE01 misses never attach; SE02 can show a restrained dissipating contact.
                if (!t.hit && (cue.spell == Kind::FireBolt || cue.spell == Kind::RayOfFrost ||
                               cue.spell == Kind::BurningHands))
                    continue;
                Remnant* r = nullptr;
                for (auto& candidate : remnants_)
                    if (!candidate.id.serial ||
                        !fresh(in.now, candidate.impactMs, duration(candidate.spell))) {
                        r = &candidate;
                        break;
                    }
                if (!r) {
                    r = &remnants_[0];
                    for (auto& candidate : remnants_)
                        if (candidate.impactMs < r->impactMs ||
                            (candidate.impactMs == r->impactMs && candidate.id.serial < r->id.serial))
                            r = &candidate;
                    ++diagnostics_.remnants;
                }
                *r = {cue.id, t, cue.spell, when};
            }
        }
        seen_ = seen;
        revision_ = in.spells->revision;
        changed = true;
    }
    const auto slow = in.conditions && in.conditions->world == world_ ? in.conditions->slow
                                                                      : std::bitset<NpcWorld::maxInstances>{};
    changed |= slow != slow_;
    slow_ = slow;
    const auto guiding = in.conditions && in.conditions->world == world_
                             ? in.conditions->guiding
                             : std::bitset<NpcWorld::maxInstances>{};
    changed |= guiding != guiding_;
    guiding_ = guiding;
    bool active = false;
    for (auto& c : casts_) {
        if (!c.cue.id.serial)
            continue;
        if (c.cue.stage == AttackStage::Canceled ||
            (in.now >= c.cue.impactMs &&
             (in.now - c.cue.impactMs >= 1000 ||
              (c.cue.stage == AttackStage::Resolved &&
               in.now - c.cue.impactMs >=
                   (c.cue.spell == Kind::BurningHands                                    ? 480u
                    : (c.cue.spell == Kind::Thunderwave || c.cue.spell == Kind::Shatter) ? 640u
                                                                                         : 0u))))) {
            c = {};
            changed = true;
        } else
            active = true;
    }
    for (auto& r : remnants_) {
        if (!r.id.serial)
            continue;
        const auto* a = in.actors ? in.actors->actor(r.target.actor) : nullptr;
        if (!a || a->life == ActorLife::Removed ||
            !fresh(in.now, r.impactMs, r.target.damage > 0 ? duration(r.spell) : 240u)) {
            r = {};
            changed = true;
        } else
            active = true;
    }
    changed |= active_ != active || (active && now_ != in.now);
    active_ = active;
    now_ = in.now;
    return changed;
}
void SpellEffectsPlayer::append(const FrameInput& in, const WorldFrame& world, EffectFrame& frame) noexcept {
    if (clips_[0].body == UINT16_MAX)
        return;
    std::array<const Billboard*, NpcWorld::maxInstances> bodies{};
    for (unsigned i = 0; i < world.count; ++i)
        if (world.billboards[i].id < bodies.size())
            bodies[world.billboards[i].id] = &world.billboards[i];
    frame.frostTexture = clips_[unsigned(Clip::FrostCoating)].body;
    frame.frostUv = clips_[unsigned(Clip::FrostCoating)].frames[2];
    for (unsigned i = 0; i < bodies.size(); ++i)
        if (slow_[i] && bodies[i] && bodies[i]->alive)
            frame.frost[i] = .28f;
    auto point = [&](const SpellTargetVisual& target, bool& visible) {
        const Billboard* b = target.actor.world == world_ && target.actor.slot < bodies.size()
                                 ? bodies[target.actor.slot]
                                 : nullptr;
        visible = b != nullptr;
        if (b) {
            const auto a = attachment(world.camera, *b, b->torso);
            if (a.valid)
                return a.position;
        }
        return Vec{target.position.x, target.floor + .45f, target.position.y};
    };
    auto animationFrame = [&](Clip kind, std::uint64_t age) {
        return unsigned(age / clips_[unsigned(kind)].frameMs);
    };
    auto emit = [&](Vec center, float width, float height, Clip kind, unsigned animation, float opacity,
                    bool additive, float angle = 0.f, const SpellCastVisual* cone = nullptr) {
        const float z = distance(world.camera, center);
        if (z < nearPlane || z > world.camera.farClip || opacity <= 0)
            return;
        if (frame.count == frame.packets.size()) {
            ++diagnostics_.packets;
            return;
        }
        const auto& clip = clips_[unsigned(kind)];
        EffectPacket p;
        p.texture = additive ? clip.emission : clip.body;
        p.additive = additive;
        p.depth = z;
        p.uv = clip.frames[animation % clip.count];
        p.tint = {1, 1, 1, opacity};
        const float c = std::cos(angle), s = std::sin(angle);
        constexpr float xs[]{-.5f, .5f, .5f, -.5f}, ys[]{-.5f, -.5f, .5f, .5f};
        for (unsigned i = 0; i < 4; ++i) {
            const float x = (xs[i] + .5f - clip.anchor[0]) * width * c -
                            (ys[i] - .5f + clip.anchor[1]) * height * s,
                        y = (xs[i] + .5f - clip.anchor[0]) * width * s +
                            (ys[i] - .5f + clip.anchor[1]) * height * c;
            p.points[i] = {center.x + world.camera.right.x * x, center.y + y,
                           center.z + world.camera.right.z * x};
        }
        if (cone) {
            p.cells = cone->cells;
            p.origin = cone->origin.cell;
            p.footprint = 512 | (cone->includeOrigin ? 1024 : 0);
            for (unsigned i = 0; i < 9; ++i)
                if (cone->eligible[i])
                    p.footprint |= 1u << i;
        }
        frame.packets[frame.count++] = p;
    };
    auto light = [&](Vec p, bool frost, float strength, unsigned priority) {
        if (strength <= 0)
            return;
        LocalLight l{p,
                     frost ? std::array<float, 3>{.23f, .60f, 1.f} : std::array<float, 3>{1.f, .29f, .055f},
                     1.5f, strength, priority};
        if (frame.lightCount < frame.lights.size())
            frame.lights[frame.lightCount++] = l;
        else {
            unsigned worst = 0;
            for (unsigned i = 1; i < frame.lightCount; ++i)
                if (frame.lights[i].priority < frame.lights[worst].priority)
                    worst = i;
            if (l.priority > frame.lights[worst].priority)
                frame.lights[worst] = l;
            ++diagnostics_.lights;
        }
    };
    // Primary bodies first. Particles are a separate bounded second pass.
    unsigned particles = 0;
    for (unsigned pass = 0; pass < 2; ++pass) {
        auto particle = [&](Vec p, bool frost, std::uint64_t serial, unsigned index, float alpha,
                            float size) {
            if (particles == spellParticleCapacity) {
                ++diagnostics_.particles;
                return;
            }
            ++particles;
            emit(p, size, size, Clip::Particles, (frost ? 4 : 0) + unsigned(noise(serial, index) * 3.99f),
                 alpha, true);
        };
        for (const auto& record : casts_) {
            const auto& c = record.cue;
            if (!c.id.serial || in.now < c.releaseMs || c.stage == AttackStage::Canceled)
                continue;
            if (c.spell != Kind::FireBolt && c.spell != Kind::RayOfFrost && c.spell != Kind::BurningHands)
                continue;
            const bool frost = c.spell == Kind::RayOfFrost;
            const auto f = forward(c.origin.heading), r = right(c.origin.heading);
            const float floor = elevation(in.geometry.surface(c.origin.cell).floor);
            Vec from{c.origin.cell.x + .5f + f.x * .12f + r.x * (c.formation % 2 ? .12f : -.12f),
                     floor + .18f,
                     c.origin.cell.y + .5f + f.y * .12f + r.y * (c.formation % 2 ? .12f : -.12f)};
            if (c.spell == Kind::BurningHands) {
                if (!fresh(in.now, c.impactMs, 480))
                    continue;
                const float age = float(in.now - c.impactMs), spread = std::min(1.f, age / 180.f);
                const float alpha = std::min(1.f, (480.f - age) / 160.f);
                if (std::any_of(c.eligible.begin(), c.eligible.end(), [](bool eligible) { return eligible; }))
                    for (unsigned layer = 0; layer < 3; ++layer) {
                        // Each plane spans the fan; the shader clips it to the authoritative footprint.
                        // Depth overlap gives one release even when only one front cell has a victim.
                        const float reach = .12f + spread * (.35f + .27f * float(layer));
                        const float height = .16f + .50f * reach, width = .24f + 2.65f * reach;
                        Vec p{c.origin.cell.x + .5f + f.x * (.12f + reach), floor + .02f + height * .5f,
                              c.origin.cell.y + .5f + f.y * (.12f + reach)};
                        if (!pass) {
                            const unsigned animation =
                                std::min(7u, animationFrame(Clip::Cone, unsigned(age)) + layer % 2);
                            emit(p, width, height, Clip::Cone, animation, alpha * .72f, false, 0, &c);
                            emit(p, width, height, Clip::Cone, animation, alpha * .16f, true, 0, &c);
                        }
                        for (unsigned cell = 0; cell < 3; ++cell)
                            if (c.eligible[cell]) {
                                Vec q{c.origin.cell.x + .5f + (c.cells[cell].x - c.origin.cell.x) * reach,
                                      p.y,
                                      c.origin.cell.y + .5f + (c.cells[cell].y - c.origin.cell.y) * reach};
                                if (!pass && layer == 1)
                                    light(q, false, .55f * alpha, 2);
                                else if (pass)
                                    particle({q.x, q.y + .18f, q.z}, false, c.id.serial, cell * 3 + layer,
                                             alpha * .6f, .035f);
                            }
                    }
                continue;
            }
            if (!c.count || in.now >= c.impactMs)
                continue;
            bool visible = false;
            auto to = point(c.targets[0], visible);
            if (!c.targets[0].hit) {
                to.x += r.x * .09f;
                to.z += r.y * .09f;
                to.y -= .025f;
            }
            const float t = std::clamp(float(in.now - c.releaseMs) /
                                           float(std::max<std::uint64_t>(1, c.impactMs - c.releaseMs)),
                                       0.f, 1.f);
            const auto sample = [&](float u) {
                return Vec{from.x + (to.x - from.x) * u, from.y + (to.y - from.y) * u + .12f * u * (1 - u),
                           from.z + (to.z - from.z) * u};
            };
            const auto p = sample(t);
            const auto a = project(world.camera, from), b = project(world.camera, to);
            const float angle = std::atan2(a.y - b.y, b.x - a.x);
            const float size = std::max(frost ? .24f : .22f, 7.f * distance(world.camera, p) / focal());
            const auto clip = frost ? Clip::FrostFlight : Clip::FireFlight;
            const unsigned animation = animationFrame(clip, in.now - c.releaseMs);
            if (!pass) {
                emit(p, size * (frost ? 1.5f : 1), size * (frost ? .42f : .70f), clip, animation, 1, false,
                     angle);
                emit(p, size * (frost ? 1.5f : 1), size * (frost ? .42f : .70f), clip, animation, .32f, true,
                     angle);
                light(p, frost, frost ? .35f : .85f, 3);
            } else
                for (unsigned i = 0; i < 8; ++i) {
                    const float lag = (float(i) + 1) * .022f;
                    if (t < lag)
                        break;
                    auto q = sample(t - lag);
                    const float n = noise(c.id.serial, i) - .5f;
                    q.y += n * .05f;
                    q.x += r.x * n * .04f;
                    q.z += r.y * n * .04f;
                    particle(q, frost, c.id.serial, i, (1 - float(i) / 8.f) * .65f, frost ? .025f : .035f);
                }
        }
        for (const auto& rem : remnants_) {
            if (!rem.id.serial || !fresh(in.now, rem.impactMs, duration(rem.spell)))
                continue;
            if (rem.spell != Kind::FireBolt && rem.spell != Kind::RayOfFrost &&
                rem.spell != Kind::BurningHands)
                continue;
            bool visible = false;
            auto p = point(rem.target, visible);
            if (!visible)
                continue;
            const auto* body = bodies[rem.target.actor.slot];
            const bool frost = rem.spell == Kind::RayOfFrost;
            const unsigned age = unsigned(in.now - rem.impactMs);
            const float fade = 1.f - float(age) / float(duration(rem.spell));
            const float strength = rem.target.saved ? .55f : 1.f;
            p.x -= world.camera.forward.x * .003f;
            p.z -= world.camera.forward.z * .003f;
            if (!pass) {
                if (age < 240) {
                    const auto clip = frost ? Clip::FrostImpact : Clip::FireImpact;
                    const unsigned animation = std::min(7u, animationFrame(clip, age) * 3);
                    emit(p, .42f, .42f, clip, animation, (1 - float(age) / 240.f) * strength, false);
                    emit(p, .42f, .42f, clip, animation, (1 - float(age) / 240.f) * strength * .26f, true);
                    light(p, frost, (frost ? .3f : .8f) * (1 - float(age) / 240.f), 2);
                }
                if (rem.target.damage > 0) {
                    if (frost)
                        frame.frost[rem.target.actor.slot] =
                            std::max(frame.frost[rem.target.actor.slot], .70f * fade);
                    else {
                        p.y += body->height * .08f;
                        emit(p, body->width * .65f, body->height * .8f, Clip::FireRemnant,
                             animationFrame(Clip::FireRemnant, age), fade * .70f * strength, false);
                        emit(p, body->width * .65f, body->height * .8f, Clip::FireRemnant,
                             animationFrame(Clip::FireRemnant, age), fade * .12f * strength, true);
                        light(p, false, fade * .22f * strength, 1);
                    }
                }
            } else if (rem.target.damage > 0)
                for (unsigned i = 0; i < 6; ++i) {
                    const float n = noise(rem.id.serial, i + rem.target.actor.slot * 7),
                                t = float(age) / 800.f;
                    Vec q{p.x + world.camera.right.x * (n - .5f) * .35f, p.y + t * (frost ? .12f : .35f),
                          p.z + world.camera.right.z * (n - .5f) * .35f};
                    particle(q, frost, rem.id.serial, i, fade * .4f, .025f);
                }
        }
        appendOffensive(in, world, frame, pass, particles);
    }
}
} // namespace blobber::world_render

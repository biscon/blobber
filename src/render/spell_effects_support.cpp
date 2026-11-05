#include "render/spell_effects.hpp"
#include "render/world_effects.hpp"
#include <cmath>
namespace blobber::world_render {
namespace {
using K = rules::SpellKind;
using C = world_assets::EffectClipKind;
C clip(K k) noexcept {
    switch (k) {
    case K::Bane:
        return C::Curse;
    case K::Sleep:
        return C::SleepMist;
    case K::ColorSpray:
        return C::PrismaticFan;
    case K::HoldPerson:
        return C::Restraint;
    case K::ShieldOfFaith:
        return C::Faith;
    case K::MageArmor:
        return C::Armor;
    case K::Sanctuary:
        return C::Sanctuary;
    case K::Grease:
        return C::GreaseSpread;
    case K::Web:
        return C::WebUnfurl;
    default:
        return C::Boon;
    }
}
unsigned loopFrame(std::uint64_t now) noexcept {
    constexpr unsigned frames[]{4, 5, 6, 7, 6, 5};
    return frames[(now / 160) % 6];
}
bool fresh(std::uint64_t now, std::uint64_t start, unsigned duration) noexcept {
    return now >= start && now - start < duration;
}
Vec boonAnchor(const Billboard& body, float height) noexcept {
    auto p = body.feet;
    // Authored content occupies 112 of the 128 tile texels; align its lower edge
    // with the registered feet midpoint, independently of the torso attachment.
    p.y += height * (112.f / 128.f) * .5f;
    return p;
}
float depth(const Camera& camera, Vec p) noexcept {
    return (p.x - camera.position.x) * camera.forward.x + (p.z - camera.position.z) * camera.forward.z;
}
} // namespace
void SpellEffectsPlayer::appendSupport(const FrameInput& in, const WorldFrame& world, EffectFrame& frame,
                                       unsigned pass, unsigned& particles) noexcept {
    std::array<const Billboard*, NpcWorld::maxInstances> bodies{};
    for (unsigned i = 0; i < world.count; ++i)
        if (world.billboards[i].id < bodies.size() && world.billboards[i].alive)
            bodies[world.billboards[i].id] = &world.billboards[i];
    auto emit = [&](Vec center, float width, float height, C kind, unsigned animation, float alpha,
                    bool ground = false, const SpellCastVisual* footprint = nullptr, bool glow = false) {
        if (frame.count == frame.packets.size()) {
            ++diagnostics_.packets;
            return;
        }
        const auto& c = clips_[unsigned(kind)];
        if (!c.count || c.body == UINT16_MAX)
            return;
        const float d = depth(world.camera, center);
        if (d < nearPlane || d > world.camera.farClip)
            return;
        EffectPacket p;
        p.texture = glow ? c.emission : c.body;
        p.uv = c.frames[animation % c.count];
        p.tint = {1, 1, 1, alpha};
        p.depth = d;
        p.ground = ground;
        p.additive = glow;
        constexpr float x[]{-.5f, .5f, .5f, -.5f}, y[]{-.5f, -.5f, .5f, .5f};
        for (unsigned j = 0; j < 4; ++j)
            p.points[j] = ground
                              ? Vec{center.x + x[j] * width, center.y, center.z - y[j] * height}
                              : Vec{center.x + world.camera.right.x * x[j] * width, center.y + y[j] * height,
                                    center.z + world.camera.right.z * x[j] * width};
        if (footprint) {
            p.cells = footprint->cells;
            p.origin = footprint->origin.cell;
            p.footprint = 512 | (footprint->includeOrigin ? 1024 : 0);
            for (unsigned j = 0; j < 9; ++j)
                if (footprint->eligible[j])
                    p.footprint |= 1u << j;
        }
        frame.packets[frame.count++] = p;
    };
    auto cellPoint = [&](Cell cell, float height) {
        return Vec{cell.x + .5f, elevation(in.geometry.surface(cell).floor) + height, cell.y + .5f};
    };
    if (!pass) {
        for (const auto& record : casts_) {
            const auto& c = record.cue;
            if (!c.id.serial || (c.spell != K::Sleep && c.spell != K::ColorSpray) ||
                !fresh(in.now, c.impactMs, 640))
                continue;
            const auto age = unsigned(in.now - c.impactMs);
            const auto alpha = std::min(1.f, float(640 - age) / 160.f);
            if (c.spell == K::Sleep) {
                for (unsigned i = 0; i < 9; ++i)
                    if (c.eligible[i])
                        emit(cellPoint(c.cells[i], .25f), .95f, .38f, C::SleepMist, age / 80, alpha * .65f,
                             false, &c);
            } else {
                const auto f = forward(c.origin.heading);
                for (unsigned layer = 0; layer < 3; ++layer) {
                    const float reach = .18f + (.18f + .25f * layer) * std::min(1.f, age / 160.f);
                    auto p = cellPoint(c.origin.cell, .22f + .1f * layer);
                    p.x += f.x * reach;
                    p.z += f.y * reach;
                    emit(p, .4f + reach * 2.5f, .5f, C::PrismaticFan, age / 80, alpha * .70f, false, &c);
                }
            }
        }
        if (in.support)
            for (const auto& cue : in.support->records) {
                if (!cue.id.serial || cue.id.world != world_ ||
                    !fresh(in.now, cue.occurredMs, supportDuration(cue.phase)))
                    continue;
                const auto age = unsigned(in.now - cue.occurredMs);
                const float fade = std::min(1.f, float(supportDuration(cue.phase) - age) / 160.f);
                C kind = clip(cue.spell);
                unsigned animation = std::min(7u, age / 80);
                if (cue.phase == SupportPhase::End)
                    animation = 7 - std::min(7u, age / 30);
                if (cue.spell == K::SpiritualWeapon) {
                    if (cue.phase != SupportPhase::End &&
                        (!in.area || cue.source.npc ||
                         cue.source.character.index >= in.area->ground.weapons.size() ||
                         !in.area->ground.weapons[cue.source.character.index].active))
                        continue;
                    kind = cue.phase == SupportPhase::Placement ? C::WeaponSummon
                           : cue.phase == SupportPhase::Move || cue.phase == SupportPhase::End
                               ? C::WeaponMove
                               : C::WeaponStrike;
                    emit(spiritualWeaponPosition(in.geometry, cue.cell, in.now), .72f, .72f, kind, animation,
                         fade * .75f);
                    if (cue.phase == SupportPhase::Move && !(cue.from == cue.cell))
                        emit(spiritualWeaponPosition(in.geometry, cue.from, in.now), .45f, .45f,
                             C::WeaponMove, animation, fade * .4f);
                    continue;
                }
                if (cue.phase == SupportPhase::Placement) {
                    // Placement remains subordinate to the authoritative patch; replacement/expiry cancels
                    // it.
                    bool present = false;
                    if (in.area)
                        for (const auto& a : in.area->ground.areas)
                            present |=
                                a.active && a.id.world == cue.area.world && a.id.serial == cue.area.serial;
                    if (present)
                        emit(cellPoint(cue.cell, .003f), 1, 1, kind, animation, fade * .85f, true);
                    continue;
                }
                if (!cue.target.npc || cue.target.enemy.world != world_ ||
                    cue.target.enemy.slot >= bodies.size())
                    continue;
                const auto* body = bodies[cue.target.enemy.slot];
                if (!body)
                    continue;
                auto p = cue.spell == K::Bless ? boonAnchor(*body, body->height * .85f)
                                               : attachment(world.camera, *body, body->torso).position;
                p.x -= world.camera.forward.x * .004f;
                p.z -= world.camera.forward.z * .004f;
                if (cue.phase == SupportPhase::Negated)
                    emit(p, body->width * .7f, body->height * .5f, kind, animation, fade * .22f);
                else
                    emit(p, body->width * .85f, body->height * .85f, kind, animation, fade * .65f);
            }
        return;
    }
    if (in.conditions)
        for (unsigned i = 0; i < bodies.size(); ++i) {
            const auto* body = bodies[i];
            if (!body)
                continue;
            const auto mask = in.conditions->actors[i];
            unsigned used = 0;
            auto p = attachment(world.camera, *body, body->torso).position;
            p.x -= world.camera.forward.x * .004f;
            p.z -= world.camera.forward.z * .004f;
            for (const auto kind : {EffectKind::HoldPerson, EffectKind::Restrained, EffectKind::Sleep,
                                    EffectKind::Blind, EffectKind::Bane}) {
                if (!(mask & (1u << unsigned(kind))) || used == 2)
                    continue;
                const auto c = kind == EffectKind::Restrained ? C::WebUnfurl : clip(effectSpell(kind));
                const auto anim = kind == EffectKind::Sleep ? loopFrame(in.now) : 5u;
                auto at = p;
                if (kind == EffectKind::Sleep || kind == EffectKind::Blind)
                    at.y += body->height * .28f;
                emit(at, body->width * .85f, body->height * (kind == EffectKind::Sleep ? .3f : .85f), c, anim,
                     .22f);
                ++used;
            }
            if (frame.guiding[i] > 0) {
                if (used == 2)
                    frame.guiding[i] = 0;
                else
                    ++used;
            }
            if (frame.frost[i] > 0) {
                if (used == 2)
                    frame.frost[i] = 0;
                else
                    ++used;
            }
            if (used < 2 && (mask & (1u << unsigned(EffectKind::Bless))))
                emit(boonAnchor(*body, body->height * .8f), body->width * .65f, body->height * .8f, C::Boon,
                     5, .18f);
        }
    if (in.area) {
        std::array<Cell, 16> burning{};
        unsigned count = 0;
        for (const auto& patch : in.area->ground.areas) {
            if (!patch.active || !patch.web || !patch.burnMs)
                continue;
            bool duplicate = false;
            for (unsigned i = 0; i < count; ++i)
                duplicate |= burning[i] == patch.cell;
            if (duplicate)
                continue;
            burning[count++] = patch.cell;
            const auto p = cellPoint(patch.cell, .012f);
            const auto anim = loopFrame(in.now);
            emit(p, 1, 1, C::WebBurn, anim, .85f, true);
            auto flame = p;
            flame.y += .13f;
            emit(flame, .7f, .3f, C::WebBurn, anim, .7f);
            if (particles < spellParticleCapacity) {
                ++particles;
                emit(flame, .7f, .3f, C::WebBurn, anim, .18f, false, nullptr, true);
            } else
                ++diagnostics_.particles;
            if (frame.lightCount < frame.lights.size())
                frame.lights[frame.lightCount++] = {
                    cellPoint(patch.cell, .2f), {1, .29f, .055f}, 1.5f, .25f, 0};
            else
                ++diagnostics_.lights;
        }
    }
}
} // namespace blobber::world_render

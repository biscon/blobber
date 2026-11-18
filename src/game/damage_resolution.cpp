#include "game/damage_resolution.hpp"
#include "game/effects.hpp"
#include <algorithm>
#include <limits>

namespace blobber {
NpcDamageResult damageNpc(ActorWorld& actors, EffectController& effects, ActorId id, std::int64_t amount,
                          rules::DamageType type, bool critical, std::uint64_t when) noexcept {
    rules::DamagePacket packet;
    packet.count = 1;
    packet.components[0] = {amount, type};
    return damageNpc(actors, effects, id, packet, critical, when);
}
NpcDamageResult damageNpc(ActorWorld& actors, EffectController& effects, ActorId id,
                          const rules::DamagePacket& packet, bool critical, std::uint64_t when) noexcept {
    NpcDamageResult out;
    const auto* target = actors.actor(id);
    if (!target) {
        out.actor.error = ActorError::InvalidHandle;
        return out;
    }
    if (target->life != ActorLife::Alive) {
        out.actor.error = ActorError::NotAlive;
        return out;
    }
    if (!target->hostile()) {
        out.actor.error = ActorError::NotCombatant;
        return out;
    }
    if (!packet.count || packet.count > packet.components.size()) {
        out.actor.error = ActorError::InvalidDamage;
        return out;
    }
    const auto& species = *actors.catalog().monster(target->monster);
    bool radiant = false;
    for (unsigned i = 0; i < packet.count; ++i) {
        const auto component = packet.components[i];
        if (component.amount < 0 || unsigned(component.type) >= rules::damageTypeCount) {
            out.actor.error = ActorError::InvalidDamage;
            return out;
        }
        auto& resolved = out.components[i];
        resolved = rules::resolveDamage(component.amount, component.type, species.damageProfile);
        const auto add = [](std::int64_t a, std::int64_t b) { return a + std::min(b, INT64_MAX - a); };
        out.resolved.raw = add(out.resolved.raw, resolved.raw);
        out.resolved.damage = add(out.resolved.damage, resolved.damage);
        radiant |= component.type == rules::DamageType::Radiant && resolved.damage > 0;
    }
    if (packet.count == 1)
        out.resolved.adjustment = out.components[0].adjustment;
    if (out.resolved.damage > 0 && target->hasDamage && when < target->damageTimeMs) {
        out.actor.error = ActorError::InvalidTime;
        return out;
    }
    auto disposition = LethalDisposition::AllowDeath;
    if (out.resolved.damage > 0 && out.resolved.damage >= target->hitPoints &&
        rules::hasTrait(species.traits, rules::Trait::UndeadFortitude)) {
        if (radiant)
            out.fortitude.status = FortitudeStatus::RadiantBypass;
        else if (critical)
            out.fortitude.status = FortitudeStatus::CriticalBypass;
        else {
            const auto dc = std::min(out.resolved.damage, std::numeric_limits<std::int64_t>::max() - 5) + 5;
            const auto save = effects.save(id, rules::Ability::Constitution, dc);
            out.fortitude = {save.succeeded ? FortitudeStatus::Saved : FortitudeStatus::Failed,
                             save.roll.selected, save.total, dc};
            if (save.succeeded)
                disposition = LethalDisposition::RetainOneHp;
        }
    }
    out.actor = actors.damage(id, out.resolved.damage, when, disposition);
    return out;
}
} // namespace blobber

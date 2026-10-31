#include "game/damage_resolution.hpp"
#include "game/effects.hpp"
#include <algorithm>
#include <limits>

namespace blobber {
NpcDamageResult damageNpc(ActorWorld& actors, EffectController& effects, ActorId id, std::int64_t amount,
                          rules::DamageType type, bool critical, std::uint64_t when) noexcept {
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
    if (amount < 0) {
        out.actor.error = ActorError::InvalidDamage;
        return out;
    }
    const auto& species = *actors.catalog().monster(target->monster);
    out.resolved = rules::resolveDamage(amount, type, species.damageProfile);
    if (out.resolved.damage > 0 && target->hasDamage && when < target->damageTimeMs) {
        out.actor.error = ActorError::InvalidTime;
        return out;
    }
    auto disposition = LethalDisposition::AllowDeath;
    if (out.resolved.damage > 0 && out.resolved.damage >= target->hitPoints &&
        rules::hasTrait(species.traits, rules::Trait::UndeadFortitude)) {
        if (type == rules::DamageType::Radiant)
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

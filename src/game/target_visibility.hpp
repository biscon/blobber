#pragma once
#include "game/command_types.hpp"
namespace blobber {
// This supplements authoritative combat validation with the UI's current body picking.
inline CommandReason targetVisibility(const PartyCommand& command, const rules::Catalog& catalog,
                                      const TargetSnapshot& targets, const ActorSnapshot& actors) noexcept {
    if (command.targetCount > command.enemies.size())
        return CommandReason::InvalidTargets;
    const auto visible = [&](ActorId id) {
        if (id.world != targets.world || targets.world != actors.world || targets.revision != actors.revision)
            return false;
        for (std::size_t i = 0; i < targets.count; ++i)
            if (sameActor(id, targets.ids[i]))
                return true;
        return false;
    };
    if ((command.action == PartyAction::Melee || command.action == PartyAction::BackMelee ||
         command.action == PartyAction::Ranged) &&
        command.target && !visible(*command.target))
        return CommandReason::NotVisible;
    if (command.action == PartyAction::Spell ||
        (command.action == PartyAction::Feature && command.feature == rules::Feature::SpiritualWeapon)) {
        const auto* spell = catalog.spell(command.spell);
        const auto* rule = spell ? rules::spellRules(spell->kind) : nullptr;
        if (rule && rule->hostile && !rule->area)
            for (std::size_t i = 0; i < command.targetCount; ++i)
                if (!visible(command.enemies[i]))
                    return CommandReason::NotVisible;
    }
    return CommandReason::Ready;
}
} // namespace blobber

#include "game/combat.hpp"
#include "game/perception.hpp"

namespace blobber {
void CombatController::setDefending(rules::CharacterId id) noexcept {
    state_.defending[id.index] = true;
    defendDue_[id.index] = actionReadyMs();
    if (state_.biteTargets & (1u << id.index))
        for (std::size_t i = 0; i < state_.enemyCount; ++i) {
            auto& warning = bites_[state_.enemies[i].slot];
            if (warning.target.index == id.index)
                warning.defended = true;
        }
}

void CombatController::updateBiteWarnings(Player pose, std::uint64_t now, CombatEvents& events) noexcept {
    if (!state_.biteTargets)
        return;
    state_.biteTargets = 0;
    for (std::size_t i = 0; i < state_.enemyCount; ++i) {
        const auto id = state_.enemies[i];
        auto& warning = bites_[id.slot];
        if (warning.target.index >= rules::partySize)
            continue;
        const auto* a = actors_.actor(id);
        if (effects_.canAct(id) && effects_.canAct(warning.target) && partyExposed(warning.target) &&
            inMeleeFront(map_, pose.cell, pose.heading, a->npc.cell) && frontal(id) &&
            meleeSlot(id, actors_.occupiedSlots(a->npc.cell)) == a->npc.subcell) {
            state_.biteTargets |= 1u << warning.target.index;
            continue;
        }
        CombatOutcome abandoned;
        abandoned.event = CombatEvent::BiteAbandoned;
        abandoned.enemy = id;
        abandoned.recipient = warning.target;
        abandoned.occurredMs = now;
        events.append(abandoned);
        warning = {};
    }
}

void CombatController::clearBiteWarnings() noexcept {
    if (state_.biteTargets) {
        bites_.fill({});
        state_.biteTargets = 0;
    }
}
} // namespace blobber

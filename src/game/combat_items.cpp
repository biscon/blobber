#include "game/combat.hpp"
#include <algorithm>
namespace blobber {
CommandReason CombatController::validateItem(const PartyCommand& c, Player pose) const noexcept {
    const auto* user = party_.character(c.character);
    if (!user)
        return CommandReason::InvalidCharacter;
    if (c.world != state_.world || c.turn != state_.turn)
        return CommandReason::StaleTurn;
    if (!effects_.canAct(c.character))
        return CommandReason::CannotAct;
    if (realTime(state_.phase)) {
        if (partyReady_[c.character.index] > state_.gameplayMs)
            return CommandReason::Cooldown;
    } else {
        if (state_.phase != CombatPhase::Turns)
            return CommandReason::NoEncounter;
        if (!state_.awaitingAction || !state_.activeCharacter ||
            state_.activeCharacter->index != c.character.index)
            return CommandReason::NotYourTurn;
        if (!(pose == origin_))
            return CommandReason::Moving;
    }
    if (casterResolving(c.character) || attackBusy(NpcWorld::maxInstances + c.character.index))
        return CommandReason::AttackPending;
    const auto& inv = party_.inventory();
    if (c.inventoryRevision != inv.revision())
        return CommandReason::StaleInventory;
    const auto loc = inv.locate(c.item);
    const auto* stack = inv.stack(c.item);
    const auto* item = stack ? party_.catalog().item(stack->item) : nullptr;
    if (!item || loc.character.index != c.character.index || loc.cell >= bagCells ||
        item->kind != rules::ItemKind::Consumable)
        return CommandReason::InvalidItem;
    const auto* ally = c.ally ? party_.character(*c.ally) : nullptr;
    if (!ally)
        return CommandReason::MissingTarget;
    if (ally->life == rules::Life::Dead)
        return CommandReason::DeadAlly;
    if (item->consumable == rules::Consumable::Healing)
        return ally->hitPoints < ally->stats.maxHitPoints ? CommandReason::Ready : CommandReason::FullHealth;
    if (item->consumable == rules::Consumable::Mana)
        return ally->maxMana > 0 && ally->mana < ally->maxMana ? CommandReason::Ready
                                                               : CommandReason::FullMana;
    return CommandReason::InvalidItem;
}
CombatOutcome CombatController::submitItem(const PartyCommand& c, Player pose, std::uint64_t now,
                                           CombatEvents&) noexcept {
    CombatOutcome out;
    out.reason = validateItem(c, pose);
    if (out.reason != CommandReason::Ready)
        return out;
    const auto* s = party_.inventory().stack(c.item);
    const auto itemId = s->item;
    const auto kind = party_.catalog().item(itemId)->consumable;
    if (!party_.consumeItem(c.item)) {
        out.reason = CommandReason::InvalidItem;
        return out;
    }
    const auto* ally = party_.character(*c.ally);
    out.character = c.character;
    out.recipient = *c.ally;
    out.item = itemId;
    out.hit = true;
    out.occurredMs = now;
    out.reason = CommandReason::Resolved;
    if (kind == rules::Consumable::Healing) {
        const int hp = ally->hitPoints;
        party_.heal(*c.ally, rng_.roll({2, 4, 2}).value.total);
        out.event = CombatEvent::Healing;
        out.damage = ally->hitPoints - hp;
    } else {
        const int mana = ally->mana;
        party_.restoreMana(*c.ally, 2);
        out.event = CombatEvent::Recovery;
        out.damage = ally->mana - mana;
    }
    partyReady_[c.character.index] = state_.gameplayMs + combatRoundMs;
    state_.spent[c.character.index] = true;
    refreshReadiness();
    partyBusy_ = std::max(partyBusy_, now + combatFeedbackDurationMs);
    if (state_.phase == CombatPhase::Turns) {
        state_.awaitingAction = false;
        state_.resolvingAction = true;
        due_ = now + combatFeedbackDurationMs;
    } else
        ++state_.turn;
    out.phase = state_.phase;
    return out;
}
} // namespace blobber

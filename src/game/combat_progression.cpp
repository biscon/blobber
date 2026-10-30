#include "game/combat.hpp"
namespace blobber {
bool CombatController::normalAction(rules::CharacterId id) const noexcept {
    return extraAction_[id.index] ||
           (realTime(state_.phase) ? partyReady_[id.index] <= state_.gameplayMs : !state_.spent[id.index]);
}
bool CombatController::cunningAvailable(rules::CharacterId id) const noexcept {
    const auto* c = party_.character(id);
    return c && c->build.level >= 2 && party_.progressionClass(id) == rules::ProgressionClass::Rogue &&
           cunningReady_[id.index] <= state_.gameplayMs;
}
bool CombatController::freeAction(const PartyCommand& c) const noexcept {
    if (c.action == PartyAction::Feature)
        return c.feature == rules::Feature::ActionSurge || c.feature == rules::Feature::CunningAction ||
               c.feature == rules::Feature::SpiritualWeapon || c.feature == rules::Feature::EndTurn;
    return c.action == PartyAction::Item && party_.character(c.character)->build.level >= 3 &&
           cunningAvailable(c.character);
}
bool CombatController::hasActions(rules::CharacterId id) const noexcept {
    const auto* c = party_.character(id);
    const auto* weapon = effects_.spiritualWeapon(id);
    return effects_.canAct(id) && (normalAction(id) || c->actionSurge || cunningAvailable(id) ||
                                   (weapon && weapon->readyMs <= state_.gameplayMs));
}
void CombatController::spendAction(const PartyCommand& c) noexcept {
    if (freeAction(c)) {
        if (c.action == PartyAction::Item || c.feature == rules::Feature::CunningAction)
            cunningReady_[c.character.index] = state_.gameplayMs + combatRoundMs;
        return;
    }
    if ((state_.phase == CombatPhase::Turns && state_.spent[c.character.index]) ||
        (realTime(state_.phase) && partyReady_[c.character.index] > state_.gameplayMs))
        extraAction_[c.character.index] = false;
    partyReady_[c.character.index] = state_.gameplayMs + combatRoundMs;
    state_.spent[c.character.index] = true;
}
void CombatController::completeAction(std::uint64_t now) noexcept {
    if (state_.activeCharacter && hasActions(*state_.activeCharacter)) {
        state_.awaitingAction = true;
        state_.resolvingAction = false;
        due_ = now;
        ++state_.turn;
    } else
        advance(now);
    refreshReadiness();
}
} // namespace blobber

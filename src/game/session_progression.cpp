#include "game/session.hpp"
namespace blobber {
void GameSession::reconcileRewards() noexcept {
    const auto& world = actors_.snapshot();
    reconcileObjective();
    if (rewardRevision_ == world.revision)
        return;
    rewardRevision_ = world.revision;
    for (std::size_t i = 0; i < world.count; ++i) {
        if (!world.actors[i].hasDeath || creditedDeaths_[i])
            continue;
        creditedDeaths_[i] = true;
        party_.awardExperience(actors_.catalog().monster(world.actors[i].monster)->experience);
    }
}
AdvancementRequest GameSession::advancementRequest(rules::CharacterId id) const noexcept {
    auto r = party_.advancementRequest(id);
    r.world = actors_.snapshot().world;
    return r;
}
AdvancementError GameSession::advancementEligibility(const AdvancementRequest& r) const noexcept {
    if (!runActive())
        return AdvancementError::Unavailable;
    if (r.world != actors_.snapshot().world)
        return AdvancementError::Stale;
    if (movement().transition.active())
        return AdvancementError::Moving;
    if (effects_.area(movement().player.cell) || !combat_.settledRecovery(movement().player, timeMs_))
        return AdvancementError::Unsafe;
    if (restToken_ == UINT64_MAX)
        return AdvancementError::Capacity;
    if (!effects_.canAct(r.character))
        return AdvancementError::Unavailable;
    return party_.advancementEligibility(r);
}
AdvancementResult GameSession::previewAdvancement(const AdvancementRequest& r) noexcept {
    const auto error = advancementEligibility(r);
    return error == AdvancementError::None ? party_.advance(r, true) : AdvancementResult{error, {}};
}
AdvancementResult GameSession::advance(const AdvancementRequest& r) noexcept {
    if (mutation_ != UINT64_MAX)
        ++mutation_;
    events_.clear();
    const auto error = advancementEligibility(r);
    if (error != AdvancementError::None)
        return {error, {}};
    auto result = party_.advance(r);
    if (result) {
        ++restToken_;
        movement_.clearPending();
        combat_.reconcileEffects(timeMs_, events_);
    }
    return result;
}
bool GameSession::grantNextLevelExperience() noexcept {
    if (!runActive())
        return false;
    if (mutation_ != UINT64_MAX)
        ++mutation_;
    if (!development_ || !combat_.settledRecovery(movement().player, timeMs_) ||
        movement().transition.active())
        return false;
    return party_.grantNextLevelExperience();
}
} // namespace blobber

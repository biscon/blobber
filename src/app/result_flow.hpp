#pragma once
#include "game/arena.hpp"
#include "game/campaign.hpp"
namespace blobber {
// App-only continuation: simulation/automation may still observe explicit Victory.
inline bool continueLocalVictory(GameSession& session, bool arena) noexcept {
    if (session.combat().snapshot().phase != CombatPhase::Victory ||
        (session.campaign() && (session.campaign()->status() != CampaignStatus::Active)) ||
        (arena && arenaCleared(session.actors().snapshot())))
        return false;
    session.clearPending();
    return session.combat().continueExploring();
}
} // namespace blobber

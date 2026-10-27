#include "game/session.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace blobber {
GameSession::GameSession(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
                         std::uint32_t seed, Player spawn, bool enableAi,
                         const std::vector<NpcTimings>& timings)
    : map_(map), party_(catalog), actors_(map, catalog, placements), movement_(spawn), rng_(seed),
      ai_(map, actors_, seed), combat_(map, party_, actors_, rng_, ai_, timings), enableAi_(enableAi) {
    if (map.solid(spawn.cell) || actors_.occupied(spawn.cell))
        throw std::runtime_error("Session spawn is occupied or outside walkable map");
    combat_.enableRoaming(enableAi_);
    detectContact();
}
void GameSession::detectContact() noexcept {
    combat_.observe(movement_.state().player);
    if (combat_.contact(movement_.state().player))
        movement_.clearPending();
}
const CombatEvents& GameSession::update(double seconds, bool paused) noexcept {
    events_.clear();
    if (paused) {
        movement_.clearPending();
        return events_;
    }
    if (!std::isfinite(seconds) || seconds < 0)
        seconds = 0;
    // Bound pathological caller deltas; no accumulated automatic-action catch-up.
    seconds = std::min(seconds, 60.0);
    remainderMs_ += seconds * 1000;
    const auto elapsed = static_cast<std::uint64_t>(remainderMs_);
    timeMs_ += elapsed;
    remainderMs_ -= static_cast<double>(elapsed);
    combat_.elapse(elapsed);
    combat_.resolveImpacts(movement_.state().player, timeMs_, events_);
    detectContact();
    movement_.update(map_, static_cast<float>(seconds), &actors_);
    driveFacing();
    detectContact();
    if (enableAi_ && realTime(combat_.snapshot().phase)) {
        const auto locks = combat_.movementLocks(timeMs_);
        ai_.update(elapsed, timeMs_, movement_.state().player, &locks);
        detectContact();
    }
    events_.step = combat_.update(movement_.state().player, movement_.state().transition.active(), timeMs_);
    events_.append(events_.step);
    driveFacing();
    return events_;
}
void GameSession::move(Action action, bool fresh) noexcept {
    if (!realTime(combat_.snapshot().phase))
        return;
    movement_.submit(map_, action, fresh, &actors_);
    detectContact();
}
CombatOutcome GameSession::submit(const PartyCommand& command) noexcept {
    if (movement_.state().transition.active()) {
        CombatOutcome result;
        result.reason = CommandReason::Moving;
        return result;
    }
    return combat_.submit(command, movement_.state().player, timeMs_);
}
void GameSession::driveFacing() noexcept {
    const auto& combat = combat_.snapshot();
    if (combat.phase != CombatPhase::Facing || !combat.facing || movement_.state().transition.active())
        return;
    const auto heading = movement_.state().player.heading;
    if (heading == *combat.facing)
        return;
    movement_.clearPending();
    const auto offset = (static_cast<unsigned>(*combat.facing) + 4 - static_cast<unsigned>(heading)) % 4;
    movement_.submit(map_, offset == 3 ? Action::TurnLeft : Action::TurnRight, true, &actors_);
    // Animation-disabled mode still commits every quarter turn and revision.
    if (!movement_.state().animated && movement_.state().player.heading != *combat.facing)
        movement_.submit(map_, Action::TurnRight, true, &actors_);
}
} // namespace blobber

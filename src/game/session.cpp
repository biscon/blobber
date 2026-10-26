#include "game/session.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace blobber {
GameSession::GameSession(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
                         std::uint32_t seed, Player spawn, bool enableAi,
                         const std::vector<NpcTimings>& timings)
    : map_(map), party_(catalog), actors_(map, catalog, placements), movement_(spawn), rng_(seed),
      combat_(party_, actors_, rng_, timings), ai_(map, actors_, seed), enableAi_(enableAi) {
    if (map.solid(spawn.cell) || actors_.occupied(spawn.cell))
        throw std::runtime_error("Session spawn is occupied or outside walkable map");
    detectContact();
}
void GameSession::detectContact() noexcept {
    if (combat_.contact(movement_.state().player))
        movement_.clearPending();
}
CombatOutcome GameSession::update(double seconds, bool paused) noexcept {
    if (paused) {
        movement_.clearPending();
        return {};
    }
    if (!std::isfinite(seconds) || seconds < 0)
        seconds = 0;
    // Bound pathological caller deltas; no accumulated automatic-action catch-up.
    seconds = std::min(seconds, 60.0);
    remainderMs_ += seconds * 1000;
    const auto elapsed = static_cast<std::uint64_t>(remainderMs_);
    timeMs_ += elapsed;
    remainderMs_ -= static_cast<double>(elapsed);
    detectContact();
    movement_.update(map_, static_cast<float>(seconds), &actors_);
    detectContact();
    if (enableAi_ && combat_.snapshot().phase == CombatPhase::Exploration) {
        ai_.update(elapsed, timeMs_, movement_.state().player);
        detectContact();
    }
    return combat_.update(movement_.state().player, movement_.state().transition.active(), timeMs_);
}
void GameSession::move(Action action, bool fresh) noexcept {
    if (combat_.snapshot().phase != CombatPhase::Exploration)
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
} // namespace blobber

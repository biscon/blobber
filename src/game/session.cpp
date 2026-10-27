#include "game/session.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace blobber {
GameSession::GameSession(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
                         std::uint32_t seed, Player spawn, bool enableAi,
                         const std::vector<NpcTimings>& timings, const RecoveryLevel& recovery)
    : map_(map), party_(catalog), actors_(map, catalog, placements), recovery_(recovery),
      loot_(actors_, recovery, seed), movement_(spawn), rng_(seed), ai_(map, actors_, seed),
      effects_(party_, actors_, rng_), combat_(map, party_, actors_, rng_, ai_, effects_, timings),
      enableAi_(enableAi) {
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
    loot_.reconcile(actors_);
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
    combat_.resolveImpacts(movement_.state().player, timeMs_, events_, elapsed);
    if (!effects_.mobileParty())
        movement_.clearPending();
    detectContact();
    movement_.update(map_, static_cast<float>(seconds), &actors_);
    driveFacing();
    detectContact();
    if (enableAi_ && realTime(combat_.snapshot().phase)) {
        const auto locks = combat_.movementLocks(timeMs_);
        const auto incapacitated = effects_.incapacitatedActors();
        ai_.update(elapsed, timeMs_, movement_.state().player, &locks, &incapacitated);
        detectContact();
    }
    events_.step =
        combat_.update(movement_.state().player, movement_.state().transition.active(), timeMs_, events_);
    events_.append(events_.step);
    driveFacing();
    loot_.reconcile(actors_);
    return events_;
}
void GameSession::move(Action action, bool fresh) noexcept {
    events_.clear();
    combat_.reconcileEffects(timeMs_, events_);
    if (!realTime(combat_.snapshot().phase) || !effects_.mobileParty())
        return;
    movement_.submit(map_, action, fresh, &actors_);
    detectContact();
}
InventoryError GameSession::inventoryAccess(const InventoryRequest& request) const noexcept {
    if (movement_.state().transition.active())
        return InventoryError::Moving;
    const auto source = party_.inventory().locate(request.source);
    if (source.cell >= 52)
        return InventoryError::InvalidItem;
    const auto phase = combat_.snapshot().phase;
    if (phase == CombatPhase::Victory || phase == CombatPhase::Defeat)
        return InventoryError::Unavailable;
    if ((source.cell >= bagCells || request.destination.cell >= bagCells ||
         source.character.index != request.destination.character.index ||
         request.operation == InventoryOp::Transfer) &&
        !combat_.safeRecovery(movement_.state().player))
        return InventoryError::Unsafe;
    return InventoryError::None;
}
InventoryResult GameSession::inventoryChange(const InventoryRequest& request) noexcept {
    events_.clear();
    const auto access = inventoryAccess(request);
    if (access != InventoryError::None)
        return {access};
    auto result = party_.inventoryChange(request);
    if (result && result.changed)
        combat_.reconcileEffects(timeMs_, events_);
    return result;
}
CombatOutcome GameSession::submit(const PartyCommand& command) noexcept {
    events_.clear();
    combat_.reconcileEffects(timeMs_, events_);
    if (movement_.state().transition.active()) {
        CombatOutcome result;
        result.reason = CommandReason::Moving;
        return result;
    }
    const auto out = combat_.submit(command, movement_.state().player, timeMs_, events_);
    if (out.event != CombatEvent::Cast)
        events_.append(out);
    loot_.reconcile(actors_);
    return out;
}
EffectResult GameSession::applyEffect(const EffectRequest& request) noexcept {
    events_.clear();
    combat_.reconcileEffects(timeMs_, events_);
    const auto result = effects_.apply(request, timeMs_, events_);
    combat_.reconcileEffects(timeMs_, events_);
    if (!effects_.mobileParty())
        movement_.clearPending();
    return result;
}
EffectResult GameSession::removeEffect(EffectId id) noexcept {
    events_.clear();
    combat_.reconcileEffects(timeMs_, events_);
    return effects_.remove(id, timeMs_, events_);
}
EffectResult GameSession::endConcentration(std::uint64_t world, rules::CharacterId caster) noexcept {
    if (world != actors_.snapshot().world) {
        events_.clear();
        return {EffectError::InvalidWorld};
    }
    return removeEffect(effects_.concentration(caster));
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

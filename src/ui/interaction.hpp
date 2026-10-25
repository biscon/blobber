#pragma once
#include "game/commands.hpp"
#include "input/ui_input.hpp"
namespace blobber {
// Input sampled before automatic advancement must not confirm a newly displayed turn/result.
inline bool confirmationMatches(const CombatSnapshot& presented, const CombatSnapshot& current) noexcept {
    return presented.world == current.world && presented.turn == current.turn &&
           presented.phase == current.phase && presented.awaitingAction == current.awaitingAction;
}
// Optional viewport hit has already been mapped by the renderer's read-only picking buffer.
void routeHudInput(CommandController& commands, const UiInput& input, std::optional<ActorId> viewportHit = {},
                   bool allowConfirm = true) noexcept;
} // namespace blobber

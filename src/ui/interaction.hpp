#pragma once
#include "game/commands.hpp"
#include "input/ui_input.hpp"
namespace blobber {
// Optional viewport hit has already been mapped by the renderer's read-only picking buffer.
void routeHudInput(CommandController& commands, const UiInput& input,
                   std::optional<ActorId> viewportHit = {}) noexcept;
} // namespace blobber

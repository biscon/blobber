#pragma once
#include "game/commands.hpp"
#include "game/movement.hpp"
#include "render/picking.hpp"
namespace blobber {
void drawUI(const MovementState& state, bool debug, const Map& map, const HudSnapshot* hud = nullptr);
void drawHudOverlay(const HudSnapshot* hud, const PickingBuffer* picking);
} // namespace blobber

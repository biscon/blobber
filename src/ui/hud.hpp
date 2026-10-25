#pragma once
#include "game/commands.hpp"
#include "game/movement.hpp"
#include "render/picking.hpp"
namespace blobber {
class UiFonts;
class UiArt;
void drawMapOverlay(const MovementState& state, const Map& map, const UiFonts* fonts = nullptr);
// Null fonts: geometry into the art target. Non-null: native text into the game target.
void drawUI(const MovementState& state, bool debug, const Map& map, const HudSnapshot* hud = nullptr,
            const UiFonts* fonts = nullptr, const UiArt* art = nullptr);
void drawHudOverlay(const HudSnapshot* hud, const PickingBuffer* picking, const UiFonts* fonts = nullptr,
                    const UiArt* art = nullptr, const UiFonts* metrics = nullptr);
} // namespace blobber

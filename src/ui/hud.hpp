#pragma once
#include "game/commands.hpp"
#include "game/movement.hpp"
#include "generation/diagnostic_view.hpp"
#include "render/picking.hpp"
namespace blobber {
class UiFonts;
class UiArt;
void drawMapOverlay(const MovementState& state, const Map& map, const UiFonts* fonts = nullptr,
                    const generation::DiagnosticView* generation = nullptr, const HudSnapshot* hud = nullptr,
                    bool reveal = false);
// One native-resolution UI pass; optional fonts/art permit isolated geometry diagnostics.
void drawUI(const MovementState& state, bool debug, const Map& map, const HudSnapshot* hud = nullptr,
            const UiFonts* fonts = nullptr, const UiArt* art = nullptr);
void drawHudOverlay(const HudSnapshot* hud, const PickingBuffer* picking, const UiFonts* fonts = nullptr,
                    const UiArt* art = nullptr, const UiFonts* metrics = nullptr);
} // namespace blobber

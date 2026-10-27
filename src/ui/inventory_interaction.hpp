#pragma once
#include "game/commands.hpp"
#include "input/ui_input.hpp"
namespace blobber {
bool routeInventoryInput(CommandController&, const UiInput&, bool allowConfirm) noexcept;
}

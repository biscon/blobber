#pragma once
#include "game/save_ui.hpp"
#include "ui/layout.hpp"
namespace blobber::saveLayout {
constexpr unsigned rowCount(const SaveHud& h) {
    return h.confirmation != SaveConfirmation::None ? 2u : h.terminalWarning ? 3u : 6u;
}
constexpr UiRect panel(const SaveHud& h) {
    if (h.confirmation == SaveConfirmation::None && !h.terminalWarning)
        return {145, 32, 350, 296};
    const int height = rowCount(h) == 2 ? 164 : 193;
    return {145, (360 - height) / 2, 350, height};
}
constexpr UiRect row(const SaveHud& h, unsigned i) {
    const auto p = panel(h);
    return {p.x + 27, p.y + 46 + int(i) * 29, 296, 25};
}
constexpr int messageY(const SaveHud& h) {
    if (h.confirmation == SaveConfirmation::None && !h.terminalWarning)
        return 260;
    const auto last = row(h, rowCount(h) - 1);
    return last.y + last.height + 12;
}
} // namespace blobber::saveLayout

#include "game/commands.hpp"
#include "ui/save_layout.hpp"
namespace blobber {
void CommandController::routeSaveMenu(const UiInput& input) noexcept {
    auto& h = state_.save;
    const bool confirmation = h.confirmation != SaveConfirmation::None;
    const unsigned count = saveLayout::rowCount(h);
    if (input.has(UiKey::Cancel)) {
        if (confirmation)
            h.confirmation = SaveConfirmation::None;
        else if (!h.terminalWarning)
            closeOverlay();
        h.cursor = 0;
        return;
    }
    if (input.has(UiKey::Up))
        h.cursor = (h.cursor + count - 1) % count;
    if (input.has(UiKey::Down) || input.has(UiKey::Tab))
        h.cursor = (h.cursor + 1) % count;
    bool confirmed = input.has(UiKey::Confirm);
    if (input.has(UiKey::Click) && input.pointer)
        for (unsigned i = 0; i < count; ++i)
            if (saveLayout::row(h, i).contains(*input.pointer)) {
                h.cursor = i;
                confirmed = true;
                break;
            }
    if (!confirmed)
        return;
    if (confirmation) {
        const auto action = h.confirmation;
        h.confirmation = SaveConfirmation::None;
        if (h.cursor == 1) {
            if (action == SaveConfirmation::Overwrite) {
                h.replace = true;
                h.intent = h.leaveAfter ? SaveIntent::SaveMenu : SaveIntent::Save;
            } else if (action == SaveConfirmation::Leave)
                state_.menuRequested = true;
            else
                state_.quitRequested = true;
        }
        h.cursor = 0;
        return;
    }
    if (h.terminalWarning) {
        if (h.cursor == 0)
            h.intent = SaveIntent::RetryTerminal;
        else
            h.confirmation = h.cursor == 1 ? SaveConfirmation::Leave : SaveConfirmation::Quit;
        h.cursor = 0;
        return;
    }
    switch (h.cursor) {
    case 0:
        closeOverlay();
        break;
    case 1:
    case 2:
        if (h.allowed) {
            h.replace = false;
            h.intent = h.cursor == 1 ? SaveIntent::Save : SaveIntent::SaveMenu;
        }
        break;
    case 3:
        openCamp();
        break;
    case 4:
        h.confirmation = SaveConfirmation::Leave;
        h.cursor = 0;
        break;
    case 5:
        h.confirmation = SaveConfirmation::Quit;
        h.cursor = 0;
        break;
    }
}
} // namespace blobber

#include "ui/save_screen.hpp"
#include "ui/hud_painter.hpp"
#include "ui/save_layout.hpp"
#include <cstring>
namespace blobber {
void drawSaveMenu(const HudPainter& p, const SaveHud& h) {
    const auto panel = saveLayout::panel(h);
    p.overlayBox(panel);
    const bool confirmation = h.confirmation != SaveConfirmation::None;
    const char* title = h.confirmation == SaveConfirmation::Overwrite ? "REPLACE PREVIOUS SAVE?"
                        : confirmation
                            ? (h.terminalWarning ? "LEAVE WITHOUT RETRYING?" : "LEAVE WITHOUT SAVING?")
                        : h.terminalWarning ? "COULDN'T WRITE SAVE"
                                            : "PAUSED";
    p.centered(title, {panel.x + 10, panel.y + 13, 330, 24}, hudInk::gold, UiTextStyle::Control);
    const char* labels[]{
        "Resume", "Save", "Save and Main Menu", "Rest", "Main Menu without saving", "Quit without saving"};
    const char* warning[]{"Retry", "Main Menu", "Quit"};
    const char* confirm[]{"Cancel", h.confirmation == SaveConfirmation::Overwrite ? "Replace save" : "Leave"};
    const unsigned count = saveLayout::rowCount(h);
    for (unsigned i = 0; i < count; ++i) {
        const auto r = saveLayout::row(h, i);
        const bool disabled = !confirmation && !h.terminalWarning && (i == 1 || i == 2) && !h.allowed;
        const auto ink = disabled ? hudInk::muted : hudInk::text;
        p.box(r, h.cursor == i ? hudInk::teal : hudInk::edge);
        p.centered(confirmation        ? confirm[i]
                   : h.terminalWarning ? warning[i]
                                       : labels[i],
                   r, ink, UiTextStyle::Heading);
    }
    const char* message = h.message.data();
    if (h.terminalWarning)
        message = "This run has ended, but writing the save file failed. An older save may still be "
                  "available after restarting.";
    else if (h.confirmation == SaveConfirmation::Overwrite)
        message = "Your previous campaign will be replaced after this save succeeds.";
    else if (confirmation)
        message = "The latest changes won't be saved.";
    else if (!confirmation && !h.allowed && h.reason)
        message = h.reason;
    // Fixed bounded wrapping in the paused panel, no frame strings/heap storage.
    const float y = float(saveLayout::messageY(h));
    for (unsigned line = 0; *message && line < 3; ++line) {
        char text[54]{};
        std::size_t n = std::min<std::size_t>(std::strlen(message), 52);
        if (message[n]) {
            auto split = n;
            while (split && message[split] != ' ')
                --split;
            if (split)
                n = split;
        }
        std::memcpy(text, message, n);
        message += n;
        while (*message == ' ')
            ++message;
        p.text(text, panel.x + 15, y + line * 14, hudInk::muted, UiTextStyle::Small);
    }
}
} // namespace blobber

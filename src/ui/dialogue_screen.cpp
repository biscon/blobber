#include "ui/dialogue_layout.hpp"
#include "ui/hud_painter.hpp"
namespace blobber {
void HudPainter::dialogue(const DialogueHud& h) const {
    using namespace hudInk;
    if (!h.open || h.alpha <= 0)
        return;
    const auto ink = [&](Color c) {
        c.a = static_cast<unsigned char>(c.a * h.alpha);
        return c;
    };
    const auto panel = [&](UiRect r) {
        fill(r, ink(hudInk::panel));
        border(r, ink(gold));
    };
    if (art)
        art->dialogueOverlay(h.overlay, dialogueLayout::overlay, ink(WHITE));
    panel(dialogueLayout::response);
    panel(dialogueLayout::topics);
    text(h.name, 12, 229, ink(gold), UiTextStyle::Display);
    text(h.introduction ? "First meeting" : "Conversation", 12, 240, ink(muted), UiTextStyle::Small);
    text(h.text.data(), 12, 252,
         ink(Color{static_cast<unsigned char>(h.textColor >> 24),
                   static_cast<unsigned char>(h.textColor >> 16),
                   static_cast<unsigned char>(h.textColor >> 8), 255}),
         UiTextStyle::Label);
    text(h.revealing        ? "Click / Space: reveal"
         : h.page < h.pages ? "Click / Space: continue"
                            : "Choose a topic",
         12, 343, ink(muted), UiTextStyle::Small);
    char page[24];
    if (h.pages) {
        std::snprintf(page, sizeof(page), "%u / %u", h.page, h.pages);
        text(page, 190, 343, ink(gold), UiTextStyle::Small);
    }
    if (h.introduction && !h.leaving) {
        border(dialogueLayout::skip, ink(gold));
        centered("Skip introduction", dialogueLayout::skip, ink(gold), UiTextStyle::Small);
    }
    text("ASK ABOUT", 320, 229, ink(gold), UiTextStyle::Display);
    for (unsigned i = 0; i < h.count; ++i) {
        const auto r = dialogueLayout::row(i);
        if (i == h.cursor)
            fill(r, ink(Color{48, 42, 26, 255}));
        border(r, ink(i == h.cursor ? gold : edge));
        char label[64];
        std::snprintf(label, sizeof(label), "%u   %s", i + 1, h.topics[i]);
        this->label(label, r, ink(h.read[i] ? muted : hudInk::text));
    }
    text("Esc: leave", 342, 346, ink(muted), UiTextStyle::Small);
    if (h.offset)
        text("<", 323, 346, ink(gold), UiTextStyle::Small);
    if (h.offset + h.count < h.totalTopics)
        text(">", 485, 346, ink(gold), UiTextStyle::Small);
}
} // namespace blobber

#pragma once
#include "app/arena_setup.hpp"
#include "app/creator_flow.hpp"
#include "app/menu_model.hpp"
#include "ui/creator_screen.hpp"
#include "ui/hud_painter.hpp"
#include <cstdio>
namespace blobber {
struct MenuView {
    MenuView(const rules::Catalog& c, CharacterLibrary& l)
        : catalog(c), library(l), creator{c, creation.model, {}} {}
    MenuModel model;
    ArenaSetup arena;
    std::array<PartyChoice, 4> dungeonChoices{};
    CreatorFlow creation;
    const rules::Catalog& catalog;
    CharacterLibrary& library;
    CreatorView creator;
    std::array<char, 192> error{};
    std::array<UiRect, 160> buttons{};
    std::array<int, 160> actions{};
    std::array<bool, 160> enabled{};
    MenuInteraction interaction;
    ButtonPress pointerButtons;
    int count = 0, focus = 0, detailOffset = 0, detailRows = 0;
    bool usable = false, canContinue = false;
    bool controlOverflow = false;
    std::array<char, 192> continueSummary{};
    MenuScreen detailCaller = MenuScreen::Library;
    UiInput pointerInput(const UiInput& input) noexcept {
        ButtonTargets targets;
        targets.overflow = controlOverflow;
        targets.context = {unsigned(model.screen), library.snapshot().revision,    model.firstRow,
                           model.activeSlot,       bool(model.deletion),           arena.first,
                           arena.firstFile,        unsigned(arena.pendingLoad + 1)};
        for (int i = 0; i < count; ++i)
            targets.add(buttons[i], enabled[i], actions[i] == 5 || (actions[i] >= 4011 && actions[i] <= 4013),
                        std::uint64_t(actions[i]) + 1);
        return pointerButtons.route(input, targets);
    }
    void message(const char* s) { std::snprintf(error.data(), error.size(), "%s", s); }
    void hit(UiRect r, int action) {
        if (count < int(buttons.size())) {
            buttons[count] = r;
            enabled[count] = true;
            actions[count++] = action;
        } else
            controlOverflow = true;
    }
    void button(const HudPainter& p, UiRect r, const char* label, int action, bool available = true) {
        if (count >= int(buttons.size())) {
            controlOverflow = true;
            return;
        }
        buttons[count] = r;
        actions[count] = action;
        enabled[count] = available;
        p.box(r, available && count == focus ? hudInk::teal : hudInk::edge);
        const auto style = model.screen == MenuScreen::Main ? UiTextStyle::Control : UiTextStyle::Heading;
        if (p.fonts->width(label, style) <= r.width - 8)
            p.centered(label, r, available ? hudInk::text : hudInk::muted, style);
        else
            p.label(label, r, available ? hudInk::text : hudInk::muted, style);
        ++count;
    }
};
void drawArena(MenuView&, const HudPainter&, const UiArt&);
void drawMenu(void*, const UiFonts&, const UiArt&);
} // namespace blobber

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
    MenuInteraction interaction;
    int count = 0, focus = 0, detailOffset = 0, detailRows = 0;
    bool usable = false, canContinue = false;
    std::array<char, 192> continueSummary{};
    MenuScreen detailCaller = MenuScreen::Library;
    void message(const char* s) { std::snprintf(error.data(), error.size(), "%s", s); }
    void hit(UiRect r, int action) {
        if (count < int(buttons.size())) {
            buttons[count] = r;
            actions[count++] = action;
        }
    }
    void button(const HudPainter& p, UiRect r, const char* label, int action) {
        if (count >= int(buttons.size()))
            return;
        buttons[count] = r;
        actions[count] = action;
        p.box(r, count == focus ? hudInk::teal : hudInk::edge);
        const auto style = model.screen == MenuScreen::Main ? UiTextStyle::Control : UiTextStyle::Heading;
        if (p.fonts->width(label, style) <= r.width - 8)
            p.centered(label, r, hudInk::text, style);
        else
            p.label(label, r, hudInk::text, style);
        ++count;
    }
};
void drawArena(MenuView&, const HudPainter&, const UiArt&);
void drawMenu(void*, const UiFonts&, const UiArt&);
} // namespace blobber

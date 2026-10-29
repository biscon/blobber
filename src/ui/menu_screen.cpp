#include "ui/menu_screen.hpp"
#include "ui/menu_layout.hpp"
#include <cstring>
namespace blobber {
void drawMenu(void* context, const UiFonts& fonts, const UiArt& art) {
    auto& v = *static_cast<MenuView*>(context);
    if (v.creation.active) {
        drawCreator(&v.creator, fonts, art);
        return;
    }
    auto& m = v.model;
    HudPainter p(&fonts, &art);
    v.count = 0;
    if (m.screen != MenuScreen::Main)
        p.fill({0, 0, 640, 360}, {12, 18, 18, 245});
    p.border({8, 8, 624, 344}, hudInk::edge);
    if (m.screen == MenuScreen::Main) {
        p.centered("B L O B B E R", {180, 60, 280, 50}, hudInk::gold, UiTextStyle::Control);
        v.button(p, {210, 145, 220, 32}, "Start new game", 1);
        v.button(p, {210, 194, 220, 32}, "Character Creator", 2);
        v.button(p, {210, 243, 220, 32}, "Quit", 3);
        p.text("Arrows / Tab: navigate     Enter: select", 220, 310, hudInk::muted);
    } else if (m.screen == MenuScreen::Loading) {
        p.text("Preparing your adventure...", 210, 170, hudInk::gold, UiTextStyle::Control);
    } else if (m.screen == MenuScreen::Detail) {
        p.text("CHARACTER REVIEW", 20, 18, hudInk::gold, UiTextStyle::Control);
        const auto* record = v.library.snapshot().find(m.selected);
        if (record) {
            v.detailRows = int(drawBuildReview(p, v.catalog, record->draft, unsigned(v.detailOffset)));
            v.button(p, {390, 325, 80, 22}, "Up", 33);
            v.button(p, {480, 325, 80, 22}, "Down", 34);
        }
        v.button(p, {20, 325, 100, 22}, "Back", 4);
    } else {
        const bool picker = m.screen == MenuScreen::Picker;
        p.text(picker ? "CHOOSE YOUR PARTY" : "CHARACTER CREATOR", 20, 16, hudInk::gold,
               UiTextStyle::Control);
        char text[100];
        const auto search = menuLayout::search;
        p.box(search, v.interaction.searchFocused ? hudInk::teal : hudInk::edge);
        v.hit(search, 5);
        const char* visible = m.search.data();
        while (*visible && fonts.width(visible, UiTextStyle::Heading) > search.width - 17)
            ++visible;
        const float textY = fonts.centeredY("Ag", search.y, search.height, UiTextStyle::Heading);
        if (!*visible && !v.interaction.searchFocused)
            p.text("Search characters...", search.x + 6, textY, hudInk::muted, UiTextStyle::Heading);
        else
            p.text(visible, search.x + 6, textY, hudInk::text, UiTextStyle::Heading);
        if (v.interaction.caretVisible()) {
            const float x = search.x + 7 + fonts.width(visible, UiTextStyle::Heading);
            p.line({x, float(search.y + 5)}, {x, float(search.y + search.height - 5)}, hudInk::teal, 2);
        }
        const char* race = m.filterRace ? v.catalog.race(m.race)->name.c_str() : "All races";
        const char* cls =
            m.filterClass ? v.catalog.characterClass(m.characterClass)->name.c_str() : "All classes";
        v.button(p, {278, 48, 125, 22}, race, 6);
        v.button(p, {410, 48, 125, 22}, cls, 7);
        v.button(p, {542, 48, 78, 22}, "Reload", 8);
        const int columns = picker ? 3 : 4;
        const int width = picker ? 125 : 145;
        const auto begin = m.firstRow * std::size_t(columns);
        for (std::size_t j = 0; j < 8 && begin + j < m.visibleCount && (!picker || j < 6); ++j) {
            const auto index = m.visible[begin + j];
            const auto& r = v.library.snapshot().records[index];
            int x = 20 + int(j % columns) * (width + 5), y = 80 + int(j / columns) * 112;
            p.box({x, y, width, 108}, r.id == m.selected ? hudInk::teal : hudInk::edge);
            art.draw(64 + r.draft.portrait.index, {x + (width - 60) / 2, y + 3, 60, 60});
            p.clipped(r.draft.name.data(), x + 5, y + 64, width - 10, hudInk::gold);
            char identity[96];
            std::snprintf(
                identity, sizeof(identity), "%s / %s #%.6s", v.catalog.race(r.draft.race)->name.c_str(),
                v.catalog.characterClass(r.draft.characterClass)->name.c_str(), r.id.text.data() + 30);
            p.clipped(identity, x + 5, y + 75, width - 10, hudInk::muted, UiTextStyle::Small);
            if (picker) {
                v.hit({x, y, width, 108}, 1000 + index);
                p.centered("Drag to formation", {x + 4, y + 87, width - 8, 17}, hudInk::muted,
                           UiTextStyle::Small);
            } else {
                v.button(p, {x + 4, y + 87, (width - 12) / 2, 17}, "View", 2000 + index);
                v.button(p, {x + width / 2, y + 87, (width - 12) / 2, 17}, "Delete", 3000 + index);
            }
        }
        if (!m.visibleCount) {
            p.text("No matching saved characters", 24, 100, hudInk::muted);
            v.button(p, {24, 124, 125, 22}, "Reset filters", 42);
            if (v.usable && v.library.snapshot().count < LibrarySnapshot::capacity)
                v.button(p, {160, 124, 140, 22}, "New character", 50);
        }
        if (picker) {
            for (int i = 0; i < 4; ++i) {
                const auto& c = m.choices[i];
                const rules::CharacterDraft* d = nullptr;
                rules::CharacterDraft preset;
                if (c.preset >= 0 && v.catalog.preset()) {
                    rules::draftFromBuild(v.catalog, v.catalog.preset()->characters[c.preset], preset);
                    d = &preset;
                } else if (const auto* r = v.library.snapshot().find(c.saved))
                    d = &r->draft;
                const int x = 415 + (i % 2) * 103, y = 80 + (i / 2) * 108;
                const auto rect = menuLayout::formation(i);
                p.box(rect,
                      v.interaction.dragging && v.interaction.target == i ? hudInk::teal : hudInk::edge);
                v.hit({x, y, 98, 87}, 10 + i);
                p.centered(i < 2 ? (i == 0 ? "Front left" : "Front right")
                                 : (i == 2 ? "Rear left" : "Rear right"),
                           {x, y, 98, 23}, hudInk::gold, UiTextStyle::Heading);
                if (!d)
                    p.centered("Drop character here", {x + 3, y + 35, 92, 35}, hudInk::muted,
                               UiTextStyle::Small);
                if (d) {
                    art.draw(64 + d->portrait.index, {x + 25, y + 27, 45, 48});
                    p.clipped(d->name.data(), x + 3, y + 76, 92, hudInk::gold);
                }
                v.button(p, {x, y + 88, 98, 18}, "Remove", 20 + i);
            }
            v.button(p, {180, 325, 120, 22}, "Use default party", 9);
            v.button(p, {470, 325, 150, 22},
                     m.ready(v.library.snapshot()) ? "Enter dungeon" : "Choose four characters", 30);
        }
        if (v.usable && v.library.snapshot().count < LibrarySnapshot::capacity)
            v.button(p, {picker ? 310 : 470, 325, 150, 22}, "New character", 50);
        else
            p.clipped(v.library.snapshot().count == LibrarySnapshot::capacity
                          ? "Library full: delete a character to create another."
                          : "Reload the library after resolving its error.",
                      picker ? 310 : 180, 315, picker ? 150 : 440, hudInk::muted, UiTextStyle::Small);
        v.button(p, {20, 325, 90, 22}, "Back", 4);
        v.button(p, {270, 300, 65, 18}, "Previous", 31);
        v.button(p, {340, 300, 55, 18}, "Next", 32);
        std::snprintf(text, sizeof(text), "%zu / 128 saved", v.library.snapshot().count);
        p.text(text, 25, 305, hudInk::muted);
    }
    if (v.interaction.dragging && m.screen == MenuScreen::Picker) {
        const auto& choice = v.interaction.carried;
        rules::CharacterDraft preset;
        const rules::CharacterDraft* draft = nullptr;
        if (choice.preset >= 0 && v.catalog.preset()) {
            rules::draftFromBuild(v.catalog, v.catalog.preset()->characters[choice.preset], preset);
            draft = &preset;
        } else if (const auto* saved = v.library.snapshot().find(choice.saved))
            draft = &saved->draft;
        if (draft) {
            const int x = std::clamp(int(v.interaction.pointer.x) + 8, 0, 566),
                      y = std::clamp(int(v.interaction.pointer.y) - 20, 0, 274);
            p.box({x, y, 72, 84}, hudInk::teal);
            art.draw(64 + draft->portrait.index, {x + 12, y + 3, 48, 56}, {255, 255, 255, 220});
            p.clipped(draft->name.data(), x + 4, y + 64, 64, hudInk::gold, UiTextStyle::Small);
        }
    }
    p.clipped(v.error.data(), 25, 351, 590, hudInk::red, UiTextStyle::Small);
    if (m.deletion) {
        p.fill({0, 0, 640, 360}, {0, 0, 0, 220});
        p.box({170, 55, 300, 250}, hudInk::gold);
        v.count = 0;
        p.text("Delete character?", 230, 70, hudInk::gold, UiTextStyle::Control);
        art.draw(64 + m.deletion->draft.portrait.index, {280, 102, 80, 100});
        p.clipped(m.deletion->draft.name.data(), 205, 212, 230, hudInk::gold);
        p.text("Permanently removes this library entry.", 200, 235);
        v.button(p, {190, 265, 120, 24}, "Keep character", 40);
        v.button(p, {325, 265, 125, 24}, "Delete character", 41);
        p.clipped(v.error.data(), 180, 292, 280, hudInk::red, UiTextStyle::Small);
    }
}
} // namespace blobber

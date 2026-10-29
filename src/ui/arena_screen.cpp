#include "ui/menu_screen.hpp"
#include <cstring>
namespace blobber {
namespace {
void field(MenuView& v, const HudPainter& p, UiRect rect, const std::array<char, 65>& text,
           const TextEdit& edit, int action) {
    p.box(rect, edit.focused ? hudInk::teal : hudInk::edge);
    const auto cursor = std::min(edit.cursor, std::strlen(text.data()));
    std::size_t begin = 0;
    std::array<char, 65> prefix{};
    auto width = [&] {
        std::copy_n(text.data() + begin, cursor - begin, prefix.begin());
        prefix[cursor - begin] = 0;
        return p.fonts->width(prefix.data(), UiTextStyle::Heading);
    };
    while (begin < cursor && width() > rect.width - 18)
        ++begin;
    p.clipped(text.data() + begin, rect.x + 6, rect.y + 6, rect.width - 12, hudInk::text,
              UiTextStyle::Heading);
    if (edit.focused && edit.blink < .5f) {
        const float x = rect.x + 6 + width();
        p.line({x, float(rect.y + 5)}, {x, float(rect.y + rect.height - 5)}, hudInk::teal, 1);
    }
    v.hit(rect, action);
}
} // namespace
void drawArena(MenuView& v, const HudPainter& p, const UiArt& art) {
    auto& a = v.arena;
    char text[192];
    if (v.model.screen == MenuScreen::Scenarios) {
        p.text("LOAD ARENA SCENARIO", 20, 18, hudInk::gold, UiTextStyle::Control);
        p.text("Share JSON files by copying them into this folder, then Refresh:", 20, 46, hudInk::muted);
        p.clipped(a.folder.data(), 20, 61, 600, hudInk::text, UiTextStyle::Small);
        for (std::size_t row = 0; row < 7 && a.firstFile + row < a.listing.count; ++row) {
            const auto i = a.firstFile + row;
            const auto& entry = a.listing.entries[i];
            const int y = 82 + int(row) * 31;
            p.clipped(entry.name.data(), 26, y + 3, 310, entry.error[0] ? hudInk::muted : hudInk::gold);
            p.clipped(entry.error[0] ? entry.error.data() : entry.file.data(), 26, y + 16, 460, hudInk::muted,
                      UiTextStyle::Small);
            v.button(p, {510, y, 105, 25}, entry.error[0] ? "Show error" : "Load", 4300 + int(i));
        }
        if (!a.listing.count)
            p.text("No saved scenarios yet.", 26, 110, hudInk::muted);
        v.button(p, {20, 320, 90, 25}, "Back", 4010);
        v.button(p, {120, 320, 90, 25}, "Refresh", 4009);
        v.button(p, {410, 320, 95, 25}, "Previous", 4014);
        v.button(p, {515, 320, 100, 25}, "Next", 4015);
        if (a.pendingLoad >= 0) {
            p.fill({0, 0, 640, 360}, {0, 0, 0, 215});
            p.box({135, 110, 370, 140}, hudInk::gold);
            v.count = 0;
            p.text("Replace current setup?", 160, 130, hudInk::gold, UiTextStyle::Control);
            p.text("Unsaved setup changes will be discarded.", 160, 163, hudInk::muted);
            v.button(p, {155, 200, 155, 26}, "Keep setup", 4501);
            v.button(p, {325, 200, 160, 26}, "Load scenario", 4500);
        }
        return;
    }
    p.text("ARENA", 20, 17, hudInk::gold, UiTextStyle::Control);
    p.text("SCENARIO NAME", 20, 43, hudInk::muted, UiTextStyle::Small);
    field(v, p, {20, 56, 200, 24}, a.scenario.name, a.nameEdit, 4011);
    p.text("PARTY / LEVEL 1", 20, 91, hudInk::gold, UiTextStyle::Heading);
    for (int i = 0; i < 4; ++i) {
        const auto& d = a.scenario.party[i].draft;
        const int x = 20 + (i % 2) * 103, y = 112 + (i / 2) * 70;
        p.box({x, y, 97, 65}, hudInk::edge);
        art.draw(64 + d.portrait.index, {x + 3, y + 3, 34, 42});
        p.text(i < 2 ? "Front" : "Rear", x + 42, y + 7, hudInk::muted, UiTextStyle::Small);
        p.clipped(v.catalog.characterClass(d.characterClass)->name.c_str(), x + 41, y + 24, 52, hudInk::gold,
                  UiTextStyle::Small);
        p.clipped(d.name.data(), x + 4, y + 49, 89, hudInk::text, UiTextStyle::Small);
    }
    v.button(p, {20, 255, 200, 23}, "Choose party / formation", 4001);
    p.text("SEED", 20, 285, hudInk::muted, UiTextStyle::Small);
    field(v, p, {55, 282, 115, 23}, a.seedText, a.seedEdit, 4012);
    v.button(p, {175, 282, 45, 23}, "New", 4006);
    p.text("MONSTER LIBRARY", 240, 20, hudInk::gold, UiTextStyle::Heading);
    std::snprintf(text, sizeof(text), "%u / 256 selected", a.scenario.total());
    p.text(text, 480, 23, hudInk::muted);
    field(v, p, {240, 43, 375, 25}, a.search, a.searchEdit, 4013);
    if (!a.search[0] && !a.searchEdit.focused)
        p.text("Search monsters...", 247, 51, hudInk::muted);
    for (std::size_t row = 0; row < 5 && a.first + row < a.visibleCount; ++row) {
        const auto i = a.visible[a.first + row];
        const auto& monster = *v.catalog.monster({i});
        const int y = 78 + int(row) * 41;
        unsigned quantity = 0;
        for (std::size_t n = 0; n < a.scenario.count; ++n)
            if (a.scenario.roster[n].monster.index == i)
                quantity = a.scenario.roster[n].quantity;
        p.box({240, y, 375, 37}, quantity ? hudInk::teal : hudInk::edge);
        p.clipped(monster.id.c_str(), 248, y + 5, 240, hudInk::gold, UiTextStyle::Heading);
        std::snprintf(text, sizeof(text), "HP %d   AC %d", monster.hitPoints, monster.armorClass);
        p.text(a.available[i] ? text : "Appearance unavailable", 248, y + 23, hudInk::muted,
               UiTextStyle::Small);
        if (quantity)
            v.button(p, {506, y + 6, 28, 25}, "-", 4200 + i);
        std::snprintf(text, sizeof(text), "%u", quantity);
        p.centered(text, {537, y + 6, 40, 25}, hudInk::text);
        if (a.available[i] && a.scenario.total() < 256)
            v.button(p, {580, y + 6, 28, 25}, "+", 4100 + i);
    }
    if (!a.visibleCount)
        p.text("No matching monsters.", 248, 90, hudInk::muted);
    v.button(p, {240, 286, 80, 20}, "Previous", 4007);
    v.button(p, {327, 286, 70, 20}, "Next", 4008);
    v.button(p, {20, 320, 75, 25}, "Exit", 4);
    v.button(p, {105, 320, 80, 25}, "Save", 4002);
    v.button(p, {195, 320, 90, 25}, "Save As", 4003);
    v.button(p, {295, 320, 80, 25}, "Load", 4004);
    if (const auto* error = a.error(v.catalog))
        p.clipped(error, 400, 322, 215, hudInk::muted, UiTextStyle::Small);
    else
        v.button(p, {500, 320, 115, 25}, "Start", 4005);
}
} // namespace blobber

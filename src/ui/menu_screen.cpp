#include "ui/menu_screen.hpp"
#include "ui/menu_layout.hpp"
#include <cstring>
namespace blobber {
void drawMenu(void* context, const UiFonts& fonts, const UiArt& art) {
    auto& v = *static_cast<MenuView*>(context);
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
            const auto& d = record->draft;
            auto preview = rules::previewBuild(v.catalog, d);
            art.draw(64 + d.portrait.index, {20, 55, 145, 218});
            p.clipped(d.name.data(), 20, 280, 145, hudInk::gold);
            char line[192];
            const auto* race = v.catalog.race(d.race);
            const auto* cls = v.catalog.characterClass(d.characterClass);
            std::snprintf(line, sizeof(line), "%s / %s / Level 1", race->name.c_str(), cls->name.c_str());
            p.text(line, 185, 60);
            std::snprintf(line, sizeof(line), "HP %d     AC %d     Point buy %d / 27",
                          preview.stats.maxHitPoints, preview.stats.armorClass, preview.points);
            p.text(line, 185, 85);
            p.text("Final abilities (including racial bonuses)", 185, 110, hudInk::gold);
            std::snprintf(line, sizeof(line), "STR %d  DEX %d  CON %d  INT %d  WIS %d  CHA %d",
                          preview.stats.abilities[0], preview.stats.abilities[1], preview.stats.abilities[2],
                          preview.stats.abilities[3], preview.stats.abilities[4], preview.stats.abilities[5]);
            p.text(line, 185, 128);
            p.text("Skills: not available", 185, 153, hudInk::muted);
            int row = 0;
            auto detail = [&](const char* value) {
                if (row >= v.detailOffset && row < v.detailOffset + 11)
                    p.clipped(value, 185, 170 + (row - v.detailOffset) * 12, 430);
                ++row;
                v.detailRows = row;
            };
            detail(d.defense      ? "Fighter: Defense"
                   : d.lifeDomain ? "Cleric: Life Domain"
                                  : "Class features apply in play");
            std::snprintf(line, sizeof(line), "Weight %.2f / %.2f lb", preview.stats.carriedWeight / 100.0,
                          preview.stats.carryingCapacity / 100.0);
            detail(line);
            if (rules::hasFeature(cls->features, rules::ClassFeature::SecondWind))
                detail("Second Wind");
            if (rules::hasFeature(cls->features, rules::ClassFeature::SneakAttack))
                detail("Sneak Attack");
            if (rules::hasFeature(cls->features, rules::ClassFeature::ArcaneRecovery))
                detail("Arcane Recovery");
            std::snprintf(line, sizeof(line), "Mana %d", rules::maximumMana(cls->casting, 1));
            detail(line);
            if (cls->casting != rules::Casting::None) {
                const int mod = preview.stats.modifiers[cls->casting == rules::Casting::Intelligence ? 3 : 4];
                std::snprintf(line, sizeof(line), "Class spell DC %d / attack %+d / prepared %u of %d",
                              8 + preview.stats.proficiency + mod, preview.stats.proficiency + mod,
                              rules::spellCount(d.spells.prepared), preview.preparation);
                detail(line);
            }
            if (d.spells.racial) {
                std::snprintf(line, sizeof(line), "Racial INT spell DC %d / attack %+d",
                              8 + preview.stats.proficiency + preview.stats.modifiers[3],
                              preview.stats.proficiency + preview.stats.modifiers[3]);
                detail(line);
            }
            for (std::size_t i = 0; i < v.catalog.spellCount(); ++i) {
                const rules::SpellId id{static_cast<std::uint16_t>(i)};
                const auto bit = rules::spellBit(id);
                const auto& g = d.spells;
                if ((g.cantrips | g.learned | g.prepared | g.domain | g.racial) & bit) {
                    std::snprintf(line, sizeof(line), "%s: %s",
                                  g.racial & bit     ? "Racial (INT)"
                                  : g.domain & bit   ? "Domain"
                                  : g.cantrips & bit ? "Cantrip"
                                  : g.prepared & bit ? "Prepared"
                                                     : "Learned",
                                  v.catalog.spell(id)->name.c_str());
                    detail(line);
                }
            }
            for (std::size_t i = 0; i < preview.loadout.itemCount; ++i) {
                const auto& item = preview.loadout.items[i];
                if (!item.quantity)
                    continue;
                const auto* def = v.catalog.item(item.item);
                if (!def)
                    continue;
                std::snprintf(line, sizeof(line), "%s: %s x %u",
                              (preview.loadout.equipment.body.index == i ||
                               preview.loadout.equipment.mainHand.index == i ||
                               preview.loadout.equipment.offHand.index == i ||
                               preview.loadout.equipment.ranged.index == i)
                                  ? "Equipped"
                                  : "Carried",
                              def->name.c_str(), unsigned(item.quantity));
                detail(line);
            }
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
        if (!m.visibleCount)
            p.text("No matching saved characters", 24, 100, hudInk::muted);
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

#include "ui/camp_layout.hpp"
#include "ui/creator_screen.hpp"
#include "ui/hud_painter.hpp"
namespace blobber {
void HudPainter::camp(const HudSnapshot& h) const {
    using namespace hudInk;
    const auto& c = h.camp;
    if (c.preparing) {
        drawPreparation(*this, c);
        return;
    }
    // During the final fade-in, expose the dungeon while retaining a black veil.
    if (c.playing && c.elapsedMs >= 1950) {
        const auto alpha = static_cast<unsigned char>(255 * std::clamp((2300 - c.elapsedMs) / 350., 0., 1.));
        fill(hudLayout::viewport, {0, 0, 0, alpha});
        return;
    }
    fill(hudLayout::viewport, background);
    if (art)
        art->camp(c.playing ? unsigned(c.elapsedMs / 200) % 4 : 0, campLayout::art);
    text("MAKE CAMP", viewX + 14, viewY + 8, bright, UiTextStyle::Display);
    text("A quiet moment between battles.", viewX + 14, viewY + 22, gold);
    fill(campLayout::panel, panel);
    const bool shortRest = c.request.kind == RestKind::Short;
    for (int i = 0; i < 2; ++i) {
        const auto r = i ? campLayout::longRest : campLayout::shortRest;
        box(r, (shortRest != bool(i)) ? bright : edge);
        centered(i ? "Long Rest" : "Short Rest", {r.x, r.y + 1, r.width, 12},
                 (shortRest != bool(i)) ? bright : muted, UiTextStyle::Display);
        centered(i ? "8 hours" : "1 hour", {r.x, r.y + 13, r.width, 8}, hudInk::text, UiTextStyle::Small);
    }
    char line[192];
    centered(c.playing   ? "The party rests by the fire"
             : shortRest ? "Choose Hit Dice to spend"
                         : "Recover together",
             {viewX, 213, viewWidth, 12}, gold);
    for (int i = 0; i < 4; ++i) {
        const auto& m = c.members[i];
        const int x = 21 + i * 119;
        if (fonts && fonts->width(m.name, UiTextStyle::Display) > 96)
            label(m.name, {x, 225, 104, 12}, gold, UiTextStyle::Display);
        else
            centered(m.name, {x, 225, 104, 12}, gold, UiTextStyle::Display);
        if (c.playing) {
            std::snprintf(line, sizeof(line), "+%d HP  |  +%d mana", c.result.healed[i], c.result.mana[i]);
            centered(line, {x, 241, 104, 11}, gold);
            std::snprintf(line, sizeof(line), "Hit Dice remaining: %d", m.available);
        } else if (shortRest) {
            const auto color = m.living && m.hp < m.maxHp && m.available ? gold : muted;
            box(campLayout::minus(i), color);
            centered("-", campLayout::minus(i), color);
            box(campLayout::plus(i), color);
            centered("+", campLayout::plus(i), color);
            std::snprintf(line, sizeof(line), "%u / %d", unsigned(c.request.dice[i]), m.available);
            centered(line, {x + 24, 239, 48, 16});
            std::snprintf(line, sizeof(line), "d%d %+d  |  HP %d/%d", m.sides, m.modifier, m.hp, m.maxHp);
        } else {
            std::snprintf(line, sizeof(line), m.living ? "HP %d/%d  Mana %d/%d" : "Dead - no recovery", m.hp,
                          m.maxHp, m.mana, m.maxMana);
        }
        centered(line, {x, c.playing || shortRest ? 256 : 241, 104, 11}, muted, UiTextStyle::Small);
    }
    bool changedPreparation = false;
    if (c.request.preparation.enabled)
        for (unsigned i = 0; i < 4; ++i)
            changedPreparation |= c.request.preparation.prepared[i] != c.contexts[i].baseline;
    centered(shortRest            ? "Second Wind refreshes. Mana does not recover."
             : changedPreparation ? "Prepared spell changes will apply with this rest."
                                  : "Full HP and mana. Hit Dice and class features recover.",
             {viewX, 267, shortRest ? viewWidth : 337, 10}, gold, UiTextStyle::Small);
    if (!shortRest && !c.playing) {
        box(campLayout::prepare, c.canPrepare ? teal : edge);
        centered("Prepare spells [Tab]", campLayout::prepare, c.canPrepare ? gold : muted,
                 UiTextStyle::Small);
    }
    if (c.playing) {
        centered("Rest complete", {12, 278, 320, 18}, bright);
    } else {
        if (!shortRest && !c.preview.sanctuary)
            std::snprintf(line, sizeof(line), "1 kit from %s | Available: %u",
                          c.supplier[0] ? c.supplier : "--", c.preview.kits);
        else
            std::snprintf(line, sizeof(line), "%s | Camp kits: %u",
                          c.preview.sanctuary && !shortRest ? "Sanctuary: no cost" : "No supplies required",
                          c.preview.kits);
        clipped(line, 14, 279, 323, gold, UiTextStyle::Small);
        if (!c.preview)
            clipped(restMessage(c.preview.error), 14, 289, 323, muted, UiTextStyle::Small);
        box(campLayout::cancel);
        centered("Cancel", campLayout::cancel, gold);
        box(campLayout::start, c.preview ? bright : edge);
        centered("Start Rest", campLayout::start, c.preview ? bright : muted);
    }
    if (c.playing && c.elapsedMs >= 1600) {
        const auto alpha = static_cast<unsigned char>(255 * std::clamp((c.elapsedMs - 1600) / 350., 0., 1.));
        fill(hudLayout::viewport, {0, 0, 0, alpha});
    }
}
} // namespace blobber

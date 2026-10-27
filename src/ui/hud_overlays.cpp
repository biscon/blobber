#include "ui/combat_feedback.hpp"
#include "ui/hud.hpp"
#include "ui/hud_painter.hpp"
namespace blobber {
using namespace hudInk;
void HudPainter::tooltip(const HudSnapshot& h) const {
    const auto& hover = h.hover;
    if (hover.kind == HoverKind::None || h.modal() || h.suspended)
        return;
    int rows = 0;
    for (const auto& line : hover.lines)
        if (line[0])
            ++rows;
    if (!rows)
        return;
    int width = 112;
    for (std::size_t i = 0; i < hover.lines.size(); ++i) {
        const auto* line = hover.lines[i].data();
        const float measured = metrics
                                   ? metrics->width(line, i == 0 ? UiTextStyle::Heading : UiTextStyle::Label)
                                   : float(std::strlen(line) * 4);
        width = std::max(width, int(std::ceil(measured)) + 14);
    }
    width = std::min(width, 240);
    const int height = 10 + rows * 11;
    int x = int(hover.x) + 10, y = int(hover.y) + 12;
    if (hover.kind == HoverKind::Action) {
        x = hudLayout::action(hover.index).x;
        y = 304 - height - 4;
    }
    if (x + width > artWidth - 4)
        x = int(hover.x) - width - 10;
    if (y + height > 300)
        y = int(hover.y) - height - 8;
    const UiRect r{std::clamp(x, 4, artWidth - width - 4), std::clamp(y, 18, artHeight - height - 4), width,
                   height};
    overlayBox(r);
    int lineY = r.y + 5;
    for (std::size_t i = 0; i < hover.lines.size(); ++i) {
        if (!hover.lines[i][0])
            continue;
        clipped(hover.lines[i].data(), r.x + 6, lineY, width - 12,
                i == 0   ? gold
                : i == 3 ? (hover.reason == CommandReason::Ready ? bright : muted)
                         : hudInk::text,
                i == 0 ? UiTextStyle::Heading : UiTextStyle::Label);
        lineY += 11;
    }
}
void HudPainter::details(const HudSnapshot& h) const {
    if (!h.inspected)
        return;
    const HudCard* c = nullptr;
    int formation = 0;
    for (int i = 0; i < 4; ++i)
        if (h.cards[i].id.index == h.inspected->index) {
            c = &h.cards[i];
            formation = i;
        }
    if (!c)
        return;
    const auto r = hudLayout::details;
    overlayBox(r);
    box(hudLayout::detailsClose);
    label("X", hudLayout::detailsClose, gold);
    text(c->name, r.x + 12, r.y + 10, gold, UiTextStyle::Display);
    picture(c->portrait, {r.x + 12, r.y + 30, 72, 84});
    border({r.x + 11, r.y + 29, 74, 86}, edge);
    char line[96];
    std::snprintf(line, sizeof(line), "Level %d %s / %s", c->level, c->raceName, c->className);
    clipped(line, r.x + 96, r.y + 31, 232);
    std::snprintf(line, sizeof(line), "HP %d/%d     Armor class %d", c->hp, c->maxHp, c->armorClass);
    text(line, r.x + 96, r.y + 46);
    constexpr const char* positions[]{"Front left", "Front right", "Rear left", "Rear right"};
    text(positions[formation], r.x + 96, r.y + 61, muted);
    constexpr const char* abilities[]{"STR", "DEX", "CON", "INT", "WIS", "CHA"};
    for (int i = 0; i < 6; ++i) {
        std::snprintf(line, sizeof(line), "%s %d", abilities[i], c->abilities[i]);
        text(line, r.x + 96 + (i % 3) * 75, r.y + 80 + (i / 3) * 16, gold);
    }
    text("EQUIPMENT", r.x + 12, r.y + 129, gold, UiTextStyle::Display);
    constexpr const char* slots[]{"Main hand", "Off hand", "Armor"};
    for (int i = 0; i < 3; ++i) {
        std::snprintf(line, sizeof(line), "%s: %s", slots[i], c->equipment[i]);
        clipped(line, r.x + 12, r.y + 145 + i * 13, 320);
    }
    clipped(c->effects.data(), r.x + 96, r.y + 115, 232, blue, UiTextStyle::Small);
    text("READ ONLY / PAUSED / ESC TO CLOSE", r.x + 12, r.y + 192, muted, UiTextStyle::Small);
}
namespace {
void splash(const HudPainter& p, const CombatFeedback& feedback, UiRect r) {
    if (!p.fonts) {
        const Color color = feedback.healing ? Color{35, 140, 95, 255}
                            : feedback.hit   ? Color{190, 24, 18, 255}
                                             : WHITE;
        const Vector2 center{r.x + r.width * .5f, r.y + r.height * .5f};
        for (std::size_t i = 0; i < bloodSplat.size(); ++i) {
            const auto a = bloodSplat[i], b = bloodSplat[(i + 1) % bloodSplat.size()];
            const Vector2 one{r.x + a.x * (r.width - 1), r.y + a.y * (r.height - 1)},
                two{r.x + b.x * (r.width - 1), r.y + b.y * (r.height - 1)};
            DrawTriangle(center, two, one, color);
            DrawLineV(one, two, {65, 16, 12, 255});
        }
    } else {
        char label[32];
        if (feedback.text)
            std::snprintf(label, sizeof(label), "%s", feedback.text);
        else if (feedback.hit)
            std::snprintf(label, sizeof(label), feedback.healing ? "+%lld" : "%lld",
                          static_cast<long long>(feedback.damage));
        else
            std::snprintf(label, sizeof(label), "MISS");
        const auto width = p.fonts->width(label, UiTextStyle::Heading);
        p.text(label, r.x + (r.width - width) * .5f,
               p.fonts->centeredY(label, float(r.y), float(r.height), UiTextStyle::Heading),
               feedback.hit ? WHITE : BLACK, UiTextStyle::Heading);
    }
}
} // namespace
void drawHudOverlay(const HudSnapshot* h, const PickingBuffer* picking, const UiFonts* fonts,
                    const UiArt* art, const UiFonts* metrics) {
    if (!h)
        return;
    const HudPainter p(fonts, art, metrics);
    const auto& combat = h->combat;
    if (!h->modal() && picking && (!h->suspended || h->magic.armed)) {
        std::array<std::uint8_t, NpcWorld::maxInstances> marks{};
        if (h->magic.armed) {
            for (std::size_t i = 0; i < h->magic.affectedCount; ++i) {
                const auto id = h->magic.affected[i];
                if (id.world == picking->targets().world && id.slot < marks.size())
                    marks[id.slot] = h->magic.area ? 2 : 1;
            }
        }
        if (!h->suspended && h->hover.kind == HoverKind::Enemy && h->hover.enemy &&
            h->hover.enemy->world == picking->targets().world && h->hover.enemy->slot < marks.size() &&
            !h->magic.area)
            marks[h->hover.enemy->slot] = marks[h->hover.enemy->slot]               ? 4
                                          : h->hover.reason == CommandReason::Ready ? 3
                                                                                    : 5;
        if (!fonts)
            for (int y = 0; y < viewHeight; ++y)
                for (int x = 0; x < viewWidth; ++x) {
                    const auto slot = picking->slot(x, y);
                    if (slot >= marks.size() || !marks[slot])
                        continue;
                    if (picking->slot(x - 1, y) != slot || picking->slot(x + 1, y) != slot ||
                        picking->slot(x, y - 1) != slot || picking->slot(x, y + 1) != slot) {
                        const Color color =
                            marks[slot] == 1 ? teal
                            : marks[slot] == 2
                                ? (h->magic.kind == rules::SpellKind::Thunderwave ? Color{96, 185, 236, 255}
                                                                                  : Color{235, 148, 45, 255})
                            : marks[slot] == 4 ? Color{174, 238, 230, 255}
                            : marks[slot] == 3 ? bright
                                               : muted;
                        DrawPixel(viewX + x, viewY + y, color);
                    }
                }
        if (!h->magic.casting() && h->hover.kind == HoverKind::Enemy && h->hover.enemy &&
            h->hover.enemy->slot < picking->targets().anchors.size()) {
            const auto anchor = picking->targets().anchors[h->hover.enemy->slot];
            if (anchor.valid)
                p.health({viewX + std::clamp(anchor.x - 23, 0, viewWidth - 46),
                          viewY + std::clamp(anchor.top - 8, 0, viewHeight - 6), 46, 6},
                         h->hover.health);
        }
    }
    if (!h->suspended && combat.phase == CombatPhase::Turns && combat.upcomingCount) {
        const int width = 54 + int(combat.upcomingCount) * 30;
        const UiRect r{viewX + (viewWidth - width) / 2, 24, width, 34};
        p.overlayBox(r, edge);
        char line[32];
        std::snprintf(line, sizeof(line), "ROUND %llu", static_cast<unsigned long long>(combat.round));
        p.label(line, {r.x + 2, r.y + 8, 50, 16}, gold, UiTextStyle::Small);
        for (std::size_t i = 0; i < combat.upcomingCount; ++i) {
            const auto& participant = combat.upcoming[i];
            const UiRect tile{r.x + 54 + int(i) * 30, r.y + 3, 26, 27};
            unsigned portrait = 4;
            if (participant.character)
                for (const auto& card : h->cards)
                    if (card.id.index == participant.character->index)
                        portrait = card.portrait;
            p.picture(portrait, {tile.x + 1, tile.y + 1, 24, 24});
            p.border(tile, i == 0 ? bright : edge);
            if (i == 0)
                p.border({tile.x - 1, tile.y - 1, tile.width + 2, tile.height + 2}, gold);
            if (participant.enemy) {
                std::snprintf(line, sizeof(line), "%u", unsigned(participant.enemy->slot) + 1);
                p.fill({tile.x + 17, tile.y + 17, 8, 9}, background);
                p.text(line, tile.x + 18, tile.y + 17, gold, UiTextStyle::Small);
            }
            if (participant.deathSave) {
                p.fill({tile.x, tile.y + 17, 26, 10}, background);
                p.text("SAVE", tile.x + 2, tile.y + 18, red, UiTextStyle::Small);
            }
        }
    }
    p.spellMarkers(*h);
    for (const auto& feedback : h->feedback)
        if (const auto rect = feedbackRect(feedback, h->cards))
            splash(p, feedback, *rect);
    if (combat.phase == CombatPhase::Victory || combat.phase == CombatPhase::Defeat) {
        p.overlayBox(hudLayout::result);
        const auto r = hudLayout::result;
        p.text(combat.phase == CombatPhase::Victory ? "VICTORY" : "DEFEAT", r.x + 14, r.y + 12, gold,
               UiTextStyle::Display);
        p.text(combat.phase == CombatPhase::Victory ? "Surviving allies are stable." : "Nobody is conscious.",
               r.x + 14, r.y + 30);
        p.text(combat.phase == CombatPhase::Victory ? "HP is not restored."
                                                    : "Restart begins a fresh session.",
               r.x + 14, r.y + 42, muted);
        p.box(hudLayout::resultButton, gold);
        p.label(combat.phase == CombatPhase::Victory ? "CONTINUE / ENTER" : "RESTART / ENTER",
                hudLayout::resultButton, gold);
    }
    if (h->modal() && !h->magic.open)
        p.fill({0, 16, 640, 344}, {0, 0, 0, 110});
    if (h->logExpanded) {
        p.overlayBox(hudLayout::expandedLog);
        p.log(*h, true);
    }
    if (h->inspected)
        p.details(*h);
    if (h->menuOpen) {
        p.overlayBox(hudLayout::menu);
        p.text("PAUSED", hudLayout::menu.x + 16, hudLayout::menu.y + 15, gold, UiTextStyle::Display);
        p.box(hudLayout::resume, gold);
        p.label("RESUME", hudLayout::resume, gold);
        p.box(hudLayout::quit);
        p.label("QUIT GAME", hudLayout::quit);
    }
    p.spellbook(*h);
    p.tooltip(*h);
    if (h->demo) {
        p.label("F2 NEXT", hudLayout::demoNext, gold, UiTextStyle::Small);
        p.label("F3 RESET", hudLayout::demoReset, gold, UiTextStyle::Small);
    }
}
} // namespace blobber

#include "ui/hud_painter.hpp"
namespace blobber {
using namespace hudInk;
namespace {
void wrapped(const HudPainter& p, const char* text, int x, int y, int width, int rows) {
    const char* cursor = text;
    for (int row = 0; *cursor && row < rows; ++row) {
        char line[128]{};
        std::size_t length = 0, lastSpace = 0;
        while (cursor[length] && length < sizeof(line) - 1) {
            line[length] = cursor[length];
            line[length + 1] = 0;
            const auto measured = p.metrics ? p.metrics->width(line, UiTextStyle::Label) : float(length * 4);
            if (measured > width) {
                line[length] = 0;
                break;
            }
            if (cursor[length] == ' ')
                lastSpace = length;
            ++length;
        }
        if (cursor[length] && lastSpace)
            length = lastSpace;
        if (!length)
            length = 1;
        line[length] = 0;
        p.text(line, x, y + row * 12);
        cursor += length;
        while (*cursor == ' ')
            ++cursor;
    }
}
} // namespace
void HudPainter::spellbook(const HudSnapshot& h) const {
    const auto& m = h.magic;
    if (!m.open)
        return;
    overlayBox(hudLayout::spellbook);
    text("SPELLS / ABILITIES", 54, 46, gold, UiTextStyle::Display);
    box(hudLayout::spellbookClose);
    label("X", hudLayout::spellbookClose, gold);
    char line[96];
    std::snprintf(line, sizeof(line), "Mana %d/%d    /    PAUSED", m.mana, m.maxMana);
    text(line, 298, 47, blue);
    for (std::size_t i = m.offset; i < m.count && i < m.offset + 10; ++i) {
        const auto& e = m.entries[i];
        const auto r = hudLayout::spellEntry(int(i - m.offset));
        const bool available = e.reason == CommandReason::Ready || e.reason == CommandReason::MissingTarget;
        box(r, i == m.cursor ? bright : edge);
        clipped(e.name, r.x + 5, r.y + 4, 169, available ? gold : muted);
        std::snprintf(line, sizeof(line), "%d", e.cost);
        text(line, r.x + 186, r.y + 4, blue);
    }
    if (!m.count) {
        wrapped(*this,
                "Rogue: rear-row melee and conditional Sneak Attack are automatic. Existing racial stat and "
                "training passives remain active.",
                284, 78, 294, 8);
    } else {
        const auto& e = m.entries[m.cursor];
        clipped(e.name, 284, 74, 290, gold, UiTextStyle::Heading);
        wrapped(*this, e.description, 284, 96, 294, 8);
        if (e.charges >= 0)
            std::snprintf(line, sizeof(line), "Uses remaining %d/1  /  Mana cost %d", e.charges, e.cost);
        else if (e.feature != rules::Feature::None)
            std::snprintf(line, sizeof(line), "No action or mana cost");
        else
            std::snprintf(line, sizeof(line), "Mana cost %d  /  Attack %+d  /  Save DC %d", e.cost,
                          m.attackBonus, m.difficulty);
        clipped(line, 284, 200, 294, blue);
        wrapped(*this,
                e.reason == CommandReason::MissingTarget
                    ? "Choose this spell, then select targets in the dungeon."
                    : commandMessage(e.reason),
                284, 216, 294, 3);
    }
    if (m.count > 10) {
        box(hudLayout::spellPrevious);
        label("PREVIOUS", hudLayout::spellPrevious);
        box(hudLayout::spellNext);
        label("NEXT", hudLayout::spellNext);
    }
    text("Click / Up-Down + Enter: choose    /    Esc: close", 54, 280, muted, UiTextStyle::Small);
}
void HudPainter::spellTargets(const HudSnapshot& h) const {
    const auto& m = h.magic;
    if (!m.armed || h.modal())
        return;
    const auto r = hudLayout::spellTargetPanel;
    overlayBox(r);
    char line[128];
    std::snprintf(line, sizeof(line), "%s / %d mana", m.name, m.cost);
    clipped(line, r.x + 6, r.y + 4, 390, gold, UiTextStyle::Heading);
    clipped(m.targets.data(), r.x + 6, r.y + 17, 390, blue);
    clipped(h.availability == CommandReason::Ready ? "Enter / Cast to confirm. Esc cancels."
                                                   : commandMessage(h.availability),
            r.x + 6, r.y + 32, 390, muted, UiTextStyle::Small);
    box(hudLayout::spellCast, h.availability == CommandReason::Ready ? bright : edge);
    label("CAST", hudLayout::spellCast, h.availability == CommandReason::Ready ? gold : muted);
    if (m.area)
        for (int i = 0; i < 3; ++i) {
            const UiRect tile{440 + i * 16, 251, 13, 13};
            fill(tile, m.cells[i] ? Color{144, 73, 27, 255} : background);
            border(tile, gold);
        }
    for (std::size_t i = 0; i < m.affectedCount; ++i) {
        const auto id = m.affected[i];
        if (id.world != h.targets.world || id.slot >= h.targets.anchors.size())
            continue;
        const auto anchor = h.targets.anchors[id.slot];
        if (!anchor.valid)
            continue;
        const int x = viewX + anchor.x, y = viewY + anchor.top - 12;
        box({x - 18, y, 36, 11}, gold);
        std::snprintf(line, sizeof(line), "#%u", unsigned(id.slot) + 1);
        clipped(line, x - 15, y + 1, 30, gold, UiTextStyle::Small);
    }
}
} // namespace blobber

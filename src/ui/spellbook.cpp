#include "ui/hud_painter.hpp"
#include "ui/spell_layout.hpp"
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
    char line[128];
    const char* caster = "";
    for (const auto& c : h.cards)
        if (c.id.index == h.selection.character.index)
            caster = c.name;
    std::snprintf(line, sizeof(line), "%s - SPELLS", caster);
    clipped(line, 15, 138, 190, gold, UiTextStyle::Display);
    std::snprintf(line, sizeof(line), "Mana %d/%d", m.mana, m.maxMana);
    text(line, 220, 139, blue, UiTextStyle::Small);
    box(hudLayout::spellbookClose);
    label("X", hudLayout::spellbookClose, gold, UiTextStyle::Small);
    box({143, 153, 165, 140});
    for (std::size_t i = m.offset; i < m.count && i < m.offset + hudLayout::spellRows; ++i) {
        const auto& e = m.entries[i];
        const auto r = hudLayout::spellEntry(int(i - m.offset));
        const bool available = e.reason == CommandReason::Ready || e.reason == CommandReason::MissingTarget;
        box(r, i == m.cursor ? teal : edge);
        picture(UiArt::spellIcon(e.kind, e.feature), {r.x + 2, r.y + 2, r.height - 4, r.height - 4},
                available ? WHITE : Color{100, 108, 106, 255});
        clipped(e.name, r.x + 21, r.y + 4, 89, available ? gold : muted, UiTextStyle::Label);
        std::snprintf(line, sizeof(line), "%d", e.cost);
        text(line, r.x + 112, r.y + 4, available ? blue : muted, UiTextStyle::Small);
        if (!available) {
            fill({r.x + 10, r.y + 10, 5, 5}, background);
            text("-", r.x + 10, r.y + 8, muted, UiTextStyle::Small);
        }
    }
    if (!m.count) {
        wrapped(*this, "Rogue abilities are automatic: rear-row melee and conditional Sneak Attack.", 166,
                140, 158, 8);
    } else {
        const auto& e = m.entries[m.cursor];
        picture(UiArt::spellIcon(e.kind, e.feature), {149, 159, 28, 28});
        clipped(e.name, 183, 161, 119, gold, UiTextStyle::Heading);
        if (e.charges >= 0)
            std::snprintf(line, sizeof(line), "Uses %d/1", e.charges);
        else
            std::snprintf(line, sizeof(line), "%d mana", e.cost);
        text(line, 183, 175, blue, UiTextStyle::Small);
        wrapped(*this, e.description, 149, 193, 153, 5);
        const auto* rules = rules::spellRules(e.kind);
        const char* targeting =
            e.feature == rules::Feature::DismissConcentration ? "No action or mana cost."
            : e.feature == rules::Feature::ArcaneRecovery     ? "No action / safe exploration."
            : e.feature != rules::Feature::None               ? "Self / 1 action."
            : rules && rules->area                            ? "Fixed area / 1 action."
            : rules && rules->hostile
                ? (e.kind == rules::SpellKind::InflictWounds ? "Exposed melee / 1 action."
                                                             : "6 cells / clear sight / 1 action.")
                : "Ally portraits / 1 action.";
        clipped(targeting, 149, 253, 153, blue, UiTextStyle::Small);
        const bool available = e.reason == CommandReason::Ready || e.reason == CommandReason::MissingTarget;
        wrapped(*this, available ? "Click or Enter to choose." : commandMessage(e.reason), 149, 268, 153, 2);
    }
    if (m.count > hudLayout::spellRows) {
        label("Up / Down", hudLayout::spellPrevious, muted, UiTextStyle::Small);
        std::snprintf(line, sizeof(line), "%zu-%zu / %zu", m.offset + 1,
                      std::min(m.count, m.offset + hudLayout::spellRows), m.count);
        label(line, hudLayout::spellNext, muted, UiTextStyle::Small);
    } else
        text("Esc: close / Browsing pauses", 15, 284, muted, UiTextStyle::Small);
}
void HudPainter::spellTargets(const HudSnapshot& h) const {
    const auto& m = h.magic;
    if (!m.casting())
        return;
    overlayBox(hudLayout::spellTargetPanel);
    const auto& c = m.pending;
    const char* caster = "";
    for (const auto& card : h.cards)
        if (card.id.index == c.character.index)
            caster = card.name;
    char line[128];
    std::snprintf(line, sizeof(line), "%s - %s", caster, m.name);
    picture(UiArt::spellIcon(m.kind, c.feature), {12, 314, 34, 34});
    border({11, 313, 36, 36}, gold);
    clipped(line, 53, 309, 229, gold, UiTextStyle::Display);
    std::snprintf(line, sizeof(line), "%d mana / %s", m.cost,
                  c.feature == rules::Feature::ArcaneRecovery ||
                          c.feature == rules::Feature::DismissConcentration
                      ? "no action"
                      : "1 action");
    text(line, 53, 321, hudInk::text, UiTextStyle::Small);
    const bool resolving = m.state == CastingState::Resolving;
    const bool enabled = !resolving && !h.suspended && !h.modal() && h.availability == CommandReason::Ready;
    clipped(resolving  ? "Resolving..."
            : m.single ? (m.hostile ? "Choose an enemy" : "Choose an ally")
                       : m.targets.data(),
            53, 333, m.area ? 229 : 145, teal, UiTextStyle::Small);
    const char* hint =
        m.feedback[0] ? m.feedback.data()
        : m.single    ? "Click to cast / Esc: cancel"
        : m.missiles  ? (m.assigned == 3 ? "Remove a dart to reassign it." : "Assign all 3 darts to cast.")
        : m.area
            ? (m.affectedCount ? "Fixed area ahead. Inspect, then Cast." : "No enemies in the affected area.")
        : c.action == PartyAction::Feature ? "Confirm to use this ability."
        : !c.targetCount                   ? "Choose at least one recipient."
                                           : "Click recipients to toggle selection.";
    if (!resolving && h.availability != CommandReason::Ready &&
        h.availability != CommandReason::MissingTarget && h.availability != CommandReason::InvalidTargets &&
        !m.feedback[0])
        hint = commandMessage(h.availability);
    if (resolving)
        hint = "Waiting for the cast to finish.";
    clipped(hint, 53, 347, 229, muted, UiTextStyle::Small);
    box(hudLayout::spellCast, enabled && !m.single ? bright : edge);
    if (enabled && !m.single)
        fill({290, 312, 86, 15}, {65, 48, 18, 255});
    label(resolving                           ? "RESOLVING"
          : m.single                          ? "Click a target"
          : c.action == PartyAction::Feature  ? "Use [Enter]"
          : m.kind == rules::SpellKind::Bless ? "Cast Bless [Enter]"
                                              : "Cast [Enter]",
          hudLayout::spellCast, enabled && !m.single ? bright : muted, UiTextStyle::Heading);
    box(hudLayout::spellBack);
    label("Back [B]", hudLayout::spellBack, resolving ? muted : gold, UiTextStyle::Small);
    box(hudLayout::spellCancel);
    label("Cancel [Esc]", hudLayout::spellCancel, resolving ? muted : gold, UiTextStyle::Small);
    if (resolving)
        return;
    if (m.area) {
        for (int i = 0; i < 3; ++i) {
            const UiRect tile{227 + i * 16, 321, 13, 7};
            fill(tile, m.cells[i] ? Color{115, 68, 24, 255} : background);
            border(tile, m.cells[i] ? gold : muted);
            if (!m.cells[i])
                text("x", tile.x + 4, tile.y, muted, UiTextStyle::Small);
        }
        if (!fonts)
            DrawTriangle({248, 331}, {254, 331}, {251, 328}, gold);
    } else if (m.missiles) {
        for (unsigned i = 0; i < 3; ++i) {
            const int x = 215 + int(i) * 20;
            fill({x, 330, 8, 8}, i < m.assigned ? teal : background);
            border({x, 330, 8, 8}, i < m.assigned ? teal : muted);
        }
    } else if (!m.hostile && c.action == PartyAction::Spell && !m.single) {
        for (std::size_t i = 0; i < c.targetCount; ++i)
            for (const auto& card : h.cards)
                if (card.id.index == c.allies[i].index) {
                    const auto r = hudLayout::recipientChip(int(i));
                    picture(card.portrait, r);
                    border(r, teal);
                    checkmark({r.x + r.width - 8, r.y, 8, 8});
                }
    }
}
void HudPainter::spellMarkers(const HudSnapshot& h) const {
    const auto& m = h.magic;
    if (!m.armed || h.modal())
        return;
    const auto markers = spellMarkerLayout(h);
    for (std::size_t i = 0; i < m.affectedCount; ++i) {
        if (!markers[i])
            continue;
        const auto rect = *markers[i];
        const int x = rect.x, y = rect.y, width = rect.width;
        const bool area = m.area;
        box({x, y, width, 12}, area ? gold : teal);
        if (m.missiles) {
            dart({x + 2, y + 2, 8, 8});
            char count[4];
            std::snprintf(count, sizeof(count), "%u", unsigned(m.pending.darts[i]));
            text(count, x + 13, y + 2, teal, UiTextStyle::Small);
        } else if (area)
            picture(UiArt::spellIcon(m.kind), {x + 1, y + 1, 10, 10});
        else
            checkmark({x + 2, y + 2, 8, 8});
    }
}
} // namespace blobber

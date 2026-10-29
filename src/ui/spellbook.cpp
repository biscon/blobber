#include "ui/hud_painter.hpp"
#include "ui/spell_layout.hpp"
namespace blobber {
using namespace hudInk;
namespace {
void usesLabel(char* output, std::size_t size, int charges) {
    std::snprintf(output, size, "%d %s remaining", charges, charges == 1 ? "use" : "uses");
}
const char* featureAction(rules::Feature feature, CombatPhase phase) {
    if (feature == rules::Feature::DismissConcentration)
        return "No action cost";
    if (feature == rules::Feature::ArcaneRecovery)
        return "6s cooldown / safe exploration";
    return realTime(phase) ? "6s cooldown" : "1 action";
}
void wrapped(const HudPainter& p, const char* text, int x, int y, int width, int rows, int rowHeight = 12) {
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
        p.text(line, x, y + row * rowHeight);
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
    std::snprintf(line, sizeof(line), "%s - %s", caster, m.spellcaster ? "SPELLS" : "CLASS FEATURES");
    clipped(line, 15, 120, 190, gold, UiTextStyle::Display);
    if (m.maxMana > 0) {
        std::snprintf(line, sizeof(line), "Mana %d/%d", m.mana, m.maxMana);
        text(line, 220, 121, blue, UiTextStyle::Small);
    }
    box(hudLayout::spellbookClose);
    label("X", hudLayout::spellbookClose, gold, UiTextStyle::Small);
    int tabX = 14;
    for (unsigned t = 0; t < m.tabs.size(); ++t)
        if (m.tabs[t]) {
            const auto r = hudLayout::spellTab(int(t), tabX);
            box(r, t == m.tab ? gold : edge);
            if (t == m.tab)
                fill({r.x + 1, r.y + 1, r.width - 2, r.height - 2}, {49, 41, 23, 255});
            label(hudLayout::spellTabName(t), r, t == m.tab ? bright : muted, UiTextStyle::Small);
            tabX += r.width + 2;
        }
    const auto details = hudLayout::spellDetails;
    const int dx = details.x + 6, dy = details.y + 6, dw = details.width - 12;
    box(details);
    for (std::size_t i = m.offset; i < m.visibleCount && i < m.offset + hudLayout::spellRows; ++i) {
        const auto index = m.visible[i];
        const auto& e = m.entries[index];
        const auto r = hudLayout::spellEntry(int(i - m.offset));
        const bool automatic = e.category == PickerEntryKind::AutomaticFeature;
        const bool available =
            automatic || e.reason == CommandReason::Ready || e.reason == CommandReason::MissingTarget;
        box(r, index == m.cursor ? teal : edge);
        picture(automatic ? UiArt::classFeaturesIcon : UiArt::spellIcon(e.kind, e.feature),
                {r.x + 3, r.y + 3, r.height - 6, r.height - 6},
                available ? WHITE : Color{100, 108, 106, 255});
        border({r.x + 2, r.y + 2, r.height - 4, r.height - 4}, {29, 36, 33, 255});
        const bool spell = e.category == PickerEntryKind::Spell;
        clipped(e.name, r.x + 21, r.y + 4, spell ? 89 : 62, available ? gold : muted, UiTextStyle::Label);
        if (automatic)
            std::snprintf(line, sizeof(line), "Automatic");
        else if (spell)
            std::snprintf(line, sizeof(line), "%d", e.cost);
        else if (e.charges >= 0)
            std::snprintf(line, sizeof(line), "%d %s", e.charges, e.charges == 1 ? "use" : "uses");
        else
            std::snprintf(line, sizeof(line), "Free");
        label(line, {r.x + (spell ? 109 : 85), r.y + 2, spell ? 12 : 36, r.height - 4},
              available ? blue : muted, UiTextStyle::Small);
        if (!available) {
            fill({r.x + 10, r.y + 10, 5, 5}, background);
            text("-", r.x + 10, r.y + 8, muted, UiTextStyle::Small);
        }
    }
    if (!m.visibleCount) {
        wrapped(*this,
                m.spellcaster ? "No spells or class features available." : "No class features available.", dx,
                dy, dw, 8);
    } else {
        const auto& e = m.entries[m.cursor];
        const bool automatic = e.category == PickerEntryKind::AutomaticFeature;
        picture(automatic ? UiArt::classFeaturesIcon : UiArt::spellIcon(e.kind, e.feature), {dx, dy, 28, 28});
        clipped(e.name, dx + 34, dy + 2, dw - 34, gold, UiTextStyle::Heading);
        if (automatic)
            std::snprintf(line, sizeof(line), "Automatic");
        else if (e.charges >= 0)
            usesLabel(line, sizeof(line), e.charges);
        else if (e.category == PickerEntryKind::ActivatedFeature)
            std::snprintf(line, sizeof(line), "No resource cost");
        else if (const auto level = rules::spellRules(e.kind)->level; level)
            std::snprintf(line, sizeof(line), "Level %d / %d mana", level, e.cost);
        else
            std::snprintf(line, sizeof(line), "Cantrip / %d mana", e.cost);
        clipped(line, dx + 34, dy + 16, dw - 34, blue, UiTextStyle::Small);
        wrapped(*this, e.description, dx, dy + 34, dw, 5, 10);
        const auto* rules = rules::spellRules(e.kind);
        const char* targeting = automatic                           ? "No separate action or activation."
                                : e.feature != rules::Feature::None ? featureAction(e.feature, h.combat.phase)
                                : rules && rules->ground            ? "Aim on floor / 1 action."
                                : rules && rules->self              ? "Self / 1 action."
                                : rules && rules->area              ? "Fixed area / 1 action."
                                : rules && rules->hostile ? (e.kind == rules::SpellKind::InflictWounds
                                                                 ? "Exposed melee / 1 action."
                                                                 : "6 cells / clear sight / 1 action.")
                                                          : "Ally portraits / 1 action.";
        if (e.category == PickerEntryKind::Spell) {
            std::snprintf(line, sizeof(line), "%s %s / Attack %+d / DC %d",
                          e.source == rules::SpellSource::Racial   ? "Racial"
                          : e.source == rules::SpellSource::Domain ? "Domain"
                                                                   : "Class",
                          e.casting == rules::Casting::Wisdom ? "WIS" : "INT", e.attack, e.difficulty);
            clipped(line, dx, dy + 86, dw, teal, UiTextStyle::Small);
        }
        clipped(targeting, dx, dy + 98, dw, blue, UiTextStyle::Small);
        const bool available = e.reason == CommandReason::Ready || e.reason == CommandReason::MissingTarget;
        wrapped(*this,
                automatic   ? "Applies automatically when its conditions are met."
                : available ? "Click or Enter to choose."
                            : commandMessage(e.reason),
                dx, dy + 111, dw, 2);
    }
    if (m.visibleCount > hudLayout::spellRows) {
        label("Up / Down", hudLayout::spellPrevious, muted, UiTextStyle::Small);
        std::snprintf(line, sizeof(line), "%zu-%zu / %zu", m.offset + 1,
                      std::min(m.visibleCount, m.offset + hudLayout::spellRows), m.visibleCount);
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
    if (c.action == PartyAction::Feature) {
        char uses[40]{};
        if (m.charges >= 0)
            usesLabel(uses, sizeof(uses), m.charges);
        std::snprintf(line, sizeof(line), "%s%s%s", featureAction(c.feature, h.combat.phase),
                      m.charges >= 0 ? " / " : "", uses);
    } else
        std::snprintf(line, sizeof(line), "%d mana / 1 action", m.cost);
    clipped(line, 53, 321, 229, hudInk::text, UiTextStyle::Small);
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
        : m.ground    ? "Click floor to aim; arrows cycle; Enter casts."
        : m.self      ? "Self / confirm to cast."
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
        hint = c.action == PartyAction::Feature ? "Waiting for the ability to finish."
                                                : "Waiting for the cast to finish.";
    clipped(hint, 53, 347, 229, muted, UiTextStyle::Small);
    box(hudLayout::spellCast, enabled && !m.single ? bright : edge);
    if (enabled && !m.single)
        fill({290, 312, 86, 15}, {65, 48, 18, 255});
    label(resolving                                 ? "RESOLVING"
          : m.single                                ? "Click a target"
          : c.feature == rules::Feature::SecondWind ? "Use Second Wind [Enter]"
          : c.action == PartyAction::Feature        ? "Use [Enter]"
          : m.kind == rules::SpellKind::Bless       ? "Cast Bless [Enter]"
                                                    : "Cast [Enter]",
          hudLayout::spellCast, enabled && !m.single ? bright : muted,
          c.feature == rules::Feature::SecondWind ? UiTextStyle::Small : UiTextStyle::Heading);
    box(hudLayout::spellBack);
    label("Back [B]", hudLayout::spellBack, resolving ? muted : gold, UiTextStyle::Small);
    box(hudLayout::spellCancel);
    label("Cancel [Esc]", hudLayout::spellCancel, resolving ? muted : gold, UiTextStyle::Small);
    if (resolving)
        return;
    if (m.area && !m.ground) {
        for (int i = 0; i < 3; ++i) {
            const UiRect tile{227 + i * 16, 321, 13, 7};
            fill(tile, m.cells[i] ? Color{115, 68, 24, 255} : background);
            border(tile, m.cells[i] ? gold : muted);
            if (!m.cells[i])
                text("x", tile.x + 4, tile.y, muted, UiTextStyle::Small);
        }
        triangle({248, 331}, {254, 331}, {251, 328}, gold);
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

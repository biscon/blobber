#include "ui/hud.hpp"
#include "ui/hud_painter.hpp"
namespace blobber {
using namespace hudInk;
void HudPainter::card(const HudCard& c, int i, bool aidTarget, bool inspected) const {
    const auto r = hudLayout::card(i), portrait = hudLayout::portrait(i);
    const auto accent = c.castCaster ? bright
                        : c.active   ? bright
                        : aidTarget  ? blue
                        : inspected  ? gold
                                     : edge;
    box(r, c.recentlyHit ? red : accent);
    if (c.castCaster || c.active || aidTarget)
        border({r.x + 1, r.y + 1, r.width - 2, r.height - 2}, accent);
    if (c.incomingShot)
        border({r.x + 2, r.y + 2, r.width - 4, r.height - 4}, blue);
    fill(portrait, background);
    picture(c.portrait, portrait,
            c.status == DisplayStatus::Dead || !c.castEligible ? Color{100, 100, 100, 255} : WHITE);
    border({portrait.x - 1, portrait.y - 1, portrait.width + 2, portrait.height + 2}, accent);
    if (c.castRecipient) {
        border({portrait.x, portrait.y, portrait.width, portrait.height}, teal);
        border({portrait.x + 1, portrait.y + 1, portrait.width - 2, portrait.height - 2},
               c.castHovered ? Color{174, 238, 230, 255} : teal);
        checkmark({portrait.x + portrait.width - 10, portrait.y + 1, 9, 9});
        fill({portrait.x + 2, portrait.y + portrait.height - 17, portrait.width - 4, 15}, {12, 26, 26, 245});
        clipped("Will receive", portrait.x + 3, portrait.y + portrait.height - 17, portrait.width - 6, teal,
                UiTextStyle::Small);
        clipped(c.castPreview, portrait.x + 3, portrait.y + portrait.height - 10, portrait.width - 6,
                hudInk::text, UiTextStyle::Small);
    } else if (c.castHovered)
        border(portrait, {170, 208, 204, 255});
    clipped(c.name, r.x + 56, r.y + 4, 68, c.active ? bright : gold, UiTextStyle::Display);
    clipped(c.className, r.x + 56, r.y + 16, 68, muted, UiTextStyle::Small);
    constexpr const char* slots[]{"FRONT LEFT", "FRONT RIGHT", "REAR LEFT", "REAR RIGHT"};
    clipped(slots[i], r.x + 56, r.y + 26, 68, muted, UiTextStyle::Small);
    health({r.x + 56, r.y + 36, 68, 5}, c.maxHp ? float(c.hp) / c.maxHp : 0);
    char line[64];
    std::snprintf(line, sizeof(line), "HP %d/%d", c.hp, c.maxHp);
    text(line, r.x + 56, r.y + 43);
    const char* status = c.status == DisplayStatus::Dead          ? "DEAD"
                         : c.status == DisplayStatus::Stable      ? "STABLE"
                         : c.status == DisplayStatus::Unconscious ? "DYING"
                         : c.active                               ? (c.selecting ? "SELECTED" : "YOUR TURN")
                         : c.defending                            ? "DEFENDING"
                                                                  : "";
    if (c.selecting && c.cooldownMs && c.status == DisplayStatus::Ready)
        std::snprintf(line, sizeof(line), "Ready %.1fs", c.cooldownMs / 1000.0);
    else if ((c.spent || c.cooldownMs) && !c.defending && c.status == DisplayStatus::Ready)
        std::snprintf(line, sizeof(line), "ACTION SPENT");
    else if (c.hasDeathSaves)
        std::snprintf(line, sizeof(line), "Saves %u+ %u-", unsigned(c.successes), unsigned(c.failures));
    else if (c.hasMana && c.status == DisplayStatus::Ready)
        std::snprintf(line, sizeof(line), "Mana %d/%d", c.mana, c.maxMana);
    else
        std::snprintf(line, sizeof(line), "%s", status);
    if (c.active)
        fill({r.x + 54, r.y + 54, 72, 10}, {49, 39, 20, 255});
    clipped(line, r.x + 56, r.y + 54, 68,
            c.active                           ? bright
            : c.status == DisplayStatus::Ready ? muted
                                               : red,
            UiTextStyle::Small);
    if (c.concentrating) {
        fill({portrait.x, portrait.y, 6, 6}, blue);
    }
    box(hudLayout::detail(i));
    label("i", hudLayout::detail(i), gold, UiTextStyle::Small);
}
void HudPainter::log(const HudSnapshot& h, bool expanded) const {
    const auto r = expanded            ? hudLayout::expandedLog
                   : h.magic.casting() ? hudLayout::castingLog
                                       : hudLayout::log;
    box(r, expanded ? gold : edge);
    text(expanded ? "COMBAT HISTORY" : "COMBAT LOG", r.x + 8, r.y + (expanded ? 2 : 4), gold,
         UiTextStyle::Display);
    if (expanded) {
        box(hudLayout::closeOverlay);
        label("X", hudLayout::closeOverlay, gold);
    } else {
        box(hudLayout::logExpand);
        label("+", hudLayout::logExpand, gold);
    }
    if (h.logOffset && !expanded)
        label("LATEST", hudLayout::logLatest, blue, UiTextStyle::Small);
    const auto columns = expanded            ? hudLayout::expandedLogColumns
                         : h.magic.casting() ? hudLayout::castingLogColumns
                                             : hudLayout::logColumns;
    const auto count = h.log.lineCount(columns);
    const auto end = count - std::min(h.logOffset, count);
    const int rows = expanded ? 21 : 4;
    const auto begin = end > static_cast<std::size_t>(rows) ? end - rows : 0;
    char line[CombatLog::messageBytes];
    for (std::size_t n = begin; n < end; ++n)
        if (h.log.line(n, columns, line, sizeof(line)))
            clipped(line, r.x + 8, r.y + (expanded ? 13 : 15) + int(n - begin) * (expanded ? 10 : 9),
                    r.width - 16, n + 1 == count ? gold : hudInk::text, UiTextStyle::Log);
}
void drawUI(const MovementState& state, bool debug, const Map& map, const HudSnapshot* h,
            const UiFonts* fonts, const UiArt* art) {
    const HudPainter p(fonts, art);
    p.box(hudLayout::header, gold);
    const auto headerText = [&](const char* value, int x, Color color, UiTextStyle style) {
        const auto r = hudLayout::header;
        p.text(value, x, fonts ? fonts->centeredY(value, float(r.y), float(r.height), style) : float(r.y),
               color, style);
    };
    headerText("STONE & SHADOW", 12, gold, UiTextStyle::Display);
    if (h && h->demo)
        headerText("UI DEMO / SAMPLE", 175, blue, UiTextStyle::Small);
    char line[96];
    std::snprintf(line, sizeof(line), ".  .  .  %s  .  .  .", headingName(state.player.heading));
    headerText(line, 245, gold, UiTextStyle::Small);
    if (!h || !h->demo)
        headerText("Depth I", 449, gold, UiTextStyle::Small);
    p.gear(hudLayout::gear);
    p.border({viewX - 1, viewY - 1, viewWidth + 2, viewHeight + 2}, edge);
    p.box(hudLayout::actions);
    if (!h)
        return;
    for (int i = 0; i < 4; ++i) {
        bool eligible = false;
        for (std::size_t j = 0; j < h->allyCount; ++j)
            eligible |= h->allies[j].index == h->cards[i].id.index;
        p.card(h->cards[i], i, h->selection.action == PartyAction::Aid && eligible,
               h->inspected && h->inspected->index == h->cards[i].id.index);
    }
    if (h->magic.casting()) {
        p.spellTargets(*h);
    } else {
        const char* actor = "EXPLORATION";
        for (const auto& c : h->cards)
            if ((h->combat.activeCharacter && c.id.index == h->combat.activeCharacter->index) ||
                (!h->production && c.id.index == h->selection.character.index))
                actor = c.name;
        if (h->production && h->combat.activeEnemy)
            actor = "ENEMY TURN";
        if (h->combat.phase == CombatPhase::Facing && h->combat.facing) {
            constexpr const char* labels[]{"FACING NORTH", "FACING EAST", "FACING SOUTH", "FACING WEST"};
            actor = labels[static_cast<unsigned>(*h->combat.facing)];
        } else if (h->combat.phase == CombatPhase::Approach)
            actor = "RANGED APPROACH";
        else if (h->combat.phase == CombatPhase::Contact)
            actor = "CONTACT / WAIT";
        else if (h->combat.phase == CombatPhase::RoundEnd)
            actor = "ROUND END";
        else if (h->combat.phase == CombatPhase::Victory)
            actor = "VICTORY";
        else if (h->combat.phase == CombatPhase::Defeat)
            actor = "DEFEAT";
        std::snprintf(line, sizeof(line), "%s%s", actor,
                      h->combat.resolvingAction  ? "  /  RESOLVING"
                      : h->combat.awaitingAction ? "  /  CHOOSE AN ACTION"
                                                 : "");
        p.clipped(line, 12, 308, 284, gold, UiTextStyle::Display);
        for (int i = 0; i < 6; ++i) {
            const auto r = hudLayout::action(i);
            const auto action = dockActions[i];
            const auto reason = h->actionAvailability[static_cast<std::size_t>(action)];
            const bool available =
                (i == 0 || i == 1 || i == 2 || i == 3 || i == 5) && !h->suspended && !h->modal() &&
                (i == 3 || reason == CommandReason::Ready || reason == CommandReason::MissingTarget ||
                 reason == CommandReason::InvalidAlly || reason == CommandReason::PreviewAccepted);
            const bool selected = action == h->selection.action;
            p.box(r, selected ? bright : edge);
            p.picture(5 + i, {r.x + 2, r.y + 2, r.width - 4, r.height - 4},
                      available ? WHITE : Color{85, 90, 88, 255});
            if (selected)
                p.border({r.x + 1, r.y + 1, r.width - 2, r.height - 2}, gold);
            p.fill({r.x + 1, r.y + r.height - 9, 8, 8}, background);
            char key[]{char('1' + i), 0};
            p.text(key, r.x + 2, r.y + r.height - 9, available ? gold : muted, UiTextStyle::Small);
        }
        const char* hint =
            h->combat.phase == CombatPhase::Facing    ? "Turning to the next group. Your action is preserved."
            : h->combat.phase == CombatPhase::Victory ? "Continue to explore with your surviving party."
            : h->combat.phase == CombatPhase::Defeat  ? "Restart to begin a fresh session."
            : h->selection.action == PartyAction::Aid ? "Click a dying ally's portrait. Right-click cancels."
            : h->production && h->combat.awaitingAction              ? "Click a reachable enemy to attack."
            : h->production && h->combat.phase == CombatPhase::Turns ? "Waiting for the active participant."
                                                                     : "WASD move / Q E turn / F1 map";
        p.clipped(hint, 12, 349, 284, muted, UiTextStyle::Small);
    }
    if (h->production && (h->combat.knownDirections || h->combat.pendingCount)) {
        char threats[96];
        char dirs[5]{};
        int n = 0;
        for (int d = 0; d < 4; ++d)
            if (h->combat.knownDirections & (1u << d))
                dirs[n++] = "NESW"[d];
        std::snprintf(threats, sizeof(threats), "Threats %s%s", dirs,
                      h->combat.pendingCount ? " / arrivals" : "");
        p.text(threats, viewX + 6, viewY + 5, gold, UiTextStyle::Small);
    }
    p.log(*h, false);
    (void)debug;
    (void)map;
}
void drawMapOverlay(const MovementState& state, const Map& map, const UiFonts* fonts) {
    const HudPainter p(fonts, nullptr);
    {
        p.overlayBox({12, 32, 176, 242}, gold);
        p.text("SURVEY / F1 CLOSE", 22, 40, gold, UiTextStyle::Display);
        const float scale = std::min(152.0f / map.width(), 200.0f / map.height());
        {
            for (int y = 0; y < map.height(); ++y)
                for (int x = 0; x < map.width(); ++x)
                    if (map.solid({x, y}))
                        p.rectangle({22 + x * scale, 58 + y * scale, scale, scale}, edge);
            p.rectangle({22 + state.player.cell.x * scale, 58 + state.player.cell.y * scale, scale, scale},
                        bright);
        }
    }
}
} // namespace blobber

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
        fill({portrait.x + 2, portrait.y + portrait.height - 25, portrait.width - 4, 15}, {12, 26, 26, 245});
        clipped(c.castRisk ? "At risk" : "Will receive", portrait.x + 3, portrait.y + portrait.height - 25,
                portrait.width - 6, teal, UiTextStyle::Small);
        clipped(c.castPreview, portrait.x + 3, portrait.y + portrait.height - 18, portrait.width - 6,
                hudInk::text, UiTextStyle::Small);
    } else if (c.castHovered)
        border(portrait, {170, 208, 204, 255});
    clipped(c.name, r.x + 56, r.y + 4, 68, c.active ? bright : gold, UiTextStyle::Display);
    clipped(c.className, r.x + 56, r.y + 16, 68, muted, UiTextStyle::Small);
    constexpr const char* slots[]{"FRONT LEFT", "FRONT RIGHT", "REAR LEFT", "REAR RIGHT"};
    clipped(slots[i], r.x + 56, r.y + 26, 68, muted, UiTextStyle::Small);
    health({r.x + 56, r.y + 36, 68, 5}, c.maxHp ? float(c.hp) / c.maxHp : 0);
    char line[64];
    if (c.temporaryHp)
        std::snprintf(line, sizeof(line), "HP %d/%d +%d", c.hp, c.maxHp, c.temporaryHp);
    else
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
    if (c.effects.count) {
        fill({portrait.x, portrait.y + portrait.height - 9, portrait.width, 9}, background);
        for (std::size_t n = 0; n < std::min(c.effects.count, std::size_t(hudLayout::statusSlots)); ++n) {
            const auto r = hudLayout::status(i, int(n));
            if (n == 5 && c.effects.count > 6) {
                char count[24];
                std::snprintf(count, sizeof(count), "+%zu", c.effects.count - 5);
                centered(count, r, gold, UiTextStyle::Small);
            } else {
                const auto& e = c.effects.entries[n];
                picture(e.effect == EffectKind::Count ? UiArt::spellIcon(e.spell)
                                                      : UiArt::effectIcon(e.effect),
                        r);
                border(r, e.harmful ? red : teal);
            }
        }
    }
    if (c.levelEligible && !c.castRecipient) {
        fill({portrait.x + 1, portrait.y + 1, 33, 10}, background);
        text("LEVEL UP", portrait.x + 2, portrait.y + 2, gold, UiTextStyle::Small);
    }
    if (c.concentration) {
        const auto badge = hudLayout::concentration(i);
        box(badge, gold);
        picture(UiArt::spellIcon(rules::SpellKind::Count, rules::Feature::DismissConcentration),
                {badge.x + 1, badge.y + 1, badge.width - 2, badge.height - 2});
    }
    box(hudLayout::detail(i));
    label("i", hudLayout::detail(i), gold, UiTextStyle::Small);
}
void HudPainter::log(const HudSnapshot& h, bool expanded) const {
    const auto r = expanded                                           ? hudLayout::expandedLog
                   : (h.magic.casting() || h.inventory.itemTargeting) ? hudLayout::castingLog
                                                                      : hudLayout::log;
    box(r, expanded ? gold : edge);
    text(expanded        ? "COMBAT HISTORY"
         : r.width < 150 ? "LOG"
                         : "COMBAT LOG",
         r.x + 8, r.y + (expanded ? 2 : 4), gold, UiTextStyle::Display);
    if (expanded) {
        box(hudLayout::closeOverlay);
        label("X", hudLayout::closeOverlay, gold);
    } else {
        box(hudLayout::logExpand);
        label("+", hudLayout::logExpand, gold);
    }
    if (h.logOffset && !expanded)
        label("LATEST", hudLayout::logLatest, blue, UiTextStyle::Small);
    const auto columns = expanded                                           ? hudLayout::expandedLogColumns
                         : (h.magic.casting() || h.inventory.itemTargeting) ? hudLayout::castingLogColumns
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
    if (!h || !h->demo) {
        const auto depth = h && h->floor ? h->floor->definition().depth : 1;
        headerText(h && h->arena ? "Arena"
                   : depth == 2  ? "Depth II"
                   : depth == 3  ? "Depth III"
                                 : "Depth I",
                   449, gold, UiTextStyle::Small);
    }
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
               (h->inventory.open && h->inventory.character.index == h->cards[i].id.index) ||
                   (h->loot.open && h->loot.recipient.index == h->cards[i].id.index));
    }
    p.picture(UiArt::restIcon, hudLayout::restButton, h->restAvailable ? WHITE : Color{110, 110, 110, 125});
    if (h->inventory.itemTargeting) {
        p.itemTargets(*h);
    } else if (h->magic.casting()) {
        p.spellTargets(*h);
    } else {
        const char* actor = h->camp.open ? "CAMP / PAUSED" : "EXPLORATION";
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
            actor = h->arena && h->arenaComplete ? "ARENA CLEARED" : "VICTORY";
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
                (i >= 0 && i < 6) && !h->suspended && !h->modal() &&
                (i == 3 || (i == 4 && h->production) || reason == CommandReason::Ready ||
                 reason == CommandReason::MissingTarget || reason == CommandReason::InvalidAlly ||
                 reason == CommandReason::PreviewAccepted);
            const bool selected = action == h->selection.action;
            p.box(r, selected ? bright : edge);
            const unsigned icon = i == 3 && !h->magic.spellcaster ? UiArt::classFeaturesIcon : 5 + i;
            p.picture(icon, {r.x + 2, r.y + 2, r.width - 4, r.height - 4},
                      available ? WHITE : Color{85, 90, 88, 255});
            if (selected)
                p.border({r.x + 1, r.y + 1, r.width - 2, r.height - 2}, gold);
            p.fill({r.x + 1, r.y + r.height - 9, 8, 8}, background);
            char key[]{char('1' + i), 0};
            p.text(key, r.x + 2, r.y + r.height - 9, available ? gold : muted, UiTextStyle::Small);
        }
        const char* hint =
            h->combat.phase == CombatPhase::Facing ? "Turning to the next group. Your action is preserved."
            : h->combat.phase == CombatPhase::Victory
                ? (h->arena && h->arenaComplete ? "Rematch or edit your arena setup."
                                                : "Continue to explore with your surviving party.")
            : h->combat.phase == CombatPhase::Defeat  ? "Restart to begin a fresh session."
            : h->selection.action == PartyAction::Aid ? "Click a dying ally's portrait. Right-click cancels."
            : h->production && h->combat.awaitingAction ? "Click a reachable enemy to attack."
            : h->production && h->combat.phase == CombatPhase::Turns
                ? "Waiting for the active participant."
                : "WASD move / Q E turn / M map / R camp";
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
    if (h->fixturePrompt[0]) {
        p.fill({viewX + 4, viewY + viewHeight - 17, viewWidth - 8, 15}, background);
        p.clipped(h->fixturePrompt.data(), viewX + 8, viewY + viewHeight - 15, viewWidth - 16, gold,
                  UiTextStyle::Small);
    }
    p.log(*h, false);
    (void)debug;
    (void)map;
}
void drawMapOverlay(const MovementState& state, const Map& map, const UiFonts* fonts,
                    const generation::DiagnosticView* generation, const HudSnapshot* hud, bool reveal) {
    const HudPainter p(fonts, nullptr);
    const auto* floor = hud ? hud->floor : nullptr;
    const bool player = floor && floor->definition().discovery && !reveal;
    p.overlayBox({12, 32, 610, 252}, gold);
    p.text(player ? "DISCOVERED MAP / M CLOSE" : "DIAGNOSTIC FULL MAP / M CLOSE", 22, 40, gold,
           UiTextStyle::Display);
    const float scale = std::min(220.f / map.width(), 210.f / map.height());
    const float ox = 25, oy = 65;
    for (int y = 0; y < map.height(); ++y)
        for (int x = 0; x < map.width(); ++x) {
            const Cell c{x, y};
            Color ink = background;
            if (!player || floor->discovered(c))
                ink = map.solid(c) ? edge : Color{64, 79, 73, 255};
            p.rectangle({ox + x * scale, oy + y * scale, scale - .4f, scale - .4f}, ink);
        }
    if (floor)
        for (unsigned i = 0; i < floor->definition().count; ++i) {
            if (player && !floor->known(i))
                continue;
            const auto& f = floor->definition().entries[i];
            const bool done = player ? floor->rememberedComplete(i) : floor->completed(i);
            if (f.kind == FixtureKind::Key && done)
                continue;
            const char* mark = f.kind == FixtureKind::GuardianGate ? (done ? "/" : "G")
                               : f.kind == FixtureKind::Door       ? (done ? "/" : "+")
                               : f.kind == FixtureKind::Key        ? "K"
                               : f.kind == FixtureKind::Upstairs   ? "U"
                               : f.kind == FixtureKind::Downstairs ? "D"
                                                                   : "O";
            p.text(mark, ox + f.cell.x * scale, oy + f.cell.y * scale - 1, done ? teal : bright,
                   UiTextStyle::Small);
        }
    const auto pos = state.player.cell, dir = forward(state.player.heading);
    const Vector2 center{ox + (pos.x + .5f) * scale, oy + (pos.y + .5f) * scale};
    p.rectangle({center.x - 1.8f, center.y - 1.8f, 3.6f, 3.6f}, teal);
    p.line(center, {center.x + dir.x * scale, center.y + dir.y * scale}, teal, 3);
    if (!player && generation) {
        for (unsigned i = 0; i < generation->lines.size(); ++i)
            p.clipped(generation->lines[i].data(), 260, 68 + i * 18, 345, text, UiTextStyle::Label);
    } else {
        p.text("Your explored routes", 270, 70, gold, UiTextStyle::Display);
        p.text("Dark space has not been seen.", 270, 96, muted);
        p.text("+ Closed door    / Open door", 270, 119, text);
        p.text("K Key    U Up    D Down    O Objective", 270, 140, text, UiTextStyle::Small);
        p.text("Quest keys", 270, 170, gold);
        if (hud && (hud->questKeys & (1u << 2)))
            p.text("Bronze seal key", 270, 190, text);
        else if (hud && (hud->questKeys & (1u << 3)))
            p.text("Silver seal key", 270, 190, text);
        else
            p.text("None collected", 270, 190, muted);
        p.text("Stairs and objectives: travel not yet available.", 270, 218, muted, UiTextStyle::Small);
    }
    if (hud && hud->development) {
        p.box(hudLayout::diagnosticMap);
        p.label(reveal ? "Hide full map - diagnostic" : "Reveal full map - diagnostic",
                hudLayout::diagnosticMap, gold, UiTextStyle::Small);
    }
}
} // namespace blobber

#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "ui/interaction.hpp"
#include <cstring>
#include <iostream>
using namespace blobber;
namespace {
constexpr Player spawn{{3, 3}, Heading::North};
void ready(GameSession& s) {
    s.update(0, false);
    for (int i = 0; i < 80; ++i) {
        const auto& combat = s.combat().snapshot();
        if (combat.awaitingAction && combat.activeCharacter->index == 0)
            return;
        if (combat.awaitingAction)
            s.submit(turnCommand(s));
        else
            s.update(.45, false);
    }
    check(false, "Fixture reaches Alden's turn");
}
void refresh(CommandController& ui, GameSession& s) {
    ui.refresh(allCombatTargets(s), false, false, true);
}
UiInput click(UiRect r) {
    UiInput input;
    input.pointer = ArtPoint{float(r.x + 3), float(r.y + 3)};
    input.pressed[static_cast<std::size_t>(UiKey::Click)] = true;
    return input;
}
void meleeRangeFeedback() {
    const auto map = combatMap();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    NpcWorld places(map, spawn.cell, {"goblin"},
                    {{{0}, {3, 2}, {0, 1}, Heading::South}, {{0}, {3, 1}, {0, 1}, Heading::South}});
    GameSession s(map, catalog, places, 5, spawn, false);
    ready(s);
    CommandController ui(s);
    refresh(ui, s);
    const auto before = s.combat().snapshot();
    UiInput hover;
    hover.pointer = ArtPoint{210, 140};
    routeHudInput(ui, hover, s.actors().id(1));
    check(ui.snapshot().hover.reason == CommandReason::OutOfRange,
          "Living distant melee target previews out of range");
    routeHudInput(ui, click(hudLayout::viewport), s.actors().id(1));
    check(s.combat().snapshot().turn == before.turn && s.combat().snapshot().awaitingAction &&
              s.combat().snapshot().cooldownMs == before.cooldownMs,
          "Out-of-range melee click preserves action and readiness");
    bool found = false;
    for (std::size_t i = 0; i < ui.snapshot().log.size(); ++i)
        found |= std::strstr(ui.snapshot().log.newest(i), "out of range") != nullptr;
    check(found, "Rejected melee click logs a range explanation");
    auto command = turnCommand(s, PartyAction::Melee);
    command.target = s.actors().id(1);
    s.actors().damage(*command.target, 1000, s.timeMs());
    check(s.submit(command).reason == CommandReason::InvalidTarget,
          "Actually dead distant target still reports invalid target");
    command.target = s.actors().id(0);
    check(s.submit(command).event == CombatEvent::AttackStarted,
          "Valid melee remains available after rejection");
}
void directActions() {
    const auto map = combatMap();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    const auto placements = combatPlacements(map);
    GameSession s(map, catalog, placements, 5, spawn);
    ready(s);
    CommandController ui(s);
    refresh(ui, s);
    const auto initial = ui.snapshot().selection;
    const auto turn = s.combat().snapshot().turn, logs = ui.snapshot().log.size();
    UiInput hover;
    hover.pointer = ArtPoint{210, 140};
    routeHudInput(ui, hover, s.actors().id(0));
    check(ui.snapshot().hover.reason == CommandReason::FrontProtected && !ui.snapshot().selection.target &&
              ui.snapshot().log.size() == logs && s.combat().snapshot().turn == turn,
          "Unreachable hover previews without selection, logs or turn mutation");
    check(std::strstr(ui.snapshot().hover.lines[1].data(), "damage"), "Hover uses equipped weapon preview");
    auto input = click(hudLayout::viewport);
    routeHudInput(ui, input, s.actors().id(0));
    check(s.combat().snapshot().turn == turn, "Unreachable click spends no turn");
    for (int i = 3; i < 5; ++i)
        routeHudInput(ui, click(hudLayout::action(i)));
    check(s.combat().snapshot().turn == turn && ui.snapshot().selection.action == PartyAction::Melee,
          "Deferred action icons cannot arm or commit commands");
    routeHudInput(ui, input, s.actors().id(2), false);
    check(s.combat().snapshot().turn == turn, "Click sampled before new turn cannot commit");
    routeHudInput(ui, input, s.actors().id(2), true);
    check(s.combat().snapshot().resolvingAction && !s.combat().snapshot().awaitingAction &&
              ui.snapshot().selection.action == PartyAction::Melee && !ui.snapshot().selection.target,
          "One reachable click attacks then clears targeting");
    const auto after = s.combat().snapshot().turn;
    routeHudInput(ui, input, s.actors().id(2), true);
    check(s.combat().snapshot().turn == after, "Repeated stale UI state cannot execute another turn");
    ready(s);
    refresh(ui, s);
    const auto defendTurn = s.combat().snapshot().turn;
    UiInput shield;
    shield.pressed[static_cast<std::size_t>(UiKey::Two)] = true;
    routeHudInput(ui, shield);
    check(s.combat().snapshot().defending[0] && s.combat().snapshot().turn != defendTurn,
          "Defend shortcut executes immediately");
    check(initial.character.index == 0, "Fixture active character");
}
void keyboardHover() {
    const auto map = combatMap();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    const auto placements = combatPlacements(map);
    GameSession s(map, catalog, placements, 5, spawn);
    ready(s);
    CommandController ui(s);
    refresh(ui, s);
    UiInput input;
    input.pointer = ArtPoint{10, 10};
    routeHudInput(ui, input);
    input.pressed[static_cast<std::size_t>(UiKey::Right)] = true;
    routeHudInput(ui, input);
    check(ui.snapshot().selection.target && ui.snapshot().hover.kind == HoverKind::Enemy,
          "Keyboard target exposes the same reachability tooltip");
    input.pressed = {};
    routeHudInput(ui, input);
    check(ui.snapshot().hover.kind == HoverKind::Enemy,
          "Stationary pointer does not erase keyboard targeting feedback");
    input.pointer = ArtPoint{11, 10};
    routeHudInput(ui, input);
    check(ui.snapshot().hover.kind == HoverKind::None, "Pointer movement resumes pointer-driven hover");
}
void overlaysAndAid() {
    const auto map = combatMap();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    const auto placements = combatPlacements(map);
    GameSession s(map, catalog, placements, 5, spawn);
    ready(s);
    CommandController ui(s);
    refresh(ui, s);
    const auto turn = s.combat().snapshot().turn, clock = s.timeMs();
    routeHudInput(ui, click(hudLayout::card(3)));
    check(ui.snapshot().inspected && ui.snapshot().inspected->index == 3 &&
              ui.snapshot().selection.character.index == 0,
          "Inspection never replaces acting character");
    s.update(10, ui.snapshot().modal());
    check(s.timeMs() == clock, "Details pause gameplay time");
    routeHudInput(ui, click(hudLayout::action(1)), s.actors().id(2));
    check(s.combat().snapshot().turn == turn, "Overlay prevents actions through its entire surface");
    routeHudInput(ui, click(hudLayout::detailsClose), s.actors().id(2));
    check(!ui.snapshot().modal() && s.combat().snapshot().turn == turn, "Closing overlay cannot attack");
    s.party().damage({2}, s.party().character({2})->hitPoints);
    refresh(ui, s);
    UiInput aid;
    aid.pressed[static_cast<std::size_t>(UiKey::Six)] = true;
    routeHudInput(ui, aid);
    check(ui.snapshot().selection.action == PartyAction::Aid, "Aid shortcut arms friendly targeting");
    routeHudInput(ui, click(hudLayout::card(1)));
    check(s.combat().snapshot().turn == turn && !ui.snapshot().inspected,
          "Invalid ally never opens details or spends action");
    routeHudInput(ui, click(hudLayout::card(2)));
    check(s.combat().snapshot().turn != turn && ui.snapshot().selection.action == PartyAction::Melee,
          "Valid ally portrait commits Aid and resets mode");
    refresh(ui, s);
    ui.selectAction(PartyAction::Aid);
    UiInput cancel;
    cancel.pressed[static_cast<std::size_t>(UiKey::RightClick)] = true;
    routeHudInput(ui, cancel);
    check(ui.snapshot().selection.action == PartyAction::Melee && !ui.snapshot().modal(),
          "Right-click cancels targeting");
    cancel.pressed = {};
    cancel.pressed[static_cast<std::size_t>(UiKey::Cancel)] = true;
    routeHudInput(ui, cancel);
    check(ui.snapshot().menuOpen, "Unarmed Escape opens pause menu instead of quitting");
    routeHudInput(ui, click(hudLayout::resume));
    check(!ui.snapshot().modal(), "Resume closes pause menu");
    routeHudInput(ui, click(hudLayout::logExpand));
    check(ui.snapshot().logExpanded, "History opens explicitly");
    routeHudInput(ui, cancel);
    check(!ui.snapshot().modal(), "Escape closes history");
    ui.toggleMenu();
    routeHudInput(ui, click(hudLayout::quit));
    check(ui.snapshot().quitRequested, "Quit requires explicit menu action");
}
void initiative() {
    const auto map = combatMap();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    const auto placements = combatPlacements(map);
    for (unsigned seed = 0; seed < 20; ++seed) {
        GameSession s(map, catalog, placements, seed, spawn);
        s.update(0, false);
        for (int i = 0; i < 30; ++i) {
            CombatSnapshot preview = s.combat().snapshot();
            s.combat().previewTurns(preview);
            check(preview.upcomingCount > 0 && preview.upcomingCount <= 8,
                  "Bounded nonempty remaining initiative");
            const auto current = preview.upcoming[0];
            check(
                (current.character && preview.activeCharacter &&
                 current.character->index == preview.activeCharacter->index) ||
                    (current.enemy && preview.activeEnemy && sameActor(*current.enemy, *preview.activeEnemy)),
                "First visible initiative participant is actual active actor");
            if (preview.awaitingAction)
                s.submit(turnCommand(s));
            else
                s.update(.45, false);
            // Presentation holds the old participant; compare preview only when
            // the authoritative turn actually advances.
            if (s.combat().snapshot().turn == preview.turn)
                continue;
            if (preview.upcomingCount > 1) {
                const auto next = preview.upcoming[1];
                const auto actual = s.combat().snapshot();
                check((next.character && actual.activeCharacter &&
                       next.character->index == actual.activeCharacter->index) ||
                          (next.enemy && actual.activeEnemy && sameActor(*next.enemy, *actual.activeEnemy)),
                      "Preview predicts actual execution order without rerolling initiative");
            } else {
                if (s.combat().snapshot().phase == CombatPhase::RoundEnd)
                    s.update(.45, false);
                check(s.combat().snapshot().round == preview.round + 1, "Queue refills only at next round");
            }
        }
    }
    GameSession s(map, catalog, placements, 5, spawn);
    ready(s);
    s.party().damage({1}, s.party().character({1})->hitPoints);
    s.actors().damage(s.actors().id(2), 1000, s.timeMs());
    CombatSnapshot preview = s.combat().snapshot();
    s.combat().previewTurns(preview);
    bool dead = false, blocked = false;
    for (std::size_t i = 0; i < preview.upcomingCount; ++i)
        if (preview.upcoming[i].enemy) {
            dead |= preview.upcoming[i].enemy->slot == 2;
            blocked |= preview.upcoming[i].enemy->slot == 1;
        }
    check(!dead && !blocked, "Preview excludes dead enemies and rear enemies whose gap is claimed earlier");
    s.actors().damage(s.actors().id(0), 1000, s.timeMs());
    s.combat().previewTurns(preview);
    bool diagonal = false;
    for (std::size_t i = 0; i < preview.upcomingCount; ++i)
        diagonal |= preview.upcoming[i].enemy && preview.upcoming[i].enemy->slot == 1;
    check(diagonal, "Preview includes protected rear actor when the diagonal vacancy remains available");
}
void logs() {
    CombatLog log;
    log.append("one two three four");
    log.append("five six");
    char line[192];
    check(log.lineCount(10) == 3 && log.line(0, 10, line, sizeof(line)) && std::strcmp(line, "one two") == 0,
          "Word wrapping maintains readable first line");
    check(log.line(1, 10, line, sizeof(line)) && std::strcmp(line, "three four") == 0,
          "Word wrapping respects exact boundary");
    check(!log.line(99, 10, line, sizeof(line)) && !line[0] && log.lineCount(0) == 0,
          "Invalid line queries bounded");
    CombatLog boundary;
    boundary.append("three four five");
    check(boundary.lineCount(10) == 2 && boundary.line(0, 10, line, sizeof(line)) &&
              std::strcmp(line, "three four") == 0,
          "A word ending exactly at the wrap boundary stays on that line");
    const auto map = combatMap();
    const auto catalog = combatCatalog();
    const auto placements = combatPlacements(map);
    GameSession s(map, catalog, placements, 5, spawn);
    CommandController ui(s);
    CombatOutcome out;
    out.event = CombatEvent::Defend;
    out.character = {0};
    for (int i = 0; i < 8; ++i)
        ui.record(out);
    ui.scrollLog(3);
    const auto count = ui.snapshot().log.lineCount(hudLayout::logColumns);
    const auto offset = ui.snapshot().logOffset;
    ui.snapshot().log.line(count - offset - 1, hudLayout::logColumns, line, sizeof(line));
    char previous[192];
    std::strcpy(previous, line);
    ui.record(out);
    const auto nextCount = ui.snapshot().log.lineCount(hudLayout::logColumns);
    ui.snapshot().log.line(nextCount - ui.snapshot().logOffset - 1, hudLayout::logColumns, line,
                           sizeof(line));
    check(std::strcmp(previous, line) == 0 && ui.snapshot().logOffset > offset,
          "New messages preserve scrolled reading position");
    ui.latestLog();
    ui.record(out);
    check(!ui.snapshot().logOffset, "Latest view follows new messages");
}
} // namespace
int main() {
    try {
        meleeRangeFeedback();
        directActions();
        keyboardHover();
        overlaysAndAid();
        initiative();
        logs();
        std::cout << "Dungeon UI checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

#include "game/tactical_navigation.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include <iostream>

using namespace blobber;
namespace {
Map room(unsigned size = 19) {
    std::vector<std::string> rows(size, std::string(size, '.'));
    rows.front() = rows.back() = std::string(size, '#');
    for (auto& row : rows)
        row.front() = row.back() = '#';
    return Map(rows);
}
struct Fixture {
    Map map = room();
    rules::Catalog catalog;
    Player pose{{9, 16}, Heading::North};
    NpcWorld places;
    Party party;
    ActorWorld actors;
    rules::DiceRng rng{5};
    NpcAiController ai;
    EffectController effects;
    CombatController combat;
    CombatEvents events;
    std::uint64_t now = 0;
    Fixture(int speed, Cell start = {9, 3}, Cell sub = {1, 0})
        : catalog(combatCatalog(1000, -100, {1, 4, -100}, 15, speed)),
          places(map, pose.cell, {"goblin"},
                 {{{0}, {9, 15}, {0, 1}, Heading::South}, {{0}, start, sub, Heading::South}}),
          party(catalog), actors(map, catalog, places), ai(map, actors, 5), effects(party, actors, rng),
          combat(map, party, actors, rng, ai, effects) {
        ai.bindEffects(&effects);
        effects.partyCell(pose.cell);
        combat.enableRoaming(false);
        ai.alert(actors.id(0), pose.cell);
        ai.alert(actors.id(1), pose.cell);
        check(combat.contact(pose, now), "Fixture starts a genuine contact encounter");
    }
    void tick() {
        events.clear();
        const auto before = now;
        now += 100;
        combat.resolveImpacts(pose, now, events, now - before);
        events.step = combat.update(pose, false, now, events);
        events.append(events.step);
        const auto& state = combat.snapshot();
        if (state.awaitingAction) {
            PartyCommand command;
            command.world = state.world;
            command.turn = state.turn;
            command.character = *state.activeCharacter;
            command.action = PartyAction::Defend;
            combat.submit(command, pose, now, events);
        }
    }
    struct Turn {
        unsigned moves = 0, attacks = 0, dashes = 0, movesBeforeDash = 0, groundSaves = 0;
    };
    Turn firstTurn() {
        Turn result;
        bool began = false;
        std::uint64_t token = 0, clock = 0, round = 0;
        for (unsigned i = 0; i < 1500; ++i) {
            const auto before = combat.snapshot();
            if (before.activeEnemy && before.activeEnemy->slot == 1 && !began) {
                began = true;
                token = before.turn;
                clock = before.gameplayMs;
                round = before.round;
            }
            allocations = 0;
            watching = true;
            tick();
            watching = false;
            check(!allocations, "Movement, search, arrival and attack allocate no engine C++ storage");
            for (const auto& event : events)
                if (event.enemy.world == actors.id(1).world && event.enemy.slot == 1) {
                    result.moves += event.event == CombatEvent::Move;
                    result.attacks += event.event == CombatEvent::AttackStarted;
                    result.groundSaves +=
                        event.event == CombatEvent::SpellSave && event.spell == rules::SpellKind::Grease;
                    if (event.event == CombatEvent::Dash) {
                        result.movesBeforeDash = result.moves;
                        ++result.dashes;
                    }
                }
            const auto& after = combat.snapshot();
            if (began && after.activeEnemy && after.activeEnemy->slot == 1) {
                check(after.turn == token && after.gameplayMs == clock && after.round == round,
                      "All substeps retain one turn token and do not charge extra rounds");
            } else if (began)
                return result;
        }
        check(false, "Movement turn finishes in a bounded number of updates");
        return result;
    }
};
void budgetsAndDash() {
    {
        Fixture f(1);
        const auto turn = f.firstTurn();
        check(!turn.moves && !turn.dashes && !turn.attacks,
              "Insufficient ordinary plus Dash movement never spends an action without progress");
    }
    for (int speed : {0, 20, 25, 30, 40}) {
        Fixture f(speed);
        const auto start = f.actors.actor(f.actors.id(1))->npc;
        const auto turn = f.firstTurn();
        check(turn.moves == unsigned(speed / 5 * 2) && turn.dashes == unsigned(speed > 0) && !turn.attacks,
              "Unreachable-in-one-turn enemy spends exactly Speed plus one Dash and cannot attack");
        check(turn.movesBeforeDash == unsigned(speed / 5),
              "Normal movement allowance is exactly four/six/eight steps before Dash");
        const auto end = f.actors.actor(f.actors.id(1))->npc;
        check(std::abs(end.cell.x * 2 + end.subcell.x - (start.cell.x * 2 + start.subcell.x)) +
                      std::abs(end.cell.y * 2 + end.subcell.y - (start.cell.y * 2 + start.subcell.y)) ==
                  int(turn.moves),
              "Movement event count matches physical cardinal subcell displacement");
    }
    for (int speed : {20, 30, 40}) {
        Fixture f(speed, {9, 12}, {1, 1}); // Six subcells from an available near slot.
        const auto turn = f.firstTurn();
        check(turn.moves == 6 && turn.attacks == unsigned(speed >= 30) && turn.dashes == unsigned(speed < 30),
              "Normal-budget arrival attacks immediately; Dash arrival does not attack");
    }
}
void conditions() {
    for (bool frost : {false, true}) {
        Fixture f(30, {9, 13}, {1, 1});
        while (!f.combat.snapshot().activeEnemy || f.combat.snapshot().activeEnemy->slot != 1)
            f.tick();
        f.effects.bindEvents(f.events, f.now);
        for (int x : {8, 9, 10})
            check(f.effects.createArea({x, 15}, {0}, 1, f.now, f.events).serial != 0,
                  "Owned front terrain covers all near-row alternatives");
        if (frost)
            f.effects.condition(f.actors.id(1), EffectKind::Slow, {0}, f.now, f.events);
        const auto turn = f.firstTurn();
        check(turn.moves == 4 && turn.attacks == unsigned(!frost) && turn.dashes == unsigned(frost),
              "Terrain costs thirty feet over four steps; Frost requires Dash instead of an attack");
        check(turn.groundSaves == 2,
              "Grease checks once on cell entry and once at turn end, never on in-cell substeps");
    }
    {
        Fixture f(0, {9, 15}, {1, 1});
        f.effects.condition(f.actors.id(1), EffectKind::Knockdown, {0}, 0, f.events);
        const auto turn = f.firstTurn();
        check(!turn.moves && !turn.dashes && turn.attacks == 1 &&
                  f.effects.has(f.actors.id(1), EffectKind::Knockdown),
              "Zero Speed prevents standing but retains a legal prone attack");
    }
    {
        Fixture f(30);
        f.effects.condition(f.actors.id(1), EffectKind::Sleep, {0}, 0, f.events);
        const auto turn = f.firstTurn();
        check(!turn.moves && !turn.dashes && !turn.attacks,
              "Incapacitated enemy cannot spend either movement or its action");
    }
    {
        Fixture f(30);
        // Apply frost after this round's party caster turn, preserving its real lifetime.
        while (!f.combat.snapshot().activeEnemy || f.combat.snapshot().activeEnemy->slot != 1)
            f.tick();
        f.effects.condition(f.actors.id(1), EffectKind::Slow, {0}, f.now, f.events);
        const auto turn = f.firstTurn();
        check(turn.moves == 8 && turn.dashes == 1 && !turn.attacks,
              "Ray of Frost subtracts ten feet from both ordinary and Dash allowances");
    }
    {
        Fixture f(30, {9, 12}, {1, 1});
        f.effects.condition(f.actors.id(1), EffectKind::Knockdown, {0}, 0, f.events);
        const auto turn = f.firstTurn();
        check(turn.moves == 6 && turn.dashes == 1 && !turn.attacks,
              "Standing spends half Speed, leaving three normal steps before Dash");
    }
    {
        Fixture f(30, {9, 14}, {1, 1});
        f.effects.condition(f.actors.id(1), EffectKind::Knockdown, {0}, 0, f.events);
        const auto turn = f.firstTurn();
        check(turn.moves == 2 && turn.attacks == 1 && !turn.dashes,
              "Standing retains an attack when the remaining movement reaches melee");
    }
    {
        Fixture f(30, {9, 12}, {1, 1});
        f.effects.bindEvents(f.events, 0);
        check(f.effects.createArea({9, 13}, {0}, 1, 0, f.events).serial != 0,
              "Terrain fixture owns its Grease area");
        const auto turn = f.firstTurn();
        // The cheaper detour avoids four costly terrain edges through this cell.
        check(turn.moves >= 6 && turn.moves <= 8 && turn.dashes == 1 && !turn.attacks,
              "Terrain route accounts for doubled entered-subcell cost or a cheaper detour");
    }
    {
        Fixture f(30);
        const auto target = f.actors.id(1);
        const auto cell = f.actors.actor(target)->npc.cell;
        f.effects.bindEvents(f.events, 0);
        check(f.effects.createWeb(&cell, 1, {0}, 1000, 0, f.events), "Owned Web fixture created");
        f.effects.groundCheck(target, cell, 0, f.events);
        check(f.effects.has(target, EffectKind::Restrained), "Owned target is restrained by Web");
        const auto turn = f.firstTurn();
        check(!turn.moves && !turn.attacks && !turn.dashes && f.effects.has(target, EffectKind::Restrained),
              "Failed Web escape spends the action and grants no movement or Dash");
    }
    {
        Fixture f(30);
        while (!f.combat.snapshot().activeEnemy || f.combat.snapshot().activeEnemy->slot != 1)
            f.tick();
        f.rng = rules::DiceRng(0); // First Dex save fails; the following Strength check succeeds.
        const auto target = f.actors.id(1);
        const auto cell = f.actors.actor(target)->npc.cell;
        f.effects.bindEvents(f.events, f.now);
        check(f.effects.createWeb(&cell, 1, {0}, 10, f.now, f.events) &&
                  f.effects.has(target, EffectKind::Restrained),
              "Owned target fails its deterministic Web entry save");
        const auto turn = f.firstTurn();
        check(turn.moves > 0 && !turn.attacks && !turn.dashes &&
                  !f.effects.has(target, EffectKind::Restrained),
              "Successful escape retains ordinary movement but consumes the attack and Dash action");
    }
    {
        Fixture f(30);
        f.effects.condition(rules::CharacterId{0}, EffectKind::Knockdown, {0}, 0, f.events);
        bool checked = false;
        for (unsigned tick = 0; tick < 100 && !checked; ++tick) {
            f.tick();
            const auto& state = f.combat.snapshot();
            if (state.spent[0]) {
                check(!f.effects.has(rules::CharacterId{0}, EffectKind::Knockdown) && state.defending[0],
                      "Party stands before command admission and can Defend in that same turn");
                checked = true;
            }
        }
        check(checked, "Party stand-and-act regression reaches its turn");
    }
}
void navigationBoundaries() {
    const auto map = room(67);
    const auto catalog = combatCatalog();
    const Player party{{17, 60}, Heading::North};
    NpcWorld placements(map, party.cell, {"goblin"}, {{{0}, {17, 5}, {0, 0}, Heading::South}});
    ActorWorld actors(map, catalog, placements);
    TacticalNavigation nav(map, actors);
    std::size_t work = 0;
    allocations = 0;
    watching = true;
    auto result = nav.next(actors.id(0), party, party.cell, false, nullptr, work);
    watching = false;
    check(result.status == TacticalNavigation::Status::Pending && work == Navigation::nodeBudget &&
              !allocations,
          "Distant search respects the per-update budget without allocation");
    for (unsigned i = 0; i < 12 && result.status == TacticalNavigation::Status::Pending; ++i) {
        work = 0;
        result = nav.next(actors.id(0), party, party.cell, false, nullptr, work);
        check(work <= Navigation::nodeBudget, "Search and route reconstruction remain bounded");
    }
    check(result.status == TacticalNavigation::Status::Step && result.costFeet == 5,
          "Pending tactical search eventually returns a five-foot subcell edge");
}
} // namespace
int main() {
    try {
        budgetsAndDash();
        conditions();
        navigationBoundaries();
        std::cout << "combat movement tests passed\n";
    } catch (const std::exception& error) {
        watching = false;
        std::cerr << error.what() << '\n';
        return 1;
    }
}

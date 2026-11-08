#include "game/approach_slots.hpp"
#include "game/perception.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "support/hybrid_fixtures.hpp"
#include <iostream>

using namespace blobber;
namespace {
Cell nearSlot(Heading heading, int lane) {
    const auto f = forward(heading);
    return f.x ? Cell{f.x > 0 ? 0 : 1, lane} : Cell{lane, f.y > 0 ? 0 : 1};
}
NpcInstance npc(Cell cell, Cell subcell, Cell party) {
    return {{0}, cell, subcell, headingToward(cell, party)};
}
void checkOccupancy(const ActorWorld& actors) {
    const auto& state = actors.snapshot();
    for (unsigned i = 0; i < state.count; ++i)
        for (unsigned j = 0; j < i; ++j)
            check(state.actors[i].life != ActorLife::Alive || state.actors[j].life != ActorLife::Alive ||
                      !(state.actors[i].npc.cell == state.actors[j].npc.cell) ||
                      !(state.actors[i].npc.subcell == state.actors[j].npc.subcell),
                  "Positioning cannot overlap living destination reservations");
}
void runToAttack(GameSession& game, ActorId waiting, unsigned limit = 8) {
    const auto party = game.movement().player;
    const auto anchor = game.actors().actor(game.actors().id(0))->npc;
    std::uint64_t attackedRound = 0;
    unsigned moves = 0, attacks = 0;
    for (unsigned tick = 0; tick < 800 && game.combat().snapshot().round <= limit && !attacks; ++tick) {
        const auto before = game.combat().snapshot();
        const auto position = game.actors().actor(waiting)->npc;
        const auto revision = game.actors().snapshot().revision;
        auto preview = before;
        game.combat().previewTurns(preview);
        check(game.actors().snapshot().revision == revision, "Turn preview never moves or reserves actors");
        allocations = 0;
        watching = true;
        if (before.awaitingAction)
            game.submit(turnCommand(game));
        else {
            const auto& events = game.update(.5, false);
            for (const auto& out : events) {
                if (out.event == CombatEvent::AttackStarted)
                    check(inPartyView(game.geometry(), party.cell, party.heading,
                                      game.actors().actor(out.enemy)->npc.cell,
                                      game.actors().actor(out.enemy)->npc.subcell),
                          "Every enemy attack starts from the current frontal view");
                if (!sameActor(out.enemy, waiting))
                    continue;
                if (out.event == CombatEvent::Move) {
                    check(attackedRound != before.round,
                          "An enemy does not resume positioning after attacking");
                    ++moves;
                }
                if (out.event == CombatEvent::AttackStarted) {
                    check(attackedRound != before.round,
                          "A repositioning enemy attacks at most once in its round");
                    attackedRound = before.round;
                    ++attacks;
                }
            }
        }
        watching = false;
        check(allocations == 0, "Combat movement, preview and path continuation allocate no C++ storage");
        if (before.phase == CombatPhase::RoundEnd)
            check(game.actors().actor(waiting)->npc.cell == position.cell &&
                      game.actors().actor(waiting)->npc.subcell == position.subcell,
                  "Round-end roaming cannot move an enrolled enemy a second time");
        const auto* held = game.actors().actor(game.actors().id(0));
        check(held->npc.cell == anchor.cell && held->npc.subcell == anchor.subcell,
              "An enemy with a usable attack position holds it");
        check(game.movement().player == party, "Advancing flank enemies do not rotate a populated front");
        checkOccupancy(game.actors());
    }
    check(moves > 0 && attacks > 0,
          "Waiting enemy reaches a usable position and attacks within bounded rounds");
}
void flanksAndBlockedRank() {
    const auto map = hybridRoom();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    for (unsigned h = 0; h < 4; ++h)
        for (int side : {-1, 1})
            for (Cell subcell : {Cell{0, 0}, Cell{1, 1}}) {
                const Player party{{6, 6}, Heading(h)};
                const auto f = forward(party.heading), r = right(party.heading);
                NpcWorld places(map, party.cell, {"goblin"},
                                {npc(party.cell + f, nearSlot(party.heading, 0), party.cell),
                                 npc(party.cell + r * side, subcell, party.cell)});
                GameSession game(map, catalog, places, 5, party, false);
                runToAttack(game, game.actors().id(1));
            }
    for (unsigned h = 0; h < 4; ++h) {
        const Player party{{6, 6}, Heading(h)};
        const auto f = forward(party.heading), front = party.cell + f;
        NpcWorld places(map, party.cell, {"goblin"},
                        {npc(front, nearSlot(party.heading, 0), party.cell),
                         npc(front, nearSlot(party.heading, 1), party.cell),
                         npc(front, nearSlot(party.heading, 0) + f, party.cell)});
        GameSession game(map, catalog, places, 5, party, false);
        runToAttack(game, game.actors().id(2));
        check(!(game.actors().actor(game.actors().id(2))->npc.cell == front),
              "Melee-only rear enemy routes to another front cell when both local near slots are occupied");
    }
}
void detourAndLocalMotion() {
    const auto map = hybridRoom();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    const Player party{{6, 6}, Heading::North};
    std::vector<NpcInstance> instances{npc({5, 6}, {1, 1}, party.cell), npc({6, 5}, {0, 1}, party.cell)};
    for (int sub = 0; sub < 4; ++sub)
        instances.push_back(npc({5, 5}, {sub % 2, sub / 2}, party.cell));
    NpcWorld places(map, party.cell, {"goblin"}, instances);
    ActorWorld actors(map, catalog, places);
    NpcAiController ai(map, actors, 5);
    check(ai.position(actors.id(0), party, 0) == PositioningResult::Moved,
          "A fully occupied direct route does not make the flanker give up");
    auto here = actors.actor(actors.id(0))->npc.cell;
    check(std::abs(here.x - party.cell.x) + std::abs(here.y - party.cell.y) > 1,
          "The detour can initially increase distance from the party");
    for (unsigned step = 1; step < 12 && !inFrontBand(party.cell, party.heading, here); ++step) {
        ai.position(actors.id(0), party, step * npcWalkDurationMs);
        here = actors.actor(actors.id(0))->npc.cell;
        checkOccupancy(actors);
    }
    check(inFrontBand(party.cell, party.heading, here),
          "Repeated normal moves complete a route around allies");

    instances = {npc({5, 5}, {0, 1}, party.cell)};
    for (int sub = 0; sub < 4; ++sub)
        instances.push_back(npc({6, 5}, {sub % 2, sub / 2}, party.cell));
    NpcWorld localPlaces(map, party.cell, {"goblin"}, instances);
    ActorWorld local(map, catalog, localPlaces);
    NpcAiController localAi(map, local, 5);
    check(!inPartyViewport(party.cell, party.heading, {5, 5}, {0, 1}), "Local fixture starts offscreen");
    check(localAi.position(local.id(0), party, 0) == PositioningResult::Moved &&
              local.actor(local.id(0))->npc.subcell == Cell{1, 1},
          "Hidden diagonal enemy can reposition into a visible slot of its own cell");
    const auto& a = *local.actor(local.id(0));
    check(actorWalking(a, 0) && std::abs(actorPosition(a, 0).x - 5.25f) < 1e-6f &&
              std::abs(actorPosition(a, 300).x - 5.5f) < 1e-6f &&
              std::abs(actorPosition(a, 600).x - 5.75f) < 1e-6f && !actorWalking(a, 600),
          "Same-cell positioning interpolates from original feet instead of teleporting");

    Party partyState(catalog);
    rules::DiceRng rng(5);
    CombatEvents events;
    EffectController effects(partyState, local, rng);
    effects.bindEvents(events, 1000);
    effects.partyCell(party.cell);
    check(effects.createArea({5, 5}, {3}, 1000, 1000, events).serial,
          "Same-cell hazard fixture creates difficult terrain");
    local.move(local.id(0), {5, 5}, {0, 1}, party.cell);
    localAi.bindEffects(&effects);
    localAi.turnMovement(true);
    events.clear();
    check(localAi.position(local.id(0), party, 1000) == PositioningResult::Moved &&
              local.actor(local.id(0))->npc.subcell == Cell{0, 1},
          "A difficult-terrain positioning action first prepares without moving");
    check(localAi.position(local.id(0), party, 2000) == PositioningResult::Moved &&
              local.actor(local.id(0))->npc.subcell == Cell{1, 1} &&
              local.actor(local.id(0))->walkDurationMs == 1200 && events.count == 0,
          "Same-cell movement keeps slow travel without repeating cell-entry hazard saves");
}
void pendingSearchTurn() {
    // Just large enough for an unreachable centre preference to exceed one node
    // budget. The actor can reach other contact cells around the outside.
    std::vector<std::string> rows(67, std::string(67, '.'));
    rows.front() = rows.back() = std::string(67, '#');
    for (auto& row : rows)
        row.front() = row.back() = '#';
    rows[31][33] = rows[32][32] = rows[32][34] = '#';
    const Map map(rows);
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    const Player before{{33, 34}, Heading::North}, party{{33, 33}, Heading::North};
    NpcWorld places(map, before.cell, {"goblin"},
                    {npc({33, 32}, {0, 1}, before.cell), npc({38, 33}, {0, 0}, before.cell)});
    {
        ActorWorld actors(map, catalog, places);
        Navigation original(map, actors), resumed(map, actors);
        original.request(1, {38, 33}, party.cell, true, Cell{33, 32});
        original.update(party.cell);
        check(original.lastWork() == Navigation::nodeBudget && !original.take(1).ready,
              "Preferred positioning fixture actually crosses the incremental search budget");
        resumed.restore(original.checkpoint());
        bool completed = false;
        for (unsigned tick = 0; tick < 8 && !completed; ++tick) {
            original.update(party.cell);
            resumed.update(party.cell);
            const auto a = original.take(1), b = resumed.take(1);
            check(a.ready == b.ready && a.found == b.found && a.step == b.step &&
                      original.lastWork() == resumed.lastWork() &&
                      original.lastWork() <= Navigation::nodeBudget &&
                      original.lastCompleted() <= Navigation::requestBudget,
                  "Saved preferred searches retain exact continuation and bounded work");
            completed = a.ready && a.found;
        }
        check(completed, "A restored preferred search publishes its reachable fallback");
    }
    GameSession game(map, catalog, places, 5, before, true);
    game.update(.1, false); // Both guards see the party before contact.
    game.restorePose(party, false);
    const auto target = game.actors().id(1);
    bool pending = false, moved = false;
    for (unsigned tick = 0; tick < 200 && !moved; ++tick) {
        if (game.combat().snapshot().awaitingAction)
            game.submit(turnCommand(game));
        else {
            const auto& events = game.update(.5, false);
            const auto& current = game.combat().snapshot();
            const bool waiting = events.step.event == CombatEvent::None &&
                                 sameActor(events.step.enemy, target) && !current.resolvingAction &&
                                 current.activeEnemy && sameActor(*current.activeEnemy, target);
            pending |= waiting;
            moved |= events.step.event == CombatEvent::Move && sameActor(events.step.enemy, target);
            if (waiting) {
                const auto state = game.combat().snapshot();
                const auto revision = game.actors().snapshot().revision;
                game.update(60, true);
                check(game.combat().snapshot().turn == state.turn &&
                          game.combat().snapshot().gameplayMs == state.gameplayMs &&
                          game.actors().snapshot().revision == revision &&
                          sameActor(*game.combat().snapshot().activeEnemy, target),
                      "Pause preserves an active pending navigation turn without charging a round");
            }
        }
    }
    check(pending && moved,
          "Budget-limited combat navigation retains its turn and eventually commits one move");
}
void reopenedVacancy() {
    const auto map = hybridRoom();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    const Player party{{6, 6}, Heading::North};
    std::vector<NpcInstance> instances{npc({5, 6}, {1, 1}, party.cell), npc({7, 6}, {0, 1}, party.cell)};
    for (int side = -1; side <= 1; ++side)
        for (int sub = 0; sub < 4; ++sub)
            instances.push_back(npc({6 + side, 5}, {sub % 2, sub / 2}, party.cell));
    NpcWorld places(map, party.cell, {"goblin"}, instances);
    ActorWorld actors(map, catalog, places);
    NpcAiController ai(map, actors, 5);
    for (unsigned i = 0; i < 2; ++i)
        check(ai.position(actors.id(i), party, 0) == PositioningResult::AtGoal,
              "A completely full front causes a stable wait rather than pointless shuffling");
    const auto vacancy = actors.actor(actors.id(8))->npc;
    actors.damage(actors.id(8), 1000, 0);
    unsigned moves = 0;
    for (unsigned step = 1; step < 16; ++step)
        for (unsigned i = 0; i < 2; ++i) {
            moves += ai.position(actors.id(i), party, step * npcWalkDurationMs) == PositioningResult::Moved;
            checkOccupancy(actors);
        }
    unsigned claimed = 0;
    for (unsigned i = 0; i < 2; ++i) {
        const auto& a = *actors.actor(actors.id(i));
        claimed += a.npc.cell == vacancy.cell && a.npc.subcell == vacancy.subcell;
    }
    check(moves > 0 && claimed == 1, "Waiting enemies retry a death-opened gap; exactly one claims it");
}
void facePartyOnArrival() {
    const auto map = hybridRoom();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    for (unsigned h = 0; h < 4; ++h)
        for (bool sameCell : {false, true}) {
            const Player party{{6, 6}, Heading(h)};
            const auto f = forward(party.heading), r = right(party.heading);
            const auto start = sameCell ? party.cell + f + r * -1 : party.cell + r * -1;
            const auto slot = nearSlot(party.heading, h < 2 ? 0 : 1);
            NpcWorld places(map, party.cell, {"goblin"},
                            {npc(party.cell + f, nearSlot(party.heading, 0), party.cell),
                             npc(party.cell + f, nearSlot(party.heading, 1), party.cell),
                             npc(start, slot, party.cell)});
            GameSession game(map, catalog, places, 5, party, false);
            const auto id = game.actors().id(2);
            bool moved = false;
            for (unsigned tick = 0; tick < 100 && !moved; ++tick) {
                if (game.combat().snapshot().awaitingAction)
                    game.submit(turnCommand(game));
                else
                    moved = game.update(.25, false).step.event == CombatEvent::Move &&
                            sameActor(game.events().step.enemy, id);
            }
            check(moved, "Facing fixture reaches the waiting enemy's positioning move");
            const auto* actor = game.actors().actor(id);
            const auto travelFacing = actor->npc.facing;
            const auto arrivedFacing = inMeleeFront(game.geometry(), party.cell, party.heading, actor->npc.cell)
                                           ? headingToward(actor->npc.cell, party.cell) : travelFacing;
            check(actorWalking(*actor, game.timeMs()), "Arrival fixture starts during committed travel");
            const auto end = actor->walkTimeMs + actor->walkDurationMs;
            const auto turn = game.combat().snapshot().turn;
            const auto round = game.combat().snapshot().round;
            const auto gameplay = game.combat().snapshot().gameplayMs;
            game.update(double(end - game.timeMs() - 1) / 1000, false);
            check(game.timeMs() == end - 1 && actor->npc.facing == travelFacing &&
                      game.combat().snapshot().turn == turn,
                  "Travel-facing holds through the last millisecond before arrival");
            game.update(60, true);
            check(game.timeMs() == end - 1 && actor->npc.facing == travelFacing,
                  "Pause cannot trigger arrival-facing early");
            allocations = 0;
            watching = true;
            const auto& events = game.update(.001, false);
            watching = false;
            check(allocations == 0 && game.timeMs() == end && !actorWalking(*actor, game.timeMs()) &&
                      actor->npc.facing == arrivedFacing && !actor->hasAttack &&
                      game.combat().snapshot().round == round &&
                      game.combat().snapshot().gameplayMs == gameplay,
                  "Arrival faces the party immediately without an attack, extra action or allocation");
            for (const auto& out : events)
                check(out.event != CombatEvent::AttackStarted,
                      "Arrival-facing must not begin the next enemy's attack");
        }
}
} // namespace

int main() {
    try {
        flanksAndBlockedRank();
        detourAndLocalMotion();
        pendingSearchTurn();
        reopenedVacancy();
        facePartyOnArrival();
        std::cout << "Combat positioning, route, turn-cost and allocation checks passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}

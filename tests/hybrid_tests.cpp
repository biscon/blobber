#include "game/perception.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "support/hybrid_fixtures.hpp"
#include "ui/interaction.hpp"
#include <iostream>
using namespace blobber;
namespace {
NpcInstance npc(Cell c, Heading h = Heading::North, Cell sub = {0, 0}) {
    return {{0}, c, sub, h};
}
PartyCommand shot(const GameSession& s, ActorId target) {
    auto c = turnCommand(s, PartyAction::Ranged);
    c.character = {2};
    c.target = target;
    return c;
}
void tick(GameSession& s) {
    const auto st = s.combat().snapshot();
    if (st.awaitingAction)
        check(s.submit(turnCommand(s)).reason == CommandReason::Resolved, "Turn resolves");
    else
        s.update(.5, false);
}
void rangesAndCosts() {
    const auto m = hybridRoom();
    const auto c = hybridCatalog();
    const Player p{{5, 8}, Heading::North};
    NpcWorld places(m, p.cell, {"goblin"}, {npc({5, 4}), npc({7, 4}), npc({10, 4})});
    GameSession s(m, c, places, 5, p, false);
    s.setAnimated(false);
    check(inBowRange({1, 1}, {7, 1}) && !inBowRange({1, 1}, {7, 2}),
          "Euclidean range includes exact six, excludes beyond");
    const Map wall({"#####", "#.#.#", "#...#", "#...#", "#####"});
    check(!lineOfSight(wall, {1, 1}, {3, 1}) && !lineOfSight(wall, {1, 1}, {3, 3}),
          "Walls and diagonal corner seams stop shots");
    auto command = shot(s, s.actors().id(0));
    auto bad = command;
    bad.character = {0};
    check(s.submit(bad).reason == CommandReason::NoWeapon, "No ready ranged slot rejects");
    bad = command;
    bad.target = s.actors().id(2);
    check(s.submit(bad).reason == CommandReason::OutOfRange, "Out-of-range rejects before cost");
    check(s.combat().snapshot().cooldownMs[2] == 0, "Rejections spend no readiness");
    const auto out = s.submit(command);
    check(out.event == CombatEvent::AttackStarted, "Approach shot resolves");
    check(s.submit(command).reason == CommandReason::StaleTurn, "Repeated approach request rejected");
    check(s.ai().actors[0].awareness == Awareness::Routine &&
              s.ai().actors[1].awareness == Awareness::Routine,
          "Accepted opening shot does not alert its target or neighbors");
    check(s.submit(shot(s, s.actors().id(0))).reason == CommandReason::Cooldown,
          "Fresh token cannot bypass cooldown");
    const auto clock = s.combat().snapshot().gameplayMs;
    s.update(60, true);
    check(s.combat().snapshot().gameplayMs == clock && s.combat().snapshot().cooldownMs[2] == 6000,
          "Pause freezes costs");
    GameSession alertProbe(m, c, places, 5, p, false);
    alertProbe.submit(shot(alertProbe, alertProbe.actors().id(0)));
    alertProbe.update(.341, false);
    check(alertProbe.ai().actors[0].awareness == Awareness::Routine,
          "Opening shot stays hidden during flight");
    alertProbe.update(.001, false);
    check(alertProbe.ai().actors[0].awareness == Awareness::Pursuing &&
              alertProbe.ai().actors[1].awareness == Awareness::Pursuing &&
              alertProbe.ai().actors[2].awareness == Awareness::Routine,
          "Local impact alert reaches radius three without chaining");
    for (int i = 0; i < 3; ++i)
        s.move(Action::Forward);
    check(s.combat().snapshot().phase == CombatPhase::Contact, "Adjacent approach enters contact");
    s.update(1.341, false);
    check(s.combat().snapshot().phase == CombatPhase::Contact, "Contact waits outstanding marker");
    s.update(.001, false);
    check(s.combat().snapshot().phase == CombatPhase::Turns, "Contact enters initiative after feedback");
    const auto before = s.combat().snapshot().gameplayMs;
    s.update(60, false);
    check(s.combat().snapshot().gameplayMs == before, "Decision time cannot recharge cooldowns");

    bool skipped = false;
    const auto round = s.combat().snapshot().round;
    for (int i = 0; i < 150 && s.combat().snapshot().round == round; ++i) {
        const auto st = s.combat().snapshot();
        if (st.spent[2]) {
            check(!st.activeCharacter || st.activeCharacter->index != 2,
                  "Recent approach shot is debited without publishing a false player turn");
            skipped = true;
        }
        tick(s);
    }
    check(skipped && s.combat().snapshot().gameplayMs == before + 6000,
          "One completed round advances exactly six seconds");
}
void facingAndFormation() {
    const auto m = hybridRoom();
    const auto c = hybridCatalog(false);
    const Player p{{5, 5}, Heading::North};
    for (int d = 0; d < 4; ++d) {
        const auto direction = static_cast<Heading>(d);
        const auto cell = p.cell + forward(direction);
        NpcWorld places(m, p.cell, {"goblin"}, {npc(cell, headingToward(cell, p.cell))});
        GameSession s(m, c, places, 5, p, false);
        bool enemyActed = false, partyActed = false;
        for (int i = 0; i < 80 && !(enemyActed && partyActed); ++i) {
            const auto st = s.combat().snapshot();
            if (st.awaitingAction) {
                check(s.movement().player.heading == direction, "Contact auto-faces the engaging group");
                const auto pose = s.movement().player;
                for (int action = 0; action < 6; ++action)
                    s.move(static_cast<Action>(action));
                check(s.movement().player == pose, "Combat facing and movement stay locked");
                s.submit(turnCommand(s));
                partyActed = true;
            } else {
                const auto out = s.update(.2, false).step;
                if (out.event == CombatEvent::AttackStarted && out.kind == rules::AttackKind::Melee) {
                    check(out.recipient.index < 2,
                          "Front row protects back row regardless of original threat direction");
                    enemyActed = true;
                }
            }
        }
        check(enemyActed && partyActed, "Every contact direction uses normal opening initiative");
    }
}
void handoff() {
    const auto m = hybridRoom();
    const auto c = hybridCatalog(false);
    const Player p{{5, 6}, Heading::North};
    NpcWorld places(
        m, p.cell, {"goblin"},
        {npc({5, 5}, Heading::South, {0, 1}), npc({6, 6}, Heading::West), npc({4, 6}, Heading::East)});
    GameSession s(m, c, places, 5, p, false);
    s.update(0, false);
    for (int i = 0; i < 80 && !(s.combat().snapshot().awaitingAction &&
                                s.combat().snapshot().activeCharacter->index == 0);
         ++i)
        tick(s);
    const auto before = s.combat().snapshot();
    check(before.awaitingAction, "Fighter decision reached");
    const auto stale = turnCommand(s, PartyAction::Melee);
    s.update(60, false);
    check(s.movement().player.heading == Heading::North, "Living front group keeps steady facing");
    s.actors().damage(s.actors().id(0), 1000, s.timeMs());
    s.update(0, false);
    check(s.combat().snapshot().phase == CombatPhase::Facing && s.combat().snapshot().facing == Heading::East,
          "Cleared front automatically selects clockwise side on tie");
    check(s.submit(stale).reason == CommandReason::Moving, "No attack during camera movement");
    const auto time = s.timeMs();
    s.update(60, true);
    check(s.timeMs() == time && s.movement().transition.progress() == 0, "Pause freezes automatic facing");
    s.update(.159, false);
    check(s.combat().snapshot().phase == CombatPhase::Facing, "Quarter turn keeps its presentation hold");
    s.update(.001, false);
    const auto after = s.combat().snapshot();
    check(after.phase == CombatPhase::Turns && after.awaitingAction && after.activeCharacter &&
              after.activeCharacter->index == before.activeCharacter->index && after.round == before.round &&
              after.gameplayMs == before.gameplayMs,
          "Facing preserves the fighter action and round clock");
    check(!confirmationMatches(before, after) && s.submit(stale).reason == CommandReason::StaleTurn,
          "Old view cannot confirm the resumed action");
    auto command = turnCommand(s, PartyAction::Melee);
    command.target = s.actors().id(1);
    check(s.submit(command).event == CombatEvent::AttackStarted,
          "Fighter can attack the newly faced goblin immediately");
}
void facingPresentation() {
    const auto m = hybridRoom();
    const auto c = hybridCatalog(false);
    const Player p{{5, 6}, Heading::North};
    NpcWorld rear(m, p.cell, {"goblin"}, {npc({5, 7}, Heading::North)});
    GameSession turn(m, c, rear, 5, p, false);
    turn.update(0, false);
    check(turn.movement().player.heading == Heading::East &&
              turn.combat().snapshot().facing == Heading::South,
          "Rear contact begins two clockwise quarter turns");
    turn.update(.16, false);
    check(turn.combat().snapshot().phase == CombatPhase::Facing &&
              turn.movement().player.heading == Heading::South && turn.movement().transition.active(),
          "Rear turn does not expose a decision at its intermediate heading");
    turn.update(.159, false);
    check(turn.combat().snapshot().phase == CombatPhase::Facing,
          "Second quarter turn holds through its last frame");
    turn.update(.001, false);
    turn.update(0, false);
    check(turn.combat().snapshot().phase == CombatPhase::Turns && turn.combat().snapshot().gameplayMs == 0,
          "Settled rear contact enters normal initiative without charging time");

    NpcWorld places(m, p.cell, {"goblin"}, {npc({5, 5}, Heading::South, {0, 1}), npc({6, 6}, Heading::West)});
    NpcTimings timing{};
    timing[0] = {{600, 600, 0, 0}, 2};
    timing[static_cast<std::size_t>(NpcAnimation::Attack)] = {{700, 250, 250, 350}, 4};
    GameSession s(m, c, places, 5, p, false, {timing});
    s.update(0, false);
    bool attacked = false;
    for (int i = 0; i < 100 && !attacked; ++i) {
        if (s.combat().snapshot().awaitingAction)
            s.submit(turnCommand(s));
        else
            attacked = s.update(.5, false).step.event == CombatEvent::AttackStarted;
    }
    check(attacked, "Front attack presentation started");
    s.actors().damage(s.actors().id(0), 1000, s.timeMs());
    s.update(1.949, false);
    check(s.combat().snapshot().resolvingAction && s.movement().player.heading == Heading::North,
          "Removing the front threat cannot interrupt the existing impact marker");
    s.update(.001, false);
    check(s.combat().snapshot().phase == CombatPhase::Facing && s.combat().snapshot().facing == Heading::East,
          "Automatic facing begins only after the attack and marker complete");
}
void offscreenAndPartialRound() {
    const auto m = hybridRoom();
    const auto c = hybridCatalog(false);
    const Player p{{5, 6}, Heading::North};
    NpcWorld places(
        m, p.cell, {"goblin"},
        {npc({5, 5}, Heading::South, {0, 1}), npc({6, 6}, Heading::West), npc({5, 9}, Heading::North)});
    GameSession s(m, c, places, 5, p, true);
    s.update(0, false);
    const auto round = s.combat().snapshot().round;
    for (int i = 0; i < 100 && s.combat().snapshot().round == round; ++i) {
        if (s.combat().snapshot().awaitingAction)
            s.submit(turnCommand(s));
        else {
            const auto out = s.update(.5, false).step;
            if (out.event == CombatEvent::AttackStarted)
                check(inPartyView(s.geometry(), p.cell, p.heading, s.actors().actor(out.enemy)->npc.cell,
                                  s.actors().actor(out.enemy)->npc.subcell),
                      "Side and rear enemies must move into the visible front before attacking");
        }
    }
    // Outside combat an alerted offscreen archer also cannot shoot.
    NpcWorld behind(m, p.cell, {"goblin"}, {npc({5, 9}, Heading::North)});
    GameSession approach(m, c, behind, 5, p, true);
    check(approach.update(.1, false).step.event != CombatEvent::AttackStarted,
          "Offscreen approach archer cannot shoot");

    NpcWorld distant(m, p.cell, {"goblin"},
                     {npc({5, 5}, Heading::South, {0, 1}), npc({5, 2}, Heading::South)});
    GameSession partial(m, c, distant, 5, {{5, 8}, Heading::North}, false);
    partial.setAnimated(false);
    check(partial.submit(shot(partial, partial.actors().id(1))).event == CombatEvent::AttackStarted,
          "Opening shot alerts distant threat");
    partial.move(Action::Forward);
    partial.move(Action::Forward);
    partial.update(1, false);
    for (int i = 0; i < 80 && !partial.combat().snapshot().awaitingAction; ++i)
        partial.update(.5, false);
    const auto actor = *partial.combat().snapshot().activeCharacter;
    partial.submit(turnCommand(partial));
    const auto clock = partial.combat().snapshot().gameplayMs;
    partial.actors().damage(partial.actors().id(0), 1000, partial.timeMs());
    partial.update(0, false);
    check(partial.combat().snapshot().phase == CombatPhase::Approach,
          "No adjacency ends the incomplete turn pass");
    check(partial.combat().snapshot().gameplayMs == clock, "An incomplete round never awards six seconds");
    check(partial.combat().snapshot().cooldownMs[actor.index] == 6000,
          "Leaving a partial round cannot recharge an action");
    partial.move(Action::Forward);
    partial.move(Action::Forward);
    partial.move(Action::Forward);
    partial.update(0, false);
    check(partial.combat().snapshot().phase == CombatPhase::Turns &&
              partial.combat().snapshot().gameplayMs == clock,
          "Immediate re-contact retains the gameplay clock");
    for (int i = 0; i < 100 && partial.combat().snapshot().gameplayMs == clock; ++i) {
        const auto st = partial.combat().snapshot();
        check(!(st.awaitingAction && st.activeCharacter->index == actor.index),
              "Re-contact cannot replay the spent action");
        tick(partial);
    }
}
void enemySurpriseFeedback() {
    const auto map = hybridRoom();
    const auto catalog = hybridCatalog(false);
    const Player p{{5, 6}, Heading::North};
    // The guard faces away and AI is disabled so it has not noticed this contact.
    NpcWorld places(map, p.cell, {"goblin"}, {npc({5, 5}, Heading::North, {0, 1})});
    GameSession s(map, catalog, places, 5, p, false);
    CommandController commands(s);
    unsigned surprise = 0;
    bool laterAttack = false;
    for (unsigned n = 0; n < 200 && !laterAttack; ++n) {
        if (s.combat().snapshot().awaitingAction)
            s.submit(turnCommand(s));
        else {
            const auto& events = s.update(.1, false);
            for (const auto& out : events) {
                if (out.event == CombatEvent::Spent && out.spentCause == SpentCause::Surprise) {
                    ++surprise;
                    check(sameActor(out.enemy, s.actors().id(0)) && s.combat().snapshot().round == 1,
                          "Only the unaware enemy loses its opening opportunity to surprise");
                    commands.record(out);
                    check(std::string(commands.snapshot().log.newest(0)).find("surprised") !=
                              std::string::npos,
                          "Surprise log identifies the enemy and the specific cause");
                }
                if (out.event == CombatEvent::AttackStarted && out.enemyAttack) {
                    check(s.combat().snapshot().round > 1,
                          "Surprised enemy cannot attack during its first round");
                    laterAttack = true;
                }
            }
        }
    }
    check(surprise == 1 && laterAttack, "Enemy-only surprise is consumed once and normal actions resume");
}
void interruptedRoundReadiness() {
    for (const unsigned gap : {0u, 1200u, 7000u}) {
        const auto m = hybridRoom();
        const auto c = hybridCatalog(false);
        const Player p{{5, 8}, Heading::North};
        auto reinforcement = npc({10, 10});
        reinforcement.monster = c.findMonster("swordsman");
        NpcWorld places(m, p.cell, {"goblin"},
                        {npc({5, 5}, Heading::South, {0, 1}), npc({5, 2}, Heading::South), reinforcement});
        GameSession s(m, c, places, 5, p, false);
        s.setAnimated(false);
        s.submit(shot(s, s.actors().id(1)));
        s.move(Action::Forward);
        s.move(Action::Forward);
        for (unsigned n = 0; n < 80 && !s.combat().snapshot().awaitingAction; ++n)
            s.update(.1, false);
        const auto first = s.combat().snapshot();
        check(first.awaitingAction, "Interrupted-round fixture reaches a real decision");
        const auto spent = *first.activeCharacter;
        s.submit(turnCommand(s));
        s.actors().damage(s.actors().id(0), 1000, s.timeMs());
        s.update(0, false);
        check(s.combat().snapshot().phase == CombatPhase::Approach,
              "Only distant threat remains between contacts");
        s.update(double(gap) / 1000, false);
        const auto approached = s.combat().snapshot().gameplayMs;
        s.move(Action::Forward);
        s.move(Action::Forward);
        s.move(Action::Forward);
        const auto newcomer = s.actors().id(2);
        const auto arrival = s.movement().player.cell + Cell{1, 0};
        for (int y = 9; y >= arrival.y; --y)
            check(bool(s.actors().move(newcomer, {10, y}, {0, 1}, s.movement().player.cell)),
                  "Reinforcement approaches along legal cells");
        for (int x = 9; x >= arrival.x; --x)
            check(bool(s.actors().move(newcomer, {x, arrival.y}, {0, 1}, s.movement().player.cell)),
                  "A new monster group arrives during the contact break");
        auto contact = s.update(0, false).step;
        for (unsigned n = 0; n < 50 && s.combat().snapshot().phase == CombatPhase::Contact; ++n)
            contact = s.update(.1, false).step;
        const auto resumed = s.combat().snapshot();
        check(contact.event == CombatEvent::Started && contact.resumedRound == (gap < 6000),
              "Contact announcement distinguishes resumed and fresh rounds");
        check(resumed.round == first.round + (gap >= 6000),
              "Short contact break retains the initiative round; a full interval renews it");
        check(resumed.gameplayMs == approached, "Recontact does not add game time");
        if (gap < 6000) {
            check(resumed.spent[spent.index], "Spent action survives a brief contact break");
            check(!s.combat().member(newcomer), "Recontact defers a newcomer until the round boundary");
            bool unusedActed = false;
            for (unsigned n = 0; n < 150 && s.combat().snapshot().round == first.round; ++n) {
                const auto st = s.combat().snapshot();
                check(!st.activeCharacter || st.activeCharacter->index != spent.index,
                      "Spent character never flashes as an active turn on recontact");
                if (st.awaitingAction) {
                    unusedActed = true;
                    s.submit(turnCommand(s));
                } else
                    s.update(.1, false);
            }
            check(unusedActed, "Unused party opportunities remain in the resumed round");
            check(s.combat().member(newcomer), "The deferred monster group joins the next round");
            check(s.combat().snapshot().gameplayMs == first.gameplayMs + 6000,
                  "Approach interval is counted exactly once in round time");
            check(s.combat().snapshot().cooldownMs[spent.index] == 0,
                  "Pre-break action is ready at the ordinary next-round boundary");
            for (unsigned i = 0; i < rules::partySize; ++i)
                check(s.combat().snapshot().cooldownMs[i] == 0,
                      "Actions used after resuming also recharge at that boundary");
        } else
            check(!resumed.spent[spent.index] && resumed.cooldownMs[spent.index] == 0,
                  "A whole interval in approach renews the old spent action");
    }
}
void reinforcementAndSaves() {
    const auto m = hybridRoom();
    const auto c = hybridCatalog(false);
    const Player p{{5, 6}, Heading::North};
    NpcWorld places(m, p.cell, {"goblin"}, {npc({5, 5}, Heading::South, {0, 1}), npc({6, 8})});
    GameSession s(m, c, places, 5, p, false);
    s.update(0, false);

    check(s.combat().snapshot().enemyCount == 1, "Distant unaware actor is outside encounter");
    const auto newcomer = s.actors().id(1);
    s.actors().move(newcomer, {6, 7}, {0, 0}, p.cell);
    check(s.actors().walk(newcomer, {6, 6}, {0, 0}, p.cell, s.timeMs()).changed,
          "Fixture introduces arrival");
    s.update(.5, false);
    check(!s.combat().member(newcomer), "Arrival cannot enter current initiative round");
    s.actors().damage(s.actors().id(0), 1000, s.timeMs());
    s.update(.099, false);
    check(s.combat().snapshot().phase == CombatPhase::Turns && s.movement().player.heading == Heading::North,
          "Facing waits for a pending arrival's committed travel");
    s.update(.001, false);
    check(s.combat().snapshot().phase != CombatPhase::Victory,
          "Pending living arrival prevents premature victory");
    const auto first = s.combat().snapshot().round;
    s.update(.16, false);
    check(s.combat().snapshot().awaitingAction && !s.combat().member(newcomer),
          "Pending arrival can be faced without joining initiative early");
    auto immediate = turnCommand(s, PartyAction::Melee);
    immediate.target = newcomer;
    check(s.submit(immediate).event == CombatEvent::AttackStarted,
          "An arrival is attackable before its first initiative boundary");
    for (int i = 0; i < 100 && s.combat().snapshot().round == first; ++i)
        tick(s);
    check(s.combat().snapshot().round == first + 1 && s.combat().member(newcomer),
          "Arrival joins next boundary");

    bool acted = false;
    for (int i = 0; i < 100 && s.combat().snapshot().round == first + 1; ++i) {
        const auto st = s.combat().snapshot();
        acted |= st.activeEnemy && sameActor(*st.activeEnemy, newcomer);
        tick(s);
    }
    check(acted, "New same-group arrival receives an action next round");

    const Player distant{{5, 8}, Heading::North};
    NpcWorld target(m, distant.cell, {"goblin"}, {npc({5, 4})});
    GameSession saves(m, c, target, 5, distant, false);
    saves.setAnimated(false);
    saves.party().damage({0}, saves.party().character({0})->hitPoints, false);
    saves.submit(shot(saves, saves.actors().id(0)));
    saves.update(.342, false); // Impact starts the approach and its death-save deadline.
    check(saves.update(5.999, false).step.event != CombatEvent::DeathSave,
          "Real-time saves wait six gameplay seconds");
    check(saves.update(.001, false).step.event == CombatEvent::DeathSave,
          "Real-time save fires at exact deadline");
    for (int i = 0; i < 3; ++i)
        saves.move(Action::Forward);
    saves.update(2, false);

    int repeated = 0;
    const auto round = saves.combat().snapshot().round;
    for (int i = 0; i < 100 && saves.combat().snapshot().round == round; ++i) {
        if (saves.combat().snapshot().awaitingAction)
            saves.submit(turnCommand(saves));
        else
            repeated += saves.update(.5, false).step.event == CombatEvent::DeathSave;
    }
    check(repeated == 0, "Contact does not grant an extra death save before next six-second boundary");
}
void threeCellFront() {
    const auto map = hybridRoom();
    const auto catalog = hybridCatalog(false);
    for (int d = 0; d < 4; ++d) {
        const Player p{{6, 6}, static_cast<Heading>(d)};
        const auto f = forward(p.heading), r = right(p.heading);
        for (int side : {-1, 1}) {
            const auto cell = p.cell + f + r * side;
            NpcWorld places(map, p.cell, {"goblin"}, {npc(cell, headingToward(cell, p.cell))});
            GameSession s(map, catalog, places, 5, p, false);
            check(s.combat().snapshot().phase == CombatPhase::Contact,
                  "Diagonal adjacency starts contact in every heading");
            bool enemyMelee = false, playerMelee = false;
            for (int i = 0; i < 100 && !(enemyMelee && playerMelee); ++i) {
                const auto st = s.combat().snapshot();
                if (st.awaitingAction) {
                    auto command = turnCommand(s);
                    if (s.combat().exposed(*st.activeCharacter)) {
                        command.action = PartyAction::Melee;
                        command.target = s.actors().id(0);
                        const auto result = s.submit(command);
                        if (result.reason == CommandReason::FrontProtected)
                            s.submit(turnCommand(s));
                        else {
                            check(result.event == CombatEvent::AttackStarted,
                                  "Exposed front character can melee a diagonal enemy");
                            playerMelee = true;
                        }
                    } else {
                        command.action = PartyAction::Melee;
                        command.target = s.actors().id(0);
                        check(s.submit(command).reason == CommandReason::RearBlocked,
                              "Diagonal melee preserves paired party protection");
                        s.submit(turnCommand(s));
                    }
                } else {
                    const auto out = s.update(.5, false).step;
                    if (out.event == CombatEvent::AttackStarted) {
                        check(out.kind == rules::AttackKind::Melee && out.recipient.index < 2,
                              "Diagonal enemy melee attacks the protected formation's front row");
                        enemyMelee = true;
                    }
                }
                if (s.combat().snapshot().awaitingAction) {
                    const auto& body = s.actors().actor(s.actors().id(0))->npc;
                    check(inPartyViewport(p.cell, s.movement().player.heading, body.cell, body.subcell),
                          "Diagonal decisions face the body, including outer subslots");
                }
            }
            check(playerMelee && enemyMelee, "Both sides use diagonal melee");

            // The enemy's near/far rows follow the party heading, including east/west diagonals.
            std::vector<NpcInstance> rows;
            for (int sub = 0; sub < 4; ++sub)
                rows.push_back(npc(cell, headingToward(cell, p.cell), {sub % 2, sub / 2}));
            NpcWorld packed(map, p.cell, {"goblin"}, rows);
            GameSession protection(map, catalog, packed, 5, p, false);
            for (int sub = 0; sub < 4; ++sub) {
                const Cell local{sub % 2 * 2 - 1, sub / 2 * 2 - 1};
                check(protection.combat().exposed(protection.actors().id(sub)) ==
                          (local.x * f.x + local.y * f.y < 0),
                      "Diagonal enemy paired rows stay aligned to party heading");
            }
        }
    }
    const Player p{{5, 6}, Heading::North};
    NpcWorld group(map, p.cell, {"goblin"}, {npc({5, 5}), npc({4, 5}), npc({6, 6}, Heading::West)});
    GameSession held(map, catalog, group, 5, p, false);
    held.update(0, false);
    held.actors().damage(held.actors().id(0), 1000, 0);
    held.update(0, false);
    check(held.combat().snapshot().phase == CombatPhase::Turns && held.movement().player.heading == p.heading,
          "Clearing center does not turn away from a living diagonal front enemy");
    held.actors().damage(held.actors().id(1), 1000, 0);
    held.update(0, false);
    check(held.combat().snapshot().phase == CombatPhase::Facing &&
              held.combat().snapshot().facing == Heading::East,
          "Clearing the entire front band permits the next automatic turn");

    const Map corner({"#####", "#...#", "##..#", "#...#", "#####"});
    NpcWorld blocked(corner, {1, 3}, {"goblin"}, {npc({2, 2})});
    GameSession walls(corner, catalog, blocked, 5, {{1, 3}, Heading::North}, false);
    check(walls.combat().snapshot().phase == CombatPhase::Exploration &&
              !inMeleeFront(corner, {1, 3}, Heading::North, {2, 2}),
          "A wall at a diagonal corner blocks contact and melee");

    auto archer = npc({6, 5}, Heading::South);
    archer.monster = catalog.findMonster("archer");
    NpcWorld ranged(map, p.cell, {"goblin"}, {archer});
    GameSession escape(map, catalog, ranged, 5, p, false);
    bool moved = false;
    for (int i = 0; i < 100 && !moved; ++i) {
        if (escape.combat().snapshot().awaitingAction)
            escape.submit(turnCommand(escape));
        else
            moved = escape.update(.5, false).step.event == CombatEvent::Move;
    }
    check(moved && !combatAdjacent(map, p.cell, escape.actors().actor(escape.actors().id(0))->npc.cell),
          "Healthy diagonal archer escapes beyond the entire close-contact zone");
}
void offscreenDiagonalHandoff() {
    const auto map = hybridRoom();
    const auto catalog = hybridCatalog(false);
    unsigned offscreenCases = 0;
    for (unsigned h = 0; h < 4; ++h)
        for (int side : {-1, 1})
            for (unsigned sub = 0; sub < 4; ++sub)
                for (bool animated : {false, true}) {
                    const Player p{{6, 6}, Heading(h)};
                    const auto front = p.cell + forward(p.heading);
                    NpcWorld places(map, p.cell, {"goblin"},
                                    {npc(front, headingToward(front, p.cell), {0, 0}),
                                     npc(front, headingToward(front, p.cell), {1, 0})});
                    GameSession game(map, catalog, places, 5, p, false);
                    game.setAnimated(animated);
                    for (unsigned i = 0; i < 100 && !(game.combat().snapshot().awaitingAction &&
                                                      game.combat().snapshot().activeCharacter->index == 0);
                         ++i)
                        tick(game);
                    check(game.combat().snapshot().awaitingAction &&
                              game.combat().snapshot().activeCharacter->index == 0,
                          "Diagonal handoff fixture reaches fighter decision");
                    const auto target = game.actors().id(1);
                    const Cell diagonal = front + right(p.heading) * side, slot{int(sub % 2), int(sub / 2)};
                    check(bool(game.actors().move(target, diagonal, slot, p.cell)),
                          "Goblin relocates into front diagonal subslot");
                    const bool visible = inPartyViewport(p.cell, p.heading, diagonal, slot);
                    offscreenCases += !visible;
                    if (!visible) {
                        const auto round = game.combat().snapshot().round;
                        for (unsigned i = 0; i < 100 && game.combat().snapshot().round == round; ++i) {
                            if (game.combat().snapshot().awaitingAction)
                                game.submit(turnCommand(game));
                            else
                                for (const auto& out : game.update(.5, false))
                                    if (out.event == CombatEvent::AttackStarted && out.enemy.slot == 1) {
                                        const auto& actor = *game.actors().actor(target);
                                        check(inPartyView(map, p.cell, p.heading, actor.npc.cell,
                                                          actor.npc.subcell),
                                              "A diagonal enemy must reposition into view before attacking");
                                    }
                            check(game.movement().player.heading == p.heading,
                                  "Visible central group keeps the camera steady");
                        }
                        check(game.combat().snapshot().round == round + 1,
                              "Offscreen attack regression completes a bounded full round");
                        for (unsigned i = 0;
                             i < 100 && !(game.combat().snapshot().awaitingAction &&
                                          game.combat().snapshot().activeCharacter->index == 0);
                             ++i)
                            tick(game);
                    }
                    // Re-establish the stationary handoff arrangement: the earlier
                    // full-round check now legitimately permits enemy repositioning.
                    check(bool(game.actors().move(target, diagonal, slot, p.cell)),
                          "Handoff fixture restores the original diagonal body position");
                    const auto before = game.combat().snapshot();
                    check(before.awaitingAction && before.activeCharacter->index == 0,
                          "Preserve fighter's actionable turn at the handoff");
                    const auto old = turnCommand(game, PartyAction::Melee);
                    game.actors().damage(game.actors().id(0), 1000, game.timeMs());
                    game.update(0, false);
                    if (!visible) {
                        check(game.combat().snapshot().phase == CombatPhase::Facing,
                              "Offscreen diagonal cannot keep the cleared front camera locked");
                        for (unsigned i = 0; i < 6 && game.combat().snapshot().phase == CombatPhase::Facing;
                             ++i)
                            game.update(.16, false);
                        check(game.submit(old).reason == CommandReason::StaleTurn,
                              "Automatic handoff invalidates old view commands");
                    }
                    const auto& after = game.combat().snapshot();
                    check(after.phase == CombatPhase::Turns && after.awaitingAction &&
                              after.activeCharacter &&
                              after.activeCharacter->index == before.activeCharacter->index &&
                              after.round == before.round && after.gameplayMs == before.gameplayMs &&
                              inPartyViewport(p.cell, game.movement().player.heading, diagonal, slot),
                          "Handoff faces the surviving body and preserves current action/round");
                    auto command = turnCommand(game, PartyAction::Melee);
                    command.target = target;
                    check(game.submit(command).event == CombatEvent::AttackStarted,
                          "Remaining diagonal goblin is immediately attackable after facing");
                    game.update(3, false);
                    game.actors().damage(target, 1000, game.timeMs());
                    for (unsigned i = 0; i < 8 && game.combat().snapshot().phase != CombatPhase::Victory; ++i)
                        game.update(.5, false);
                    check(game.combat().snapshot().phase == CombatPhase::Victory,
                          "Diagonal handoff encounter can finish normally");
                }
    check(offscreenCases >= 8, "Regression covers actual offscreen subslots in all orientations");
}
void contactNeedsAnActionableFront() {
    const auto map = hybridRoom();
    const auto catalog = hybridCatalog(false);
    for (unsigned h = 0; h < 4; ++h)
        for (int side : {-1, 1}) {
            const Player pose{{6, 6}, Heading(h)};
            const auto f = forward(pose.heading);
            const auto cell = pose.cell + f + right(pose.heading) * side;
            Cell rear{}, front{};
            bool found = false;
            for (int sub = 0; sub < 4 && !found; ++sub) {
                rear = {sub % 2, sub / 2};
                front = rear + f * -1;
                if (front.x < 0 || front.x > 1 || front.y < 0 || front.y > 1)
                    continue;
                found = inPartyViewport(pose.cell, pose.heading, cell, rear) &&
                        !inPartyViewport(pose.cell, pose.heading, cell, front);
            }
            check(found, "Owned diagonal fixture has a visible protected rear and offscreen protector");
            NpcWorld places(map, pose.cell, {"goblin"},
                            {npc(cell, headingToward(cell, pose.cell), front),
                             npc(cell, headingToward(cell, pose.cell), rear)});
            GameSession game(map, catalog, places, 5, pose, false);
            check(!game.combat().occupiedFront(pose), "Protected rear alone cannot hold the facing lock");
            for (unsigned i = 0; i < 12 && game.combat().snapshot().phase != CombatPhase::Turns; ++i)
                game.update(.16, false);
            check(game.combat().snapshot().phase == CombatPhase::Turns &&
                      game.movement().player.heading != pose.heading,
                  "Entry automatically faces a reachable exposed body before presenting initiative");
            for (unsigned i = 0; i < 100 && !(game.combat().snapshot().awaitingAction &&
                                              game.combat().snapshot().activeCharacter->index == 0);
                 ++i)
                tick(game);
            check(game.combat().snapshot().awaitingAction &&
                      game.combat().snapshot().activeCharacter->index == 0,
                  "Fighter decision reached");
            bool melee = false;
            for (unsigned i = 0; i < 2; ++i) {
                auto command = turnCommand(game, PartyAction::Melee);
                command.target = game.actors().id(i);
                melee |= game.combat().validate(command, game.movement().player) == CommandReason::Ready;
            }
            check(melee, "Entry cannot strand the fighter with only protected/out-of-range targets");
        }

    const Player pose{{5, 7}, Heading::North};
    NpcWorld close(map, pose.cell, {"goblin"}, {npc({5, 6}, Heading::South, {0, 1})});
    GameSession cleared(map, catalog, close, 5, pose, false);
    cleared.setAnimated(false);
    cleared.update(0, false);
    const auto clock = cleared.combat().snapshot().gameplayMs;
    const auto id = cleared.actors().id(0);
    check(bool(cleared.actors().move(id, {5, 5}, {0, 1}, pose.cell)) &&
              bool(cleared.actors().move(id, {5, 4}, {0, 1}, pose.cell)),
          "Fixture withdraws the last close body");
    cleared.update(0, false);
    check(realTime(cleared.combat().snapshot().phase) && cleared.combat().snapshot().gameplayMs == clock,
          "Only distant enemies cannot keep the player in turnbased or charge a partial round");
    cleared.move(Action::Forward);
    check(cleared.movement().player.cell == Cell{5, 6}, "Player can close the distance normally");

    NpcWorld ahead(map, pose.cell, {"goblin"}, {npc({5, 5}, Heading::South, {0, 1})});
    GameSession queued(map, catalog, ahead, 5, pose, false);
    queued.move(Action::Forward);
    check(realTime(queued.combat().snapshot().phase), "Moving player has not yet reached close contact");
    queued.move(Action::TurnRight); // Queued input must not rotate away at the contact endpoint.
    queued.update(.159, false);
    check(realTime(queued.combat().snapshot().phase), "Contact waits for the settled endpoint");
    queued.update(.001, false);
    check(!realTime(queued.combat().snapshot().phase) && queued.movement().player.heading == Heading::North &&
              !queued.movement().transition.active() && queued.movement().revision == 1,
          "Settled endpoint takes contact before consuming queued exploration input");
}
void frontApproachInCombat() {
    const auto map = hybridRoom();
    const auto catalog = hybridCatalog();
    for (int d = 0; d < 4; ++d) {
        const Player party{{6, 6}, static_cast<Heading>(d)};
        const auto f = forward(party.heading), r = right(party.heading);
        const auto front = party.cell + f, start = party.cell + f * 2 + r;
        NpcWorld places(map, party.cell, {"goblin"},
                        {npc(front, headingToward(front, party.cell)),
                         npc(start, headingToward(start, party.cell), {1, 0})});
        GameSession s(map, catalog, places, 5, party, true);
        int melee = 0;
        bool approached = false;
        for (int i = 0; i < 500 && melee < 2; ++i) {
            if (s.combat().snapshot().awaitingAction)
                s.submit(turnCommand(s));
            else {
                const auto out = s.update(.5, false).step;
                if (out.enemy.slot == 1) {
                    if (out.event == CombatEvent::Move)
                        approached = true;
                    if (out.event == CombatEvent::AttackStarted)
                        check(approached && out.kind == rules::AttackKind::Melee,
                              "Healthy mixed goblin closes before choosing a ranged attack");
                }
                melee += out.event == CombatEvent::AttackStarted && out.enemy.slot == 1 &&
                         out.kind == rules::AttackKind::Melee;
            }
            const auto* actor = s.actors().actor(s.actors().id(1));
            check(inPartyView(map, party.cell, party.heading, actor->npc.cell) &&
                      s.movement().player.heading == party.heading,
                  "Healthy approaching goblin stays in the frontal fight across rounds");
            check(actor->hitPoints == actor->maxHitPoints, "Route fixture stays at full health");
        }
        check(approached && melee == 2 &&
                  inMeleeFront(map, party.cell, party.heading, s.actors().actor(s.actors().id(1))->npc.cell),
              "Approaching goblin joins a reachable front attack position and keeps attacking");
    }
}
void sideArcherFiringPositions() {
    const auto map = hybridRoom();
    const auto catalog = hybridCatalog();
    for (unsigned h = 0; h < 4; ++h)
        for (bool mixed : {false, true}) {
            const Player p{{6, 6}, Heading(h)};
            const auto f = forward(p.heading), r = right(p.heading);
            const auto side = p.cell + r;
            auto archer = npc(side, headingToward(side, p.cell), {1, 0});
            archer.monster = catalog.findMonster(mixed ? "goblin" : "archer");
            NpcWorld places(map, p.cell, {"goblin"}, {npc(p.cell + f, Heading((h + 2) % 4), {0, 0}), archer});
            GameSession s(map, catalog, places, 5, p, false);
            if (mixed)
                s.actors().damage(s.actors().id(1), 600, 0);
            bool moved = false, fired = false;
            std::uint64_t moveRound = 0, moveTurn = 0;
            for (unsigned n = 0; n < 400 && !fired; ++n) {
                if (s.combat().snapshot().awaitingAction)
                    s.submit(turnCommand(s));
                else {
                    const auto out = s.update(.1, false).step;
                    if (out.enemy.slot != 1 || !out.enemy.world)
                        continue;
                    if (out.event == CombatEvent::Move && !moved) {
                        moved = true;
                        moveRound = s.combat().snapshot().round;
                        moveTurn = s.combat().snapshot().turn;
                    }
                    if (out.event == CombatEvent::AttackStarted) {
                        check(moved && out.kind == rules::AttackKind::Ranged,
                              "Hidden side archer moves to a firing position before shooting");
                        check(s.combat().snapshot().round == moveRound &&
                                  s.combat().snapshot().turn == moveTurn,
                              "Archer completes multi-step reposition and shot in one opportunity");
                        const auto& a = *s.actors().actor(out.enemy);
                        check(inPartyView(map, p.cell, s.movement().player.heading, a.npc.cell,
                                          a.npc.subcell) &&
                                  a.npc.facing == headingToward(a.npc.cell, p.cell),
                              "Repositioned archer is targetable and faces the party");
                        fired = true;
                    }
                }
            }
            check(fired, "Pure and wounded mixed archers find usable firing positions in every heading");
        }
}
void singleWeaponPolicies() {
    const auto map = hybridRoom();
    const auto catalog = hybridCatalog();
    const Player p{{5, 8}, Heading::North};
    auto archer = npc({5, 4}, Heading::South);
    archer.monster = catalog.findMonster("archer");
    auto swordsman = archer;
    swordsman.monster = catalog.findMonster("swordsman");

    for (const bool wounded : {false, true}) {
        NpcWorld places(map, p.cell, {"goblin"}, {archer});
        GameSession ranged(map, catalog, places, 5, p, true);
        if (wounded)
            ranged.actors().damage(ranged.actors().id(0), 500, 0);
        int shots = 0;
        for (int i = 0; i < 140; ++i) {
            const auto out = ranged.update(.1, false).step;
            shots += out.event == CombatEvent::AttackStarted;
            check(ranged.actors().actor(ranged.actors().id(0))->npc.cell == archer.cell,
                  "Ranged-only NPC holds a clear shot at either health level");
        }
        check(shots >= 2, "Ranged-only NPC keeps shooting on cooldown without closing");

        NpcWorld meleePlaces(map, p.cell, {"goblin"}, {swordsman});
        GameSession melee(map, catalog, meleePlaces, 5, p, true);
        if (wounded)
            melee.actors().damage(melee.actors().id(0), 500, 0);
        bool attacked = false;
        for (int i = 0; i < 180 && !attacked; ++i) {
            if (melee.combat().snapshot().awaitingAction)
                melee.submit(turnCommand(melee));
            else {
                const auto out = melee.update(.1, false).step;
                if (out.event == CombatEvent::AttackStarted) {
                    check(out.kind == rules::AttackKind::Melee,
                          "Melee-only NPC never invents a ranged attack");
                    attacked = true;
                }
            }
        }
        check(attacked && melee.actors().actor(melee.actors().id(0))->npc.cell == Cell{5, 7},
              "Melee-only NPC closes and attacks even when wounded with escape trait");
    }

    // Same appearance, distinct rules binding; malformed overrides fail at setup.
    NpcWorld bound(map, p.cell, {"goblin"}, {archer, npc({6, 4})});
    ActorWorld actors(map, catalog, bound);
    check(actors.snapshot().actors[0].monster.index == catalog.findMonster("archer").index &&
              actors.snapshot().actors[1].monster.index == catalog.findMonster("goblin").index &&
              actors.snapshot().typeCount == 1,
          "Rules variants share one appearance and preserve default bindings");
    auto invalid = archer;
    invalid.monster = rules::MonsterId{999};
    bool rejected = false;
    try {
        NpcWorld broken(map, p.cell, {"goblin"}, {invalid});
        ActorWorld bad(map, catalog, broken);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    check(rejected, "Invalid rules override rejected during construction");

    // Turn combat remains active because the melee group still occupies the front.
    NpcWorld group(map, p.cell, {"goblin"}, {npc({5, 7}, Heading::South, {0, 1}), archer});
    GameSession combat(map, catalog, group, 5, p, true);
    int shots = 0;
    for (int i = 0; i < 600 && shots < 3; ++i) {
        if (combat.combat().snapshot().awaitingAction)
            combat.submit(turnCommand(combat));
        else {
            const auto out = combat.update(.5, false).step;
            if (out.enemy.slot == 1 && out.event == CombatEvent::AttackStarted) {
                check(out.kind == rules::AttackKind::Ranged, "Archer shoots in initiative");
                ++shots;
            }
        }
        check(combat.actors().actor(combat.actors().id(1))->npc.cell == archer.cell,
              "Ranged-only actor never alternates shooting with closing during combat");
    }
    check(shots == 3, "Ranged-only actor shoots across successive combat rounds");

    // Contact escape is independent of health and Nimble Escape.
    const auto noTrait = hybridCatalog(false);
    archer.cell = {5, 7};
    NpcWorld close(map, p.cell, {"goblin"}, {archer});
    GameSession retreat(map, noTrait, close, 5, p, false);
    bool moved = false;
    for (int i = 0; i < 100 && !moved; ++i) {
        if (retreat.combat().snapshot().awaitingAction)
            retreat.submit(turnCommand(retreat));
        else {
            const auto out = retreat.update(.5, false).step;
            check(out.event != CombatEvent::AttackStarted,
                  "Archer spends its action creating distance first");
            moved = out.event == CombatEvent::Move;
        }
    }
    check(moved && retreat.actors().actor(retreat.actors().id(0))->npc.cell == Cell{5, 6},
          "Full-health ranged-only NPC escapes without needing Nimble Escape");
    const auto retreatTurn = retreat.combat().snapshot().turn;
    bool retreatShot = false;
    for (int i = 0; i < 30 && !retreatShot; ++i) {
        const auto out = retreat.update(.1, false).step;
        if (out.event == CombatEvent::AttackStarted) {
            check(out.kind == rules::AttackKind::Ranged && retreat.combat().snapshot().turn == retreatTurn,
                  "Retreating archer uses its retained action to shoot in the same turn");
            retreatShot = true;
        }
    }
    check(retreatShot, "Escaping preserves the archer's ordinary attack action");

    const Map trapped({"#####", "#####", "##.##", "#...#", "#####"});
    archer.cell = {2, 2};
    NpcWorld blocked(trapped, {2, 3}, {"goblin"}, {archer});
    GameSession cornered(trapped, noTrait, blocked, 5, {{2, 3}, Heading::North}, false);
    bool fired = false;
    for (int i = 0; i < 100 && !fired; ++i) {
        if (cornered.combat().snapshot().awaitingAction)
            cornered.submit(turnCommand(cornered));
        else {
            const auto out = cornered.update(.5, false).step;
            fired = out.event == CombatEvent::AttackStarted && out.kind == rules::AttackKind::Ranged;
        }
    }
    check(fired && cornered.actors().actor(cornered.actors().id(0))->npc.cell == Cell{2, 2},
          "Trapped ranged-only actor shoots instead of inventing melee or crossing walls");
}
void rangedPursuit() {
    const Map map({"#############", "#...........#", "#...........#", "#...........#", "#...........#",
                   "#....#......#", "#...........#", "#...........#", "#...........#", "#...........#",
                   "#...........#", "#...........#", "#############"});
    const auto catalog = hybridCatalog(false);
    for (const Cell start : {Cell{5, 4}, Cell{8, 2}}) {
        const Player p{{5, 9}, Heading::North};
        auto archer = npc(start, Heading::South);
        archer.monster = catalog.findMonster("archer");
        NpcWorld places(map, p.cell, {"goblin"}, {archer});
        ActorWorld actors(map, catalog, places);
        NpcAiController ai(map, actors, 5);
        ai.alert(actors.id(0), p.cell);
        bool reached = false;
        for (int i = 1; i <= 80 && !reached; ++i) {
            ai.update(100, i * 100, p);
            const auto cell = actors.actor(actors.id(0))->npc.cell;
            reached = inPartyView(map, p.cell, p.heading, cell, actors.actor(actors.id(0))->npc.subcell);
        }
        const auto cell = actors.actor(actors.id(0))->npc.cell;
        check(reached && !(cell == start) && cellDistance(cell, p.cell) > 1,
              "Ranged-only pursuit finds a shot around blocked LOS or from outside range");
        for (int i = 81; i <= 120; ++i)
            ai.update(100, i * 100, p);
        check(actors.actor(actors.id(0))->npc.cell == cell,
              "Pursuit stops once a usable ranged position is reached");
    }
}
void diagonalBodyTargeting() {
    const auto map = hybridRoom();
    const auto catalog = hybridCatalog(false);
    for (unsigned h = 0; h < 4; ++h) {
        const Player p{{6, 6}, Heading(h)};
        const auto f = forward(p.heading), r = right(p.heading);
        auto sub = Cell{0, 1};
        for (unsigned turn = 0; turn < h; ++turn)
            sub = {1 - sub.y, sub.x};
        auto archer = npc(p.cell + f * 2 + r * 2, Heading((h + 2) % 4), sub);
        archer.monster = catalog.findMonster("archer");
        NpcWorld places(map, p.cell, {"goblin"}, {archer});
        GameSession game(map, catalog, places, 5, p, true);
        check(!inPartyView(game.geometry(), p.cell, p.heading, archer.cell),
              "Regression straddles the old cell-centre cone");
        check(inPartyView(game.geometry(), p.cell, p.heading, archer.cell, sub),
              "Visible diagonal body is inside the actual camera viewport");
        auto command = shot(game, game.actors().id(0));
        check(game.combat().validate(command, p) == CommandReason::Ready,
              "Bow can select the visible diagonal body in every heading");
        check(game.submit(command).event == CombatEvent::AttackStarted, "Opening shot alerts archer");
        bool replied = false;
        for (unsigned tick = 0; tick < 50 && !replied; ++tick)
            for (const auto& event : game.update(.1, false))
                replied |= event.event == CombatEvent::AttackStarted && event.enemyAttack;
        check(replied, "Archer can return fire from the same visible position");
        check(game.actors().actor(game.actors().id(0))->npc.cell == archer.cell,
              "Archer holds a usable firing position instead of wandering");
        FloorFixtures doors;
        doors.count = 1;
        doors.entries[0] = {p.cell + f + r, Heading::North, FixtureKind::Door, 0};
        FloorGeometry blocked(map, doors);
        check(!inPartyView(blocked.view(), p.cell, p.heading, archer.cell, sub),
              "Actor-aware viewport does not bypass a closed door");
    }
}
void detailsInput() {
    const auto m = hybridRoom();
    const auto c = hybridCatalog();
    const Player p{{5, 8}, Heading::North};
    NpcWorld places(m, p.cell, {"goblin"}, {npc({5, 4})});
    GameSession s(m, c, places, 5, p, false);
    CommandController ui(s);
    ui.refresh(allCombatTargets(s), false, false, true);
    UiInput input;
    const auto box = hudLayout::detail(2);
    input.pointer = ArtPoint{float(box.x + 1), float(box.y + 1)};
    routeHudInput(ui, input);
    check(!ui.snapshot().modal(), "Hovering details does not open a pause overlay");
    input.pressed[static_cast<std::size_t>(UiKey::Click)] = true;
    routeHudInput(ui, input);
    check(ui.snapshot().inventory.open && ui.snapshot().inventory.page == CharacterPage::Character &&
              ui.snapshot().inventory.character.index == 2,
          "Details click opens the chosen character");
}
void capacityAndBlockedEscape() {
    const Map blocked({"#####", "#####", "##.##", "#...#", "#####"});
    const auto c = hybridCatalog();
    const Player p{{2, 3}, Heading::North};
    NpcWorld trapped(blocked, p.cell, {"goblin"}, {npc({2, 2}, Heading::South, {0, 1})});
    GameSession s(blocked, c, trapped, 5, p, false);
    s.actors().damage(s.actors().id(0), 500, 0);
    s.update(0, false);

    bool attacked = false;
    for (int i = 0; i < 80 && !attacked; ++i) {
        if (s.combat().snapshot().awaitingAction)
            s.submit(turnCommand(s));
        else {
            const auto out = s.update(.5, false).step;
            attacked = out.event == CombatEvent::AttackStarted;
        }
    }
    check(attacked && s.actors().actor(s.actors().id(0))->npc.cell == Cell{2, 2},
          "Blocked escape falls back to attack without crossing walls");
    const auto m = hybridRoom();
    const Player center{{6, 6}, Heading::North};
    std::vector<NpcInstance> crowd;
    for (int y = 1; y < 12 && crowd.size() < NpcWorld::maxInstances; ++y)
        for (int x = 1; x < 12 && crowd.size() < NpcWorld::maxInstances; ++x) {
            const Cell cell{x, y};
            if (!inBowRange(cell, center.cell))
                continue;
            for (int slot = 0; slot < 4; ++slot)
                crowd.push_back(npc(cell, headingToward(cell, center.cell), {slot % 2, slot / 2}));
        }
    NpcWorld many(m, center.cell, {"goblin"}, crowd);
    GameSession full(m, c, many, 5, center, true);
    full.update(0, false);
    for (int i = 0; i < 250 && full.combat().snapshot().round < 2; ++i)
        tick(full);
    check(full.combat().snapshot().enemyCount == NpcWorld::maxInstances,
          "Encounter enrolls all 256 live actors without truncation");

    auto preview = full.combat().snapshot();
    full.combat().previewTurns(preview);
    check(preview.upcomingCount == 8, "Large encounter retains bounded HUD preview");
}
void traitAndPresentation() {
    const auto m = hybridRoom();
    const Player p{{5, 6}, Heading::North};
    for (bool enabled : {false, true}) {
        const auto c = hybridCatalog(enabled);
        NpcWorld places(m, p.cell, {"goblin"}, {npc({5, 5}, Heading::South, {0, 1})});
        GameSession s(m, c, places, 5, p, false);
        s.actors().damage(s.actors().id(0), 500, 0);
        s.update(0, false);

        bool acted = false, moved = false;
        for (int i = 0; i < 80 && !acted; ++i) {
            if (s.combat().snapshot().awaitingAction)
                s.submit(turnCommand(s));
            else {
                const auto out = s.update(.5, false).step;
                if (out.event == CombatEvent::Move) {
                    check(enabled, "Only the wounded Nimble Escape actor retreats");
                    moved = true;
                }
                if (out.event == CombatEvent::AttackStarted) {
                    check(enabled ? moved && out.kind == rules::AttackKind::Ranged
                                  : !moved && out.kind == rules::AttackKind::Melee,
                          "Catalog trait chooses retreat plus shot or an immediate melee attack");
                    acted = true;
                }
            }
        }
        check(acted, "Wounded actor takes turn");
        if (enabled)
            check(cellDistance(s.actors().actor(s.actors().id(0))->npc.cell, p.cell) == 2,
                  "Escape increases distance by one cell");
    }
    const auto c = hybridCatalog();
    NpcWorld places(m, p.cell, {"goblin"}, {npc({5, 3}, Heading::South)});
    NpcTimings t{};
    t[0] = {{600, 600, 0, 0}, 2};
    t[5] = {{600, 400, 0, 0}, 2};
    GameSession s(m, c, places, 5, p, true, {t});
    NpcAnimator animator(s.actors(), {t});
    const auto out = s.update(.1, false).step;
    check(out.event == CombatEvent::AttackStarted && out.kind == rules::AttackKind::Ranged &&
              out.impactMs == 600,
          "Bow outcome publishes release timing");
    animator.setTime(s.timeMs());
    check(animator.snapshot().frames[0].clip == NpcAnimation::RangedAttack, "Bow cue selects bow clip");
    s.update(.599, false);
    animator.setTime(s.timeMs());
    check(animator.snapshot().frames[0].active == 0, "Draw holds until release");
    s.update(.001, false);
    animator.setTime(s.timeMs());
    check(animator.snapshot().frames[0].active == 1, "Exact release frame");
    s.update(60, true);
    check(s.timeMs() == 700, "Pause freezes bow playback");
}
} // namespace
int main() {
    try {
        rangesAndCosts();
        facingAndFormation();
        handoff();
        facingPresentation();
        offscreenAndPartialRound();
        traitAndPresentation();
        enemySurpriseFeedback();
        interruptedRoundReadiness();
        reinforcementAndSaves();
        capacityAndBlockedEscape();
        threeCellFront();
        contactNeedsAnActionableFront();
        offscreenDiagonalHandoff();
        frontApproachInCombat();
        sideArcherFiringPositions();
        singleWeaponPolicies();
        rangedPursuit();
        diagonalBodyTargeting();
        detailsInput();
        std::cout << "Hybrid checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

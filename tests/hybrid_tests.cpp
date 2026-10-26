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
    check(out.event == CombatEvent::Attack, "Approach shot resolves");
    check(s.submit(command).reason == CommandReason::StaleTurn, "Repeated approach request rejected");
    check(s.ai().actors[0].awareness == Awareness::Pursuing &&
              s.ai().actors[1].awareness == Awareness::Pursuing &&
              s.ai().actors[2].awareness == Awareness::Routine,
          "Local alert reaches radius three without chaining");
    check(s.submit(shot(s, s.actors().id(0))).reason == CommandReason::Cooldown,
          "Fresh token cannot bypass cooldown");
    const auto clock = s.combat().snapshot().gameplayMs;
    s.update(60, true);
    check(s.combat().snapshot().gameplayMs == clock && s.combat().snapshot().cooldownMs[2] == 6000,
          "Pause freezes costs");
    for (int i = 0; i < 3; ++i)
        s.move(Action::Forward);
    check(s.combat().snapshot().phase == CombatPhase::Contact, "Adjacent approach enters contact");
    s.update(.999, false);
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
        if (st.activeCharacter && st.activeCharacter->index == 2) {
            check(!st.awaitingAction && st.spent[2], "Recent approach shot spends first-round action");
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
                const auto out = s.update(.2, false);
                if (out.event == CombatEvent::Attack && out.kind == rules::AttackKind::Melee) {
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
    check(s.submit(command).event == CombatEvent::Attack,
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
            attacked = s.update(.5, false).event == CombatEvent::Attack;
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
            const auto out = s.update(.5, false);
            if (out.event == CombatEvent::Attack)
                check(out.enemy.slot == 0, "Side and rear enemies cannot attack while front group remains");
        }
    }
    // Outside combat an alerted offscreen archer also cannot shoot.
    NpcWorld behind(m, p.cell, {"goblin"}, {npc({5, 9}, Heading::North)});
    GameSession approach(m, c, behind, 5, p, true);
    check(approach.update(.1, false).event != CombatEvent::Attack, "Offscreen approach archer cannot shoot");

    NpcWorld distant(m, p.cell, {"goblin"},
                     {npc({5, 5}, Heading::South, {0, 1}), npc({5, 2}, Heading::South)});
    GameSession partial(m, c, distant, 5, {{5, 8}, Heading::North}, false);
    partial.setAnimated(false);
    check(partial.submit(shot(partial, partial.actors().id(1))).event == CombatEvent::Attack,
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
    check(s.submit(immediate).event == CombatEvent::Attack,
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
    saves.update(0, false);
    check(saves.update(5.999, false).event != CombatEvent::DeathSave,
          "Real-time saves wait six gameplay seconds");
    check(saves.update(.001, false).event == CombatEvent::DeathSave,
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
            repeated += saves.update(.5, false).event == CombatEvent::DeathSave;
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
                            check(result.event == CombatEvent::Attack,
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
                    const auto out = s.update(.5, false);
                    if (out.event == CombatEvent::Attack) {
                        check(out.kind == rules::AttackKind::Melee && out.recipient.index < 2,
                              "Diagonal enemy melee attacks the protected formation's front row");
                        enemyMelee = true;
                    }
                }
                check(s.movement().player.heading == p.heading,
                      "Diagonal front occupants keep the current facing");
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
            moved = escape.update(.5, false).event == CombatEvent::Move;
    }
    check(moved && !combatAdjacent(map, p.cell, escape.actors().actor(escape.actors().id(0))->npc.cell),
          "Healthy diagonal archer escapes beyond the entire close-contact zone");
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
        for (int i = 0; i < 500 && melee < 2; ++i) {
            if (s.combat().snapshot().awaitingAction)
                s.submit(turnCommand(s));
            else {
                const auto out = s.update(.5, false);
                melee += out.event == CombatEvent::Attack && out.enemy.slot == 1 &&
                         out.kind == rules::AttackKind::Melee;
            }
            const auto* actor = s.actors().actor(s.actors().id(1));
            check(inPartyView(map, party.cell, party.heading, actor->npc.cell) &&
                      s.movement().player.heading == party.heading,
                  "Healthy approaching goblin stays in the frontal fight across rounds");
            check(actor->hitPoints == actor->maxHitPoints, "Route fixture stays at full health");
        }
        check(melee == 2 && s.actors().actor(s.actors().id(1))->npc.cell == front,
              "Approaching goblin joins the existing front group and keeps attacking");
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
            const auto out = ranged.update(.1, false);
            shots += out.event == CombatEvent::Attack;
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
                const auto out = melee.update(.1, false);
                if (out.event == CombatEvent::Attack) {
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
            const auto out = combat.update(.5, false);
            if (out.enemy.slot == 1 && out.event == CombatEvent::Attack) {
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
            const auto out = retreat.update(.5, false);
            check(out.event != CombatEvent::Attack, "Archer spends its action creating distance first");
            moved = out.event == CombatEvent::Move;
        }
    }
    check(moved && retreat.actors().actor(retreat.actors().id(0))->npc.cell == Cell{5, 6},
          "Full-health ranged-only NPC escapes without needing Nimble Escape");
    const auto deadline = retreat.combat().snapshot().gameplayMs + combatRoundMs;
    for (int i = 0; i < 30 && retreat.combat().snapshot().gameplayMs < deadline; ++i) {
        const auto out = retreat.update(.1, false);
        check(out.event != CombatEvent::Attack, "Escaping cannot also shoot before action cooldown");
    }

    const Map trapped({"#####", "#####", "##.##", "#...#", "#####"});
    archer.cell = {2, 2};
    NpcWorld blocked(trapped, {2, 3}, {"goblin"}, {archer});
    GameSession cornered(trapped, noTrait, blocked, 5, {{2, 3}, Heading::North}, false);
    bool fired = false;
    for (int i = 0; i < 100 && !fired; ++i) {
        if (cornered.combat().snapshot().awaitingAction)
            cornered.submit(turnCommand(cornered));
        else {
            const auto out = cornered.update(.5, false);
            fired = out.event == CombatEvent::Attack && out.kind == rules::AttackKind::Ranged;
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
            reached = inPartyView(map, p.cell, p.heading, cell);
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
    check(ui.snapshot().inspected && ui.snapshot().inspected->index == 2,
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
            const auto out = s.update(.5, false);
            attacked = out.event == CombatEvent::Attack;
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

        bool acted = false;
        for (int i = 0; i < 80 && !acted; ++i) {
            if (s.combat().snapshot().awaitingAction)
                s.submit(turnCommand(s));
            else {
                const auto out = s.update(.5, false);
                if (out.event == CombatEvent::Move || out.event == CombatEvent::Attack) {
                    check(enabled ? out.event == CombatEvent::Move : out.event == CombatEvent::Attack,
                          "Catalog trait controls escape, not creature name");
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
    const auto out = s.update(.1, false);
    check(out.event == CombatEvent::Attack && out.kind == rules::AttackKind::Ranged && out.impactMs == 600,
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
        reinforcementAndSaves();
        capacityAndBlockedEscape();
        threeCellFront();
        frontApproachInCombat();
        singleWeaponPolicies();
        rangedPursuit();
        detailsInput();
        std::cout << "Hybrid checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

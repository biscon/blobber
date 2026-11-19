#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/newt_fixtures.hpp"
#include <iostream>
using namespace blobber;
using namespace blobber::rules;
namespace {
using blobber::test::newtCatalog;
void hearing() {
    const Map map({"#########", "#.......#", "#...#...#", "#.......#", "#.......#", "#########"});
    const auto rules = newtCatalog();
    const Player spawn{{1, 3}, Heading::East};
    const NpcWorld npcs(map, spawn.cell, {"test.newt"}, {{{0}, {6, 3}, {0, 0}, Heading::West}});
    ActorWorld world(map, rules, npcs);
    NpcAiController ai(map, world, 4);
    const auto id = world.id(0);
    check(!ai.seesParty(id, {3, 3}), "A facing newt has no direct vision");
    ai.hearFootstep({2, 3});
    ai.hearFootstep({4, 2});
    ai.hearFootstep({4, 1});
    check(!ai.aware(id), "Distance four and an obstructed ray are silent");
    FloorFixtures fixtures;
    fixtures.count = 1;
    fixtures.entries[0] = {{5, 3}, Heading::East, FixtureKind::Door, 0};
    GameSession door(map, rules, npcs, 4, {{3, 3}, Heading::East}, true, {}, {}, fixtures);
    door.setAnimated(false);
    door.move(Action::Forward);
    check(door.ai().actors[0].awareness == Awareness::Routine, "Closed door screens a nearby step");
    door.move(Action::Forward);
    check(door.movement().blocked && door.ai().actors[0].awareness == Awareness::Routine,
          "Blocked movement emits no footstep");
    check(bool(door.interact(door.nearbyFixture())), "Open the door");
    door.move(Action::Back);
    check(door.ai().actors[0].lastSeen == Cell{3, 3}, "Open door passes a real footstep at radius three");
    ai.hearFootstep({7, 3});
    check(ai.aware(id) && ai.snapshot().actors[0].lastSeen == Cell{7, 3}, "Heard from behind");
    for (bool animated : {true, false}) {
        GameSession game(map, rules, npcs, 4, spawn, true);
        game.setAnimated(animated);
        game.move(Action::TurnRight);
        game.update(.2, false);
        game.move(Action::TurnLeft);
        game.update(.2, false);
        game.move(Action::Forward);
        if (animated)
            game.move(Action::Forward);
        game.update(.17, false);
        check(game.ai().actors[0].awareness == Awareness::Routine, "Quiet until the in-range step settles");
        if (!animated)
            game.move(Action::Forward);
        allocations = 0;
        watching = true;
        game.update(.17, false);
        watching = false;
        check(!allocations && game.ai().actors[0].lastSeen == Cell{3, 3},
              "Headless queued and immediate steps alert at the completed destination");
        auto previous = game.actors().snapshot().actors[0].npc.cell;
        std::uint64_t lastMove = 0;
        unsigned steps = 0;
        for (unsigned tick = 0; tick < 28; ++tick) {
            game.update(.1, false);
            check(game.ai().searchWork <= Navigation::nodeBudget &&
                      game.ai().completedSearches <= Navigation::requestBudget,
                  "Bounded hearing pursuit");
            const auto cell = game.actors().snapshot().actors[0].npc.cell;
            if (!(cell == previous)) {
                check(!steps || game.timeMs() - lastMove >= 1200, "Slow pursuit cadence");
                previous = cell;
                lastMove = game.timeMs();
                ++steps;
            }
        }
        check(steps >= 2, "Heard footsteps lead to contact");
    }
}

enum class Response {
    Defend,
    Ignore,
    DownTarget,
    LoseContact,
    KillDuringWarning,
    KillDuringStrike,
    Ward,
    Skip
};
CombatOutcome bite(Response response, unsigned seed, bool& roundCrossed) {
    const auto map = combatMap();
    const auto rules = newtCatalog();
    const Player pose{{3, 3}, Heading::North};
    const NpcWorld npcs(map, pose.cell, {"test.newt"}, {{{0}, {3, 2}, {0, 1}, Heading::South}});
    GameSession game(map, rules, npcs, seed, {{1, 5}, Heading::East}, false);
    for (std::uint16_t i = 1; i < partySize; ++i)
        game.party().damage({i}, 10000);
    if (response == Response::Skip) {
        auto floor = std::make_unique<FloorState>();
        game.exportFloor(*floor);
        floor->combat.partyReady[0] = 4 * combatRoundMs;
        game.importFloor(*floor);
    }
    game.restorePose(pose, false);
    const auto id = game.actors().id(0);
    CombatOutcome result;
    const auto hp = game.party().character({0})->hitPoints;
    unsigned warnings = 0, impacts = 0, commits = 0;
    bool acted = false, interrupted = false, skipped = false;
    std::uint64_t defendRound = 0;
    for (unsigned tick = 0; tick < 900 && !impacts && !interrupted; ++tick) {
        allocations = 0;
        watching = true;
        const auto& batch = game.update(.05, false);
        watching = false;
        check(!allocations, "Bite update does not allocate");
        bool warned = false, committed = false;
        for (const auto& e : batch) {
            if (e.event == CombatEvent::BiteWarning) {
                ++warnings;
                warned = true;
                check(e.recipient.index == 0 && !e.roll && !e.damage && game.saveSafety(),
                      "Warning names the target before rolling, during unsafe contact");
            }
            skipped |= warnings && !e.enemy.world && e.event == CombatEvent::Spent &&
                       e.spentCause == SpentCause::Cooldown;
            if (e.event == CombatEvent::AttackStarted && e.enemyAttack) {
                ++commits;
                committed = true;
                check(warnings == 1 && acted, "No commitment until a playable defender opportunity");
            }
            if (e.event == CombatEvent::Attack && e.enemyAttack) {
                ++impacts;
                result = e;
                const bool defended = response == Response::Defend;
                check(e.guardedBite == defended, "Only Defend during this warning guards the bite");
                if (e.hit) {
                    check(e.rawDamage >= (e.critical ? 9 : 6) && e.rawDamage <= (e.critical ? 39 : 21),
                          "Heavy bite damage includes the modifier once");
                    check(e.damage == (defended ? e.rawDamage / 2 : e.rawDamage),
                          "Defend halves the full rolled bite, rounded down");
                }
                check(game.party().character({0})->hitPoints == std::max<std::int64_t>(0, hp - e.damage),
                      "Resolved bite damage matches the victim's HP loss");
                check(commits == 1 && e.recipient.index == 0 &&
                          e.attackRollMode ==
                              (response == Response::Defend ? RollMode::Disadvantage : RollMode::Normal),
                      "Single impact honors the named victim's choice");
            }
            interrupted |= e.event == CombatEvent::BiteAbandoned || e.event == CombatEvent::AttackCanceled;
        }
        if (warned) {
            const auto clock = game.timeMs();
            game.update(10., true);
            check(game.timeMs() == clock && game.combat().warnedTarget(id).index == 0,
                  "Pause holds the warning");
            if (response == Response::Ward) {
                EffectRequest ward;
                ward.world = id.world;
                ward.caster = CharacterId{0};
                ward.kind = EffectKind::Sanctuary;
                ward.targets[0] = CharacterId{0};
                ward.count = 1;
                ward.difficulty = 100;
                check(bool(game.applyEffect(ward)), "Ward granted during the warning");
            } else if (response == Response::DownTarget) {
                game.party().damage({0}, 10000);
            } else if (response == Response::LoseContact)
                check(bool(game.actors().move(id, {3, 1}, {0, 1}, pose.cell)), "Move beyond contact");
            else
                game.actors().damage(id, response == Response::KillDuringWarning ? 100 : 1, clock);
        }
        if (committed && response == Response::KillDuringStrike)
            game.actors().damage(id, 100, game.timeMs());
        const auto& turn = game.combat().snapshot();
        if (committed && defendRound && turn.round > defendRound)
            roundCrossed = true;
        if (!turn.activeCharacter || !turn.awaitingAction)
            continue;
        auto c = turnCommand(game, PartyAction::Feature);
        c.feature = Feature::EndTurn;
        if (warnings) {
            acted = true;
            if (response == Response::Defend) {
                c = turnCommand(game, PartyAction::Defend);
                defendRound = turn.round;
            }
        }
        check(game.submit(c).reason == CommandReason::Resolved, "Defender's action accepted");
    }
    const bool canceled = response == Response::DownTarget || response == Response::LoseContact ||
                          response == Response::KillDuringWarning || response == Response::KillDuringStrike ||
                          response == Response::Ward;
    if (warnings != 1 || impacts != unsigned(!canceled) || interrupted != canceled)
        std::cerr << "response " << unsigned(response) << " seed " << seed << " warnings " << warnings
                  << " impacts " << impacts << " commits " << commits << " interrupted " << interrupted
                  << " phase " << unsigned(game.combat().snapshot().phase) << "\n";
    check(warnings == 1 && impacts == unsigned(!canceled) && interrupted == canceled,
          "Warning either resolves a single impact or cancels");
    check(response != Response::Skip || skipped, "A cooldown skip did not authorize the bite");
    return result;
}
} // namespace
int main() try {
    hearing();
    bool roundCrossed = false;
    // This seed rolls the same odd bite damage with either player response.
    const auto guarded = bite(Response::Defend, 41, roundCrossed);
    const auto full = bite(Response::Ignore, 41, roundCrossed);
    check(full.hit && guarded.hit && full.damage == guarded.rawDamage && full.damage % 2 == 1 &&
              guarded.damage == full.damage / 2,
          "The same bite roll costs half as much HP damage when defended");
    for (unsigned seed : {5u, 8u, 12u, 19u})
        bite(Response::Defend, seed, roundCrossed);
    check(bite(Response::Ignore, 19, roundCrossed).critical, "An unanswered critical uses the full critical roll");
    check(roundCrossed, "A bite remembers Defend across round turnover");
    for (auto response :
         {Response::Ignore, Response::DownTarget, Response::LoseContact, Response::KillDuringWarning,
          Response::KillDuringStrike, Response::Ward, Response::Skip})
        bite(response, 5, roundCrossed);
    std::cout << "Newt hearing, decision window and disruption passed.\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}

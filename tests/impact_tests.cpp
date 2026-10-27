#include "game/commands.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "support/hybrid_fixtures.hpp"
#include <chrono>
#include <iostream>
#include <string_view>
using namespace blobber;
namespace {
NpcTimings timings() {
    NpcTimings t{};
    t[0] = {{600, 600, 0, 0}, 2};
    t[2] = {{700, 250, 250, 350}, 4};
    t[3] = {{150, 150, 0, 0}, 2};
    t[4] = {{200, 200, 200, 600}, 4};
    t[5] = {{600, 400, 0, 0}, 2};
    return t;
}
rules::Catalog catalog(int hp = 1000, int ac = 1, int bonus = 100, rules::Dice damage = {1, 4, 0}) {
    auto f = partyFixture();
    for (auto& c : f.preset.characters) {
        c.equipment.offHand = {};
        const auto slot = c.itemCount++;
        c.items[slot] = {{3}, 1};
        c.equipment.ranged = {static_cast<std::uint16_t>(slot)};
    }
    rules::MonsterDefinition m;
    m.id = "goblin";
    m.abilities.fill(10);
    m.hitPoints = hp;
    m.armorClass = ac;
    m.attacks[0] = {0};
    m.attacks[1] = {1};
    m.attackCount = 2;
    return {{{"blade", rules::AttackKind::Melee, bonus, damage, rules::DamageType::Slashing},
             {"bow", rules::AttackKind::Ranged, bonus, damage, rules::DamageType::Piercing}},
            {m},
            f.races,
            f.classes,
            f.items,
            f.preset};
}
PartyCommand shot(GameSession& s, unsigned who = 2, unsigned target = 0) {
    auto c = turnCommand(s, PartyAction::Ranged);
    c.character = {static_cast<std::uint16_t>(who)};
    c.target = s.actors().id(target);
    return c;
}
CombatOutcome impact(const CombatEvents& events, std::uint64_t serial) {
    for (const auto& e : events)
        if (e.event == CombatEvent::Attack && e.attackId.serial == serial)
            return e;
    return {};
}
const AttackVisual& visual(const GameSession& s, const CombatOutcome& accepted) {
    for (const auto& v : s.combat().attacks().attacks)
        if (v.id.serial == accepted.attackId.serial)
            return v;
    throw std::runtime_error("Missing accepted attack");
}
void advanceTo(GameSession& s, std::uint64_t t) {
    check(t >= s.timeMs(), "Monotonic test clock");
    s.update(double(t - s.timeMs()) / 1000, false);
}
void partyArrows() {
    const auto map = hybridRoom();
    const auto c = catalog();
    const Player p{{5, 9}, Heading::North};
    NpcWorld places(map, p.cell, {"goblin"}, {{{0}, {5, 4}, {0, 0}, Heading::North}});
    bool hit = false, miss = false, critical = false;
    for (unsigned seed = 0; seed < 80 && !(hit && miss && critical); ++seed) {
        GameSession s(map, c, places, seed, p, false, {timings()});
        CommandController ui(s);
        auto targets = allCombatTargets(s);
        targets.anchors[0] = {200, 120, true};
        ui.refresh(targets, false, false, true);
        const auto hp = s.actors().actor(s.actors().id(0))->hitPoints;
        const auto command = shot(s);
        const auto accepted = s.submit(command);
        check(accepted.event == CombatEvent::AttackStarted && accepted.damage == 0 && accepted.roll == 0,
              "Acceptance hides pending result");
        ui.record(accepted);
        check(ui.snapshot().log.size() == 1, "Acceptance has no result log");
        check(s.submit(command).reason == CommandReason::StaleTurn, "Duplicate spends no action");
        const auto deadline = visual(s, accepted).impactMs;
        check(deadline == 396 && s.combat().snapshot().cooldownMs[2] == 6000,
              "Distance flight and start-time cost");
        ui.record(s.update(.395, false));
        check(s.actors().actor(*command.target)->hitPoints == hp, "HP unchanged until arrival");
        check(!s.actors().actor(*command.target)->hasDamage, "No premature reaction");
        const auto before = s.timeMs();
        s.update(60, true);
        check(s.timeMs() == before, "Pause freezes flight");
        const auto& events = s.update(.001, false);
        const auto result = impact(events, accepted.attackId.serial);
        ui.record(events);
        check(result.event == CombatEvent::Attack && result.occurredMs == deadline,
              "Impact exactly at deadline");
        check(s.actors().actor(*command.target)->hitPoints == hp - result.damage,
              "Damage committed with event");
        check(ui.snapshot().log.size() == 2, "Result log arrives with impact");
        hit |= result.hit;
        miss |= !result.hit;
        critical |= result.critical;
        const auto remaining = s.actors().actor(*command.target)->hitPoints;
        check(!impact(s.update(0, false), accepted.attackId.serial).attackId.serial,
              "Impact does not replay");
        s.update(1, false);
        check(s.actors().actor(*command.target)->hitPoints == remaining, "Exactly-once damage");
    }
    check(hit && miss && critical, "Fixed seeds exercise ranged hit miss critical");
}
void movementAndInvalidation() {
    const auto map = hybridRoom();
    const auto c = catalog();
    const Player p{{5, 9}, Heading::North};
    NpcWorld places(map, p.cell, {"goblin"}, {{{0}, {5, 4}, {0, 0}, Heading::North}});
    GameSession a(map, c, places, 5, p, false, {timings()}), b(map, c, places, 5, p, false, {timings()});
    const auto x = a.submit(shot(a)), y = b.submit(shot(b));
    a.setAnimated(false);
    a.move(Action::TurnRight);
    a.move(Action::Forward);
    const auto ar = impact(a.update(.396, false), x.attackId.serial),
               br = impact(b.update(.396, false), y.attackId.serial);
    check(ar.roll == br.roll && ar.damage == br.damage && ar.hit == br.hit,
          "Movement and turning preserve outcome/RNG");
    GameSession removed(map, c, places, 5, p, false, {timings()});
    const auto shotId = removed.submit(shot(removed));
    removed.actors().remove(removed.actors().id(0));
    const auto gone = impact(removed.update(.396, false), shotId.attackId.serial);
    check(gone.targetGone && gone.damage == 0,
          "Released arrow finishes against removed target without damage");
    GameSession restarted(map, c, places, 5, p, false, {timings()});
    check(restarted.combat().attacks().world != removed.combat().attacks().world,
          "Restart invalidates attack world");
    for (const auto& v : restarted.combat().attacks().attacks)
        check(v.stage == AttackStage::Empty, "Restart clears pending attacks");
}
void overlappingAndLethal() {
    const auto map = hybridRoom();
    const auto c = catalog(1);
    const Player p{{5, 9}, Heading::North};
    NpcWorld places(map, p.cell, {"goblin"}, {{{0}, {5, 4}, {0, 0}, Heading::North}});
    GameSession s(map, c, places, 5, p, false, {timings()});
    const auto a = s.submit(shot(s, 0)), b = s.submit(shot(s, 1)), d = s.submit(shot(s, 2)),
               e = s.submit(shot(s, 3));
    check(s.actors().occupied({5, 4}), "Lethal pending damage does not vacate occupancy");
    const auto& batch = s.update(.396, false);
    std::size_t count = 0;
    unsigned kills = 0;
    std::uint64_t previous = 0;
    for (const auto& out : batch)
        if (out.event == CombatEvent::Attack) {
            ++count;
            kills += out.died;
            check(out.attackId.serial > previous, "Equal-time impacts ordered by attack id");
            previous = out.attackId.serial;
        }
    check(count == 4 && kills == 1 && !s.actors().occupied({5, 4}),
          "Overlapping lethal shots resolve once each, one death");
    check(impact(batch, b.attackId.serial).targetGone && impact(batch, e.attackId.serial).targetGone,
          "Later shots never retarget");
    check(s.combat().snapshot().phase != CombatPhase::Victory, "Victory waits full reaction tail");
    advanceTo(s, 1595);
    check(s.combat().snapshot().phase != CombatPhase::Victory, "Corpse hold before boundary");
    s.update(.001, false);
    check(s.combat().snapshot().phase == CombatPhase::Victory, "Victory follows settled lethal impact");
    (void)a;
    (void)d;
    // Same initial commands, split versus skipped time: no lost or duplicated impacts.
    const auto sturdy = catalog();
    GameSession split(map, sturdy, places, 7, p, false, {timings()}),
        skipped(map, sturdy, places, 7, p, false, {timings()});
    for (unsigned i = 0; i < 4; ++i) {
        split.submit(shot(split, i));
        skipped.submit(shot(skipped, i));
    }
    for (int i = 0; i < 40; ++i)
        split.update(.01, false);
    const auto& many = skipped.update(60, false);
    count = 0;
    for (const auto& out : many)
        count += out.event == CombatEvent::Attack;
    check(count == 4 && split.actors().snapshot().actors[0].hitPoints ==
                            skipped.actors().snapshot().actors[0].hitPoints,
          "Large step drains all preexisting shots exactly");
}
CombatOutcome enemyStart(GameSession& s) {
    for (int i = 0; i < 200; ++i) {
        if (s.combat().snapshot().awaitingAction) {
            s.submit(turnCommand(s));
            continue;
        }
        const auto out = s.update(.1, false).step;
        if (out.event == CombatEvent::AttackStarted)
            return out;
    }
    throw std::runtime_error("Expected enemy attack");
}
void enemyStrikesAndArrows() {
    const auto map = hybridRoom();
    const auto c = catalog();
    const Player p{{5, 9}, Heading::North};
    for (bool ranged : {false, true})
        for (unsigned seed = 0; seed < 10; ++seed) {
            NpcWorld places(map, p.cell, {"goblin"},
                            {{{0}, ranged ? Cell{5, 5} : Cell{5, 8}, {0, 1}, Heading::South}});
            GameSession s(map, c, places, seed, p, true, {timings()});
            NpcAnimator animator(s.actors(), {timings()});
            const auto started = enemyStart(s);
            const auto v = visual(s, started);
            check(v.ranged == ranged, "Requested enemy attack kind");
            const auto hp = s.party().character(started.recipient)->hitPoints;
            advanceTo(s, v.releaseMs - 1);
            animator.setTime(s.timeMs());
            check(s.party().character(started.recipient)->hitPoints == hp,
                  "Enemy windup cannot damage early");
            const auto& release = s.update(.001, false);
            if (ranged) {
                bool released = false;
                for (const auto& out : release)
                    released |= out.event == CombatEvent::ProjectileReleased;
                check(released && s.party().character(started.recipient)->hitPoints == hp,
                      "Bow releases without early damage");
                advanceTo(s, v.impactMs - 1);
            }
            const auto result = ranged ? impact(s.update(.001, false), started.attackId.serial)
                                       : impact(release, started.attackId.serial);
            check(result.event == CombatEvent::Attack, "Both enemy kinds resolve at impact");
            check(s.party().character(started.recipient)->hitPoints == hp - result.damage,
                  "Party HP agrees with impact event");
        }
}
void interruption() {
    const auto map = hybridRoom();
    const auto c = catalog();
    const Player p{{5, 9}, Heading::North};
    NpcWorld places(map, p.cell, {"goblin"}, {{{0}, {5, 5}, {0, 0}, Heading::South}});
    GameSession s(map, c, places, 5, p, true, {timings()});
    NpcAnimator animator(s.actors(), {timings()});
    const auto enemy = enemyStart(s);
    const auto original = visual(s, enemy);
    const auto outgoing = s.submit(shot(s, 0));
    const auto arrival = visual(s, outgoing).impactMs;
    advanceTo(s, arrival - 1);
    const auto hit = impact(s.update(.001, false), outgoing.attackId.serial);
    check(hit.hit && hit.damage > 0, "Interruption fixture hits nonlethally");
    animator.setTime(s.timeMs());
    check(animator.snapshot().frames[0].clip == NpcAnimation::Hurt, "Hurt starts at incoming impact");
    check(visual(s, enemy).releaseMs == original.releaseMs + 300, "Windup deadline delayed by hurt interval");
    const auto second = s.submit(shot(s, 1));
    advanceTo(s, arrival + 300);
    animator.setTime(s.timeMs());
    check(animator.snapshot().frames[0].clip == NpcAnimation::RangedAttack &&
              animator.snapshot().frames[0].active == 0,
          "Draw resumes before release");
    advanceTo(s, visual(s, second).impactMs);
    const auto delayed = visual(s, enemy);
    advanceTo(s, delayed.releaseMs - 1);
    check(visual(s, enemy).stage == AttackStage::Windup, "Repeated hurt preserves unreleased shot");
    s.update(.001, false);
    animator.setTime(s.timeMs());
    check(visual(s, enemy).stage == AttackStage::Flight && animator.snapshot().frames[0].active == 1,
          "Resumed arrow and authored pose release together");
    s.actors().damage(s.actors().id(0), 100000, s.timeMs());
    const auto flight = visual(s, enemy);
    advanceTo(s, flight.impactMs - 1);
    check(impact(s.update(.001, false), enemy.attackId.serial).event == CombatEvent::Attack,
          "Released arrow survives shooter death");
    GameSession cancel(map, c, places, 5, p, true, {timings()});
    const auto unreleased = enemyStart(cancel);
    cancel.actors().damage(cancel.actors().id(0), 100000, cancel.timeMs());
    const auto& canceled = cancel.update(0, false);
    bool found = false;
    for (const auto& out : canceled)
        found |=
            out.event == CombatEvent::AttackCanceled && out.attackId.serial == unreleased.attackId.serial;
    check(found && visual(cancel, unreleased).stage == AttackStage::Canceled,
          "Incapacitation cancels before release");
    check(!impact(cancel.update(60, false), unreleased.attackId.serial).attackId.serial,
          "Canceled attack never damages");
}
void downedTargetsAndMutualDefeat() {
    const auto map = hybridRoom();
    const Player p{{5, 9}, Heading::North};
    NpcWorld places(map, p.cell, {"goblin"}, {{{0}, {5, 5}, {0, 0}, Heading::South}});
    const auto regular = catalog();
    GameSession downed(map, regular, places, 5, p, true, {timings()});
    const auto accepted = enemyStart(downed);
    const auto v = visual(downed, accepted);
    advanceTo(downed, v.releaseMs);
    const auto recipient = accepted.recipient;
    downed.party().damage(recipient, downed.party().character(recipient)->hitPoints);
    const rules::HealthState before = *downed.party().character(recipient);
    advanceTo(downed, v.impactMs - 1);
    const auto result = impact(downed.update(.001, false), accepted.attackId.serial);
    const auto expected = rules::takeDamage(before, downed.party().character(recipient)->stats.maxHitPoints,
                                            result.damage, result.critical);
    const rules::HealthState after = *downed.party().character(recipient);
    check(after.life == expected.state.life && after.failures == expected.state.failures,
          "In-flight damage applies existing downed-target rules");
    const auto lethal = catalog(1, 1, 100, {1, 4, 100});
    GameSession mutual(map, lethal, places, 5, p, true, {timings()});
    for (std::uint16_t i : {0, 1, 3})
        mutual.party().damage({i}, 10000);
    const auto incoming = enemyStart(mutual);
    const auto incomingVisual = visual(mutual, incoming);
    advanceTo(mutual, incomingVisual.releaseMs);
    const auto outgoing = mutual.submit(shot(mutual));
    check(outgoing.event == CombatEvent::AttackStarted, "Last survivor shoots before incoming impact");
    const auto& outcomes = mutual.update(.342, false);
    check(impact(outcomes, incoming.attackId.serial).died && impact(outcomes, outgoing.attackId.serial).died,
          "Opposing released arrows survive mutual incapacitation");
    mutual.update(1.2, false);
    check(mutual.combat().snapshot().phase == CombatPhase::Defeat,
          "Mutual elimination ends in defeat after reactions");
}
void overlappingHurtAndLongHolds() {
    const auto map = hybridRoom();
    const auto c = catalog();
    const Player p{{5, 9}, Heading::North};
    NpcWorld places(map, p.cell, {"goblin"}, {{{0}, {5, 5}, {0, 0}, Heading::South}});
    GameSession s(map, c, places, 5, p, true, {timings()});
    const auto enemy = enemyStart(s);
    const auto release = visual(s, enemy).releaseMs;
    const auto first = s.submit(shot(s, 0));
    s.update(.02, false);
    const auto second = s.submit(shot(s, 1));
    advanceTo(s, visual(s, first).impactMs);
    advanceTo(s, visual(s, second).impactMs);
    check(visual(s, enemy).releaseMs == release + 320,
          "Overlapping hurts extend their union rather than double counting");
    auto longTiming = timings();
    longTiming[3] = {{60000, 60000, 60000, 60000}, 4};
    GameSession held(map, c, places, 5, p, false, {longTiming});
    const auto arrow = held.submit(shot(held, 0));
    held.update(.342, false);
    held.setAnimated(false);
    held.move(Action::TurnRight);
    held.update(6, false);
    check(held.combat().snapshot().cooldownMs[0] == 0, "Cooldown is independent of presentation hold");
    check(held.submit(shot(held, 0)).reason == CommandReason::AttackPending,
          "Unfinished attacker cannot overwrite bounded slot after cooldown");
    check(visual(held, arrow).id.serial == arrow.attackId.serial,
          "Rejected replacement retains original attack");
}
void partyMelee() {
    const auto map = combatMap();
    const auto c = combatCatalog(1, -100, {1, 4, -100}, 1);
    const auto places = combatPlacements(map, {{3, 3}, Heading::North}, 1);
    GameSession s(map, c, places, 5, {{3, 3}, Heading::North}, false, {timings()});
    for (int i = 0; i < 100 && !(s.combat().snapshot().awaitingAction &&
                                 s.combat().snapshot().activeCharacter->index == 0);
         ++i) {
        if (s.combat().snapshot().awaitingAction)
            s.submit(turnCommand(s));
        else
            s.update(.5, false);
    }
    auto cmd = turnCommand(s, PartyAction::Melee);
    cmd.target = s.actors().id(0);
    const auto start = s.submit(cmd);
    check(start.event == CombatEvent::AttackStarted, "Party melee accepted");
    s.update(.199, false);
    check(s.actors().occupied({3, 2}), "Party melee lethal occupancy retained until 200ms");
    const auto result = impact(s.update(.001, false), start.attackId.serial);
    check(result.died && !s.actors().occupied({3, 2}), "Party melee kills at exact strike");
    s.update(1.199, false);
    check(s.combat().snapshot().resolvingAction, "Party death reaction hold");
    s.update(.001, false);
    check(s.combat().snapshot().phase == CombatPhase::Victory, "Victory follows full corpse hold");
}
// Deliberately separate from CTest: fill every attacker slot via public owners,
// then drain a whole burst in one update. The viewpoint visits each shooter
// during setup; this is a capacity stress, not a normal movement/AI scenario.
void stress() {
    std::vector<std::string> rows(40, std::string(40, '.'));
    for (int i = 0; i < 40; ++i)
        rows[0][i] = rows[39][i] = rows[i][0] = rows[i][39] = '#';
    const Map map(rows);
    const Player player{{20, 30}, Heading::North};
    const auto c = catalog(1000, 1, -100, {1, 4, -100});
    std::vector<NpcInstance> instances;
    for (int y = 1; y <= 16; ++y)
        for (int x = 1; x <= 16; ++x)
            instances.push_back({{0}, {x, y}, {0, 0}, Heading::South});
    const NpcWorld placements(map, player.cell, {"goblin"}, instances);
    double totalUs = 0, maxUs = 0;
    constexpr int repetitions = 100;
    for (int repeat = 0; repeat < repetitions; ++repeat) {
        Party party(c);
        ActorWorld actors(map, c, placements);
        rules::DiceRng rng(5);
        NpcAiController ai(map, actors, 5);
        EffectController effects(party, actors, rng);
        CombatEvents events;
        CombatController combat(map, party, actors, rng, ai, effects, {timings()});
        Player pose = player;
        for (std::size_t i = 0; i < NpcWorld::maxInstances; ++i) {
            pose.cell = {instances[i].cell.x, instances[i].cell.y + 2};
            ai.alert(actors.id(i), pose.cell);
            combat.observe(pose);
            check(combat.update(pose, false, 0, events).event == CombatEvent::AttackStarted,
                  "Every NPC books its own pending slot");
        }
        for (std::uint16_t i = 0; i < rules::partySize; ++i) {
            PartyCommand command;
            command.character = {i};
            command.action = PartyAction::Ranged;
            command.target = actors.id(NpcWorld::maxInstances - 1);
            command.world = combat.snapshot().world;
            command.turn = combat.snapshot().turn;
            check(combat.submit(command, pose, 0, events).event == CombatEvent::AttackStarted,
                  "Four party attacks coexist with full NPC population");
        }
        events.clear();
        const auto start = std::chrono::steady_clock::now();
        combat.resolveImpacts(pose, 3000, events);
        const auto elapsed =
            std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
        totalUs += elapsed;
        maxUs = std::max(maxUs, elapsed);
        unsigned impacts = 0, releases = 0;
        for (const auto& event : events) {
            impacts += event.event == CombatEvent::Attack;
            releases += event.event == CombatEvent::ProjectileReleased;
        }
        check(impacts == attackCapacity && releases == attackCapacity,
              "Full burst delivers all 520 release/impact events without loss");
        combat.resolveImpacts(pose, 3000, events);
        check(events.count == 520, "Full burst never repeats consequences");
    }
    std::cout << "260 attacks, 520 events, " << repetitions
              << " bursts; drain mean_us=" << totalUs / repetitions << " max_us=" << maxUs << '\n';
}

} // namespace
int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--stress") {
            stress();
            return 0;
        }
        partyArrows();
        movementAndInvalidation();
        overlappingAndLethal();
        enemyStrikesAndArrows();
        interruption();
        downedTargetsAndMutualDefeat();
        overlappingHurtAndLongHolds();
        partyMelee();
        std::cout << "Impact checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

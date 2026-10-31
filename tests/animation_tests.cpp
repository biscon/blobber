#include "support/actor_fixtures.hpp"
#include "support/animation_fixtures.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include <iostream>
#include <limits>
using namespace blobber;
using namespace blobber::test;
namespace walking {
const Map map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
ActorWorld world(NpcAnimation animation, std::uint32_t phase = 0) {
    return testActors(map, {3, 5}, {"test"}, {{{0}, {3, 3}, {0, 0}, Heading::South, animation, phase}});
}
void timing() {
    const auto metadata = animatedTimings();
    const auto npcs = world(NpcAnimation::Attack);
    NpcAnimator animator(npcs, {metadata});
    for (const auto& pair : std::array<std::array<std::uint64_t, 2>, 10>{{{0, 0},
                                                                          {69, 0},
                                                                          {70, 1},
                                                                          {94, 1},
                                                                          {95, 2},
                                                                          {119, 2},
                                                                          {120, 3},
                                                                          {154, 3},
                                                                          {155, 0},
                                                                          {1000, 1}}}) {
        animator.setTime(pair[0]);
        check(animator.snapshot().frames[0].active == pair[1], "Attack timing boundaries and wraparound");
        check(animator.snapshot().frames[0].idle == (pair[0] % 120 >= 60),
              "Fallback has independent idle timeline");
    }
    animator.setTime(0);
    const auto revision = animator.snapshot().revision;
    animator.setTime(59);
    check(animator.snapshot().revision == revision, "No revision change within held frames");
    animator.setTime(60);
    check(animator.snapshot().revision == revision + 1, "Fallback change publishes new selection");
    const auto phased = world(NpcAnimation::Walk, std::numeric_limits<std::uint32_t>::max());
    NpcAnimator phase(phased, {metadata});
    const auto huge = std::numeric_limits<std::uint64_t>::max();
    phase.setTime(huge);
    check(phase.snapshot().frames[0].active ==
              ((huge % 60 + phased.snapshot().actors[0].npc.phaseMs % 60) % 60 >= 30),
          "Large elapsed time and phase cannot overflow or require catch-up loops");
    phase.setTime(0);
    check(phase.snapshot().frames[0].active == (phased.snapshot().actors[0].npc.phaseMs % 60 >= 30),
          "Deterministic seek resets time");
    const auto idle = world(NpcAnimation::Idle);
    NpcAnimator still(idle, {staticTimings()});
    still.setTime(huge);
    check(still.snapshot().revision == 0 && still.snapshot().frames[0].active == 0,
          "Legacy static idle never advances");
    rejects([&] { NpcAnimator bad(npcs, {}); }, "Metadata count mismatch");
    rejects([&] { NpcAnimator bad(npcs, {staticTimings()}); }, "Absent selected clip rejected");
    auto bad = metadata;
    bad[0].durationMs[0] = 0;
    rejects([&] { NpcAnimator invalid(idle, {bad}); }, "Zero duration rejected");
    bad = metadata;
    bad[2].count = 5;
    rejects([&] { NpcAnimator invalid(idle, {bad}); }, "Bounded frame count");
    auto empty = testActors(map, {1, 1}, {}, {});
    NpcAnimator none(empty, {});
    none.setTime(huge);
    check(none.snapshot().count == 0 && none.snapshot().revision == 0, "Empty world clock");
}
void actualWalking() {
    auto actors = world(NpcAnimation::Idle);
    const auto id = actors.id(0);
    auto timings = reactionTimings();
    NpcAnimator animator(actors, {timings});
    check(actors.walk(id, {3, 2}, {0, 0}, {3, 5}, 100).changed, "Successful cross-cell walk commits");
    animator.setTime(100);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Walk,
          "Successful move triggers walk at frame zero");
    animator.setTime(699);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Walk, "Walk lasts 600 ms");
    animator.setTime(700);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Idle, "Walk expires to idle");
    check(!actors.walk(id, {0, 0}, {0, 0}, {3, 5}, 800), "Wall step rejected");
    animator.setTime(800);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Idle, "Rejected step cannot animate walking");
    actors.walk(id, {3, 3}, {0, 0}, {3, 5}, 900);
    actors.attack(id, 950);
    animator.setTime(950);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Attack, "Attack interrupts walk");
    animator.setTime(1200);
    check(animator.snapshot().frames[0].clip != NpcAnimation::Walk,
          "Canceled walk cannot resume after attack");
    actors.walk(id, {3, 2}, {0, 0}, {3, 5}, 1500);
    actors.damage(id, 1, 1550);
    animator.setTime(1550);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Hurt, "Hurt interrupts walk");
    animator.setTime(2050);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Idle, "Damage canceled old walking");
    NpcAnimator legacy(actors, {staticTimings()});
    actors.walk(id, {3, 3}, {0, 0}, {3, 5}, 2200);
    legacy.setTime(2200);
    check(legacy.snapshot().frames[0].clip == NpcAnimation::Idle, "Missing walk art remains idle");
    actors.damage(id, 10000, 2250);
    animator.setTime(2250);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Death, "Lethal damage interrupts walk");
    check(!actors.walk(id, {3, 2}, {0, 0}, {3, 5}, 2300), "Dead actor cannot walk");
    auto old = world(NpcAnimation::Idle);
    NpcAnimator oldAnimator(old, {animatedTimings()});
    old.walk(old.id(0), {3, 2}, {0, 0}, {3, 5}, 100);
    old.damage(old.id(0), 10000, 120);
    oldAnimator.setTime(150);
    check(oldAnimator.snapshot().frames[0].clip == NpcAnimation::Walk &&
              oldAnimator.snapshot().frames[0].flash,
          "Legacy lethal flash freezes the walking pose");
}
void travelTiming() {
    auto actors = world(NpcAnimation::Idle);
    const auto id = actors.id(0);
    NpcAnimator animator(actors, {staticTimings()});
    actors.walk(id, {4, 3}, {0, 0}, {3, 5}, 100);
    check(actors.occupied({4, 3}) && !actors.occupied({3, 3}), "Travel commits grid collision immediately");
    animator.setTime(100);
    check(animator.snapshot().frames[0].offsetX == -1, "Travel starts at source, including without walk art");
    const auto revision = animator.snapshot().revision;
    animator.setTime(101);
    check(animator.snapshot().revision > revision, "Position invalidates a held animation frame");
    animator.setTime(400);
    check(animator.snapshot().frames[0].offsetX == -.5f, "Halfway position is halfway between cells");
    const auto held = animator.snapshot().revision;
    animator.setTime(400);
    check(animator.snapshot().revision == held, "Paused presentation reuses the same position");
    animator.setTime(700);
    check(animator.snapshot().frames[0].offsetX == 0, "Arrival has exact destination position");
    actors.walk(id, {4, 2}, {1, 1}, {3, 5}, 700);
    const auto midway = actorPosition(*actors.actor(id), 1000);
    check(midway.x == 4.5f && midway.y == 3.f, "Travel includes changed subcell coordinates");
    actors.walk(id, {3, 2}, {0, 0}, {3, 5}, 1000);
    const auto chained = actorPosition(*actors.actor(id), 1000);
    check(chained.x == midway.x && chained.y == midway.y, "Early chained step starts without snapping");
    actors.move(id, {3, 3}, {0, 0}, {3, 5});
    check(actorPosition(*actors.actor(id), 1001).y == 3.25f, "Explicit move cancels travel");
    actors.walk(id, {3, 2}, {0, 0}, {3, 5}, 1100);
    actors.damage(id, 10000, 1400);
    const auto dead = actorPosition(*actors.actor(id), 1400);
    check(dead.y == 2.75f && actorPosition(*actors.actor(id), 5000).y == dead.y,
          "Lethal damage freezes travel where the actor died");
}
void staggeredIdle() {
    const std::vector<NpcInstance> placements{{{0}, {3, 3}, {0, 0}, Heading::South},
                                              {{0}, {3, 3}, {1, 0}, Heading::South},
                                              {{0}, {3, 3}, {0, 1}, Heading::South},
                                              {{0}, {3, 3}, {1, 1}, Heading::South, NpcAnimation::Idle, 60},
                                              {{0}, {4, 3}, {0, 0}, Heading::South, NpcAnimation::Attack}};
    auto actors = testActors(map, {3, 5}, {"test"}, placements);
    auto rebuilt = testActors(map, {3, 5}, {"test"}, placements);
    const auto metadata = animatedTimings();
    NpcAnimator animator(actors, {metadata}), repeat(rebuilt, {metadata});
    std::array<int, 3> firstChange{{-1, -1, -1}};
    const auto initial = animator.snapshot().frames;
    for (std::uint64_t time = 0; time <= 120; ++time) {
        animator.setTime(time);
        repeat.setTime(time);
        for (std::size_t i = 0; i < placements.size(); ++i) {
            const auto& frame = animator.snapshot().frames[i];
            const auto& again = repeat.snapshot().frames[i];
            check(frame.active == again.active && frame.idle == again.idle,
                  "Reconstructed worlds have deterministic phases despite different world IDs");
            if (i < 3 && frame.active != initial[i].active && firstChange[i] < 0)
                firstChange[i] = static_cast<int>(time);
            if (i < 4)
                check(frame.active == frame.idle, "Idle playback and directional fallback share phase");
        }
        check(animator.snapshot().frames[3].active == ((time + 60) % 120 >= 60),
              "Explicit idle phase overrides automatic staggering");
        const auto attack = time < 70 ? 0 : time < 95 ? 1 : time < 120 ? 2 : 3;
        check(animator.snapshot().frames[4].active == attack,
              "Automatic idle staggering does not alter authored attack timing");
    }
    check(firstChange[0] > 0 && firstChange[1] > 0 && firstChange[2] > 0 &&
              firstChange[0] != firstChange[1] && firstChange[0] != firstChange[2] &&
              firstChange[1] != firstChange[2],
          "Default idle actors transition at different times within one cycle");
    check(bool(actors.remove(actors.id(0))), "Remove earlier slot");
    check(bool(actors.move(actors.id(1), {2, 3}, {0, 0}, {3, 5})), "Move phased actor");
    for (std::uint64_t time = 0; time < 120; ++time) {
        animator.setTime(time);
        repeat.setTime(time);
        for (std::size_t i = 1; i < placements.size(); ++i)
            check(animator.snapshot().frames[i].idle == repeat.snapshot().frames[i].idle,
                  "Movement and removal preserve surviving idle phases, including attack fallback");
    }
    animator.setTime(53);
    const auto frozen = animator.snapshot().frames[1];
    check(actors.damage(actors.id(1), 7, 53).died, "Lethal damage fixture");
    for (std::uint64_t time : {53u, 100u, 152u}) {
        animator.setTime(time);
        const auto& frame = animator.snapshot().frames[1];
        check(frame.visible && frame.flash && frame.active == frozen.active && frame.idle == frozen.idle,
              "Death flash holds the staggered pose from hit time");
    }
    animator.setTime(153);
    check(!animator.snapshot().frames[1].visible && !animator.snapshot().frames[1].flash,
          "Idle phase does not shift the 100 ms death flash lifetime");
}
} // namespace walking
namespace reactions {
const Map map = combatMap();
ActorWorld world() {
    return testActors(map, {3, 5}, {"test"}, {{{0}, {3, 3}, {0, 0}, Heading::South}});
}
void timing() {
    auto actors = world();
    const auto metadata = reactionTimings();
    NpcAnimator animator(actors, {metadata});
    const auto id = actors.id(0);
    actors.attack(id, 900);
    check(actors.damage(id, 0, 1000).changed == false, "Zero damage has no cue");
    animator.setTime(1000);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Attack, "Zero damage leaves attack intact");
    actors.damage(id, 1, 1000);
    for (const auto ms : {0u, 99u, 100u, 149u, 150u, 299u, 300u, 10000u}) {
        animator.setTime(1000 + ms);
        const auto& f = animator.snapshot().frames[0];
        check(f.visible && f.flash == (ms < 100), "Hurt preserves 100 ms flash overlay");
        check(f.clip == (ms < 300 ? NpcAnimation::Hurt : NpcAnimation::Idle), "Hurt expires without looping");
        if (ms < 300)
            check(f.active == (ms >= 150), "Recoil then recovery at exact boundary");
    }
    actors.attack(id, 2000);
    actors.damage(id, 1, 2000);
    animator.setTime(2000);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Hurt, "Damage wins equal timestamp");
    actors.attack(id, 2050);
    animator.setTime(2050);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Attack, "New attack interrupts hurt");
    actors.damage(id, 1, 2075);
    actors.damage(id, 1, 2200);
    animator.setTime(2200);
    check(animator.snapshot().frames[0].active == 0 &&
              animator.snapshot().frames[0].clip == NpcAnimation::Hurt,
          "Repeated damage restarts hurt");
    // A long attack would still be playing after hurt expires: it must stay canceled.
    auto longMetadata = metadata;
    longMetadata[2].durationMs = {1000, 1000, 1000, 1000};
    NpcAnimator longAttack(actors, {longMetadata});
    longAttack.setTime(2500);
    check(longAttack.snapshot().frames[0].clip == NpcAnimation::Idle, "Interrupted attack never resumes");
    check(actors.damage(id, 999, 2600).died && !actors.occupied({3, 3}),
          "Death unblocks before presentation");
    for (const auto ms : {0u, 99u, 100u, 199u, 200u, 399u, 400u, 599u, 600u, 1199u, 1200u, 9000u}) {
        animator.setTime(2600 + ms);
        const auto& f = animator.snapshot().frames[0];
        check(f.visible == (ms < 1200) && f.flash == (ms < 100), "Death visibility and flash boundaries");
        if (f.visible)
            check(f.clip == NpcAnimation::Death && f.active == (ms < 200   ? 0
                                                                : ms < 400 ? 1
                                                                : ms < 600 ? 2
                                                                           : 3),
                  "Death advances collapse, holds corpse and never falls back to living art");
    }
    animator.setTime(3300);
    const auto revision = animator.snapshot().revision;
    animator.setTime(3799);
    check(animator.snapshot().revision == revision, "Final corpse hold does not invalidate on idle phases");
    animator.setTime(std::numeric_limits<std::uint64_t>::max());
    check(!animator.snapshot().frames[0].visible, "Huge time jump hides without wraparound");
    animator.setTime(2700);
    check(animator.snapshot().frames[0].visible, "Explicit diagnostic seek reconstructs death");
    actors.remove(id);
    animator.setTime(2700);
    check(!animator.snapshot().frames[0].visible, "Removal cancels death immediately");
    for (const auto clip : {NpcAnimation::Hurt, NpcAnimation::Death})
        rejects([&] { testActors(map, {3, 5}, {"test"}, {{{0}, {3, 3}, {0, 0}, Heading::South, clip}}); },
                "Reaction cannot be a placement loop");
}
void clocks() {
    const auto catalog = combatCatalog();
    const auto placements = combatPlacements(map, {{3, 3}, Heading::North}, 1);
    GameSession session(map, catalog, placements, 5, {{3, 3}, Heading::North});
    NpcAnimator animator(session.actors(), {reactionTimings()});
    session.actors().damage(session.actors().id(0), 7, session.timeMs());
    session.update(.2, false);
    animator.setTime(session.timeMs());
    check(session.combat().snapshot().phase == CombatPhase::Victory &&
              animator.snapshot().frames[0].active == 1,
          "Victory does not wait for death animation");
    session.update(10, true);
    animator.setTime(session.timeMs());
    check(animator.snapshot().frames[0].active == 1, "Pause freezes reactions");
    check(session.combat().continueExploring(), "Continue accepted during collapse");
    session.update(.4, false);
    animator.setTime(session.timeMs());
    check(animator.snapshot().frames[0].active == 3 && animator.snapshot().frames[0].visible,
          "Continue preserves corpse clock");
    session.update(.6, false);
    animator.setTime(session.timeMs());
    check(!animator.snapshot().frames[0].visible, "Continued exploration expires corpse");
    GameSession restarted(map, catalog, placements, 5, {{3, 3}, Heading::North});
    NpcAnimator fresh(restarted.actors(), {reactionTimings()});
    check(fresh.snapshot().world != animator.snapshot().world &&
              fresh.snapshot().frames[0].clip == NpcAnimation::Idle && fresh.snapshot().frames[0].visible,
          "Restart has independent lifetime and no stale reaction");
}
} // namespace reactions
int main() {
    try {
        walking::timing();
        walking::actualWalking();
        walking::travelTiming();
        walking::staggeredIdle();
        reactions::timing();
        reactions::clocks();
        std::cout << "Animation clocks, interruptions and lifetime checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

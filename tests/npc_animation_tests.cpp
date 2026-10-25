#include "render/frame_pipeline.hpp"
#include "render/transitions.hpp"
#include "support/actor_fixtures.hpp"
#include "support/check.hpp"
#include "support/npc_fixtures.hpp"
#include "support/render_fixtures.hpp"
#include <iostream>
#include <limits>
using namespace blobber;
namespace {
const Map map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
ActorWorld world(NpcAnimation animation, std::uint32_t phase = 0) {
    return testActors(map, {3, 5}, {"test"}, {{{0}, {3, 3}, {0, 0}, Heading::South, animation, phase}});
}
void timing() {
    const auto metadata = npcTimings(testAnimatedNpcAppearance());
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
    NpcAnimator still(idle, {npcTimings(testNpcAppearance())});
    still.setTime(huge);
    check(still.snapshot().revision == 0 && still.snapshot().frames[0].active == 0,
          "Legacy static idle never advances");
    rejects([&] { NpcAnimator bad(npcs, {}); }, "Metadata count mismatch");
    rejects([&] { NpcAnimator bad(npcs, {npcTimings(testNpcAppearance())}); },
            "Absent selected clip rejected");
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
void staggeredIdle() {
    const std::vector<NpcInstance> placements{{{0}, {3, 3}, {0, 0}, Heading::South},
                                              {{0}, {3, 3}, {1, 0}, Heading::South},
                                              {{0}, {3, 3}, {0, 1}, Heading::South},
                                              {{0}, {3, 3}, {1, 1}, Heading::South, NpcAnimation::Idle, 60},
                                              {{0}, {4, 3}, {0, 0}, Heading::South, NpcAnimation::Attack}};
    auto actors = testActors(map, {3, 5}, {"test"}, placements);
    auto rebuilt = testActors(map, {3, 5}, {"test"}, placements);
    const auto metadata = npcTimings(testAnimatedNpcAppearance());
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
void triggeredAttacks() {
    const auto appearance = testAnimatedNpcAppearance();
    auto actors = world(NpcAnimation::Idle);
    const auto idleActors = world(NpcAnimation::Idle);
    NpcAnimator animator(actors, {npcTimings(appearance)}), idle(idleActors, {npcTimings(appearance)});
    const SceneRenderer renderer(testMaterials(), {appearance});
    Frame attackFrame, idleFrame;
    const auto id = actors.id(0);
    const auto hp = actors.actor(id)->hitPoints;
    check(actors.attack(id, 1000).changed, "Attack cue independent of damage");
    for (const auto elapsed : {0u, 69u, 70u, 94u, 95u, 119u, 120u, 154u, 155u, 310u}) {
        animator.setTime(1000 + elapsed);
        const auto& frame = animator.snapshot().frames[0];
        check(frame.clip == (elapsed < 155 ? NpcAnimation::Attack : NpcAnimation::Idle),
              "One-shot attack plays to completion then returns to idle without looping");
        if (elapsed < 155)
            check(frame.active == (elapsed < 70    ? 0
                                   : elapsed < 95  ? 1
                                   : elapsed < 120 ? 2
                                                   : 3),
                  "Triggered clip uses relative time without placement phase");
    }
    animator.setTime(1070);
    idle.setTime(1070);
    renderer.compose(map, {{3, 5}, Heading::North}, attackFrame, &actors, &animator.snapshot());
    renderer.compose(map, {{3, 5}, Heading::North}, idleFrame, &idleActors, &idle.snapshot());
    check(!same(attackFrame, idleFrame) && actors.actor(id)->hitPoints == hp,
          "Miss animation changes actual prepared pixels without health mutation");
    renderer.compose(map, {{5, 3}, Heading::West}, attackFrame, &actors, &animator.snapshot());
    renderer.compose(map, {{5, 3}, Heading::West}, idleFrame, &idleActors, &idle.snapshot());
    check(same(attackFrame, idleFrame), "Missing attack direction uses current idle frame");
    const auto revision = actors.snapshot().revision;
    check(actors.attack(id, 999).error == ActorError::InvalidTime &&
              actors.attack(idleActors.id(0), 1100).error == ActorError::InvalidHandle &&
              actors.snapshot().revision == revision,
          "Invalid cues leave actor state unchanged");
    check(actors.attack(id, 1100).changed, "A new attack restarts the clip");
    animator.setTime(1100);
    check(animator.snapshot().frames[0].active == 0, "Retrigger starts on frame zero");
    actors.damage(id, hp, 1170);
    animator.setTime(1190);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Attack &&
              animator.snapshot().frames[0].active == 1 && animator.snapshot().frames[0].flash,
          "Death freezes triggered attack pose at the lethal timestamp");
    check(actors.attack(id, 1200).error == ActorError::NotAlive, "Dead actor cannot start an attack");
    animator.setTime(1270);
    check(!animator.snapshot().frames[0].visible, "Death expiry still hides attacking body");
    auto noArt = world(NpcAnimation::Idle);
    NpcAnimator fallback(noArt, {npcTimings(testNpcAppearance())});
    noArt.attack(noArt.id(0), 0);
    fallback.setTime(10);
    check(fallback.snapshot().frames[0].clip == NpcAnimation::Idle,
          "Missing entire attack clip safely retains idle");
}
void rendering() {
    const auto appearance = testAnimatedNpcAppearance();
    const auto npcs = world(NpcAnimation::Attack);
    const auto idleWorld = world(NpcAnimation::Idle);
    NpcAnimator animator(npcs, {npcTimings(appearance)}), idle(idleWorld, {npcTimings(appearance)});
    const SceneRenderer renderer(testMaterials(), {appearance});
    Frame a, b, source, destination;
    const std::array<Player, 4> cameras{{{{3, 5}, Heading::North},
                                         {{3, 1}, Heading::South},
                                         {{5, 3}, Heading::West},
                                         {{1, 3}, Heading::East}}};
    animator.setTime(70);
    idle.setTime(70);
    for (std::size_t side = 0; side < cameras.size(); ++side) {
        renderer.compose(map, cameras[side], a, &npcs, &animator.snapshot());
        renderer.compose(map, cameras[side], b, &idleWorld, &idle.snapshot());
        check(same(a, b) == (side != 0),
              "Front attack differs; absent directions exactly match current idle");
    }
    MovementState state;
    state.player = cameras[0];
    FramePipeline pipeline;
    animator.setTime(0);
    check(pipeline.update(renderer, map, state, &npcs, &animator.snapshot()),
          "First animation frame rendered");
    a.copyFrom(pipeline.frame());
    animator.setTime(59);
    check(!pipeline.update(renderer, map, state, &npcs, &animator.snapshot()),
          "Stationary held frame reused");
    animator.setTime(70);
    check(pipeline.update(renderer, map, state, &npcs, &animator.snapshot()) && !same(a, pipeline.frame()),
          "Stationary player sees NPC frame changes");
    // Every movement effect must use one shared time for both endpoints, including
    // ticks during an already active transition and animation-mode switching.
    for (int action = 0; action < 6; ++action) {
        const auto kind = static_cast<Action>(action);
        Player from{{3, 4}, Heading::North}, to = from;
        check(tryAction(map, to, kind), "Fixture transition possible");
        state.player = to;
        state.revision++;
        for (std::uint64_t time : {0u, 70u, 95u, 120u, 155u}) {
            state.transition = {from, to, kind, Transition::duration * 0.5f};
            animator.setTime(time);
            pipeline.update(renderer, map, state, &npcs, &animator.snapshot());
            if (isStrafe(kind))
                renderer.composeStrafe(map, from, to, 0.5f, b, &npcs, &animator.snapshot());
            else {
                renderer.compose(map, from, source, &npcs, &animator.snapshot());
                renderer.compose(map, to, destination, &npcs, &animator.snapshot());
                composeTransition(source, destination, kind, 0.5f, b);
            }
            check(same(pipeline.frame(), b), "Transition endpoints share current NPC snapshot");
        }
        state.transition.elapsed = Transition::duration;
        pipeline.update(renderer, map, state, &npcs, &animator.snapshot());
        renderer.compose(map, to, b, &npcs, &animator.snapshot());
        check(same(pipeline.frame(), b), "Completion does not restore stale animation frame");
    }
    state.player = cameras[0];
    state.transition.elapsed = Transition::duration;
    ++state.revision;
    animator.setTime(70);
    pipeline.update(renderer, map, state, &npcs, &animator.snapshot());
    check(pipeline.update(renderer, map, state, &npcs), "Removing snapshot invalidates held frame");
    renderer.compose(map, state.player, b, &npcs);
    check(same(pipeline.frame(), b), "Null snapshot displays first selected clip frame");
    check(pipeline.update(renderer, map, state, &npcs, &animator.snapshot()),
          "Restoring snapshot invalidates held frame");
    auto oversized = appearance;
    oversized.width = 0.5f;
    oversized.height = 1;
    rejects([&] { PreparedNpcs bad(std::vector<NpcAppearance>(8, oversized)); },
            "All animation frames count toward prepared pixel budget");
    auto invalid = animator.snapshot();
    invalid.frames[0].active = 4;
    rejects([&] { renderer.compose(map, cameras[0], a, &npcs, &invalid); },
            "Invalid frame selection rejected");
    invalid = animator.snapshot();
    invalid.count = 0;
    rejects([&] { renderer.compose(map, cameras[0], a, &npcs, &invalid); },
            "Snapshot count mismatch rejected");
    auto malformed = appearance;
    malformed.animations[1].views[2].clear();
    rejects([&] { PreparedNpcs bad({malformed}); }, "Walk requires all directions");
    malformed = appearance;
    malformed.animations[0].views[0][1].anchorY = 0.5f;
    rejects([&] { PreparedNpcs bad({malformed}); }, "Every frame respects ground anchor");
}
} // namespace
int main() {
    try {
        timing();
        staggeredIdle();
        triggeredAttacks();
        rendering();
        std::cout << "NPC animation checks passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

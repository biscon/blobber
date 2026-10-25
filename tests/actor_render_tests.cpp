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
ActorWorld world() {
    return testActors(
        map, {3, 5}, {"test"},
        {{{0}, {3, 3}, {0, 0}, Heading::South, NpcAnimation::Attack}, {{0}, {3, 3}, {1, 1}, Heading::South}});
}
void checkPresentation(const SceneRenderer& renderer, const NpcTimings& timing) {
    auto actors = world();
    NpcAnimator animator(actors, {timing});
    FramePipeline pipeline;
    Frame before, expected;
    MovementState state;
    state.player = {{3, 5}, Heading::North};
    auto draw = [&](std::uint64_t time) {
        animator.setTime(time);
        return pipeline.update(renderer, map, state, &actors, &animator.snapshot());
    };
    check(draw(70) && !draw(70), "Initial compose then stationary reuse");
    before.copyFrom(pipeline.frame());
    const auto id = actors.id(0);
    check(actors.move(id, {2, 3}, {0, 0}, state.player.cell).changed, "Controlled actor move");
    rejects([&] { renderer.compose(map, state.player, expected, &actors, &animator.snapshot()); },
            "Stale animation state rejected");
    check(draw(70) && !same(before, pipeline.frame()) && !draw(70), "Move refreshes stationary view once");
    before.copyFrom(pipeline.frame());
    check(actors.face(id, Heading::North).changed && draw(70) && !same(before, pipeline.frame()),
          "Facing refreshes stationary view");
    actors.face(id, Heading::South);
    draw(70);
    before.copyFrom(pipeline.frame());
    check(actors.damage(id, 1, 70).damage == 1 && draw(70), "Damage starts immediate flash");
    check(animator.snapshot().frames[0].flash && !same(before, pipeline.frame()), "White flash changes body");
    std::size_t brightened = 0;
    for (std::size_t i = 0; i < static_cast<std::size_t>(viewWidth) * viewHeight; ++i) {
        const auto a = before[i], b = pipeline.frame()[i];
        check(b.r >= a.r && b.g >= a.g && b.b >= a.b,
              "Flash only whitens visible body, preserving shadow and holes");
        brightened += b.r > a.r || b.g > a.g || b.b > a.b;
    }
    check(brightened != 0, "Fixture exercises visible flash pixels");
    draw(169);
    check(animator.snapshot().frames[0].flash, "Flash still active at 99 ms");
    draw(170);
    check(!animator.snapshot().frames[0].flash && animator.snapshot().frames[0].visible,
          "Survivor returns to normal at 100 ms");
    actors.damage(id, 1, 200);
    draw(200);
    actors.damage(id, 1, 250);
    draw(349);
    check(animator.snapshot().frames[0].flash, "Repeated damage restarts flash");
    draw(350);
    check(!animator.snapshot().frames[0].flash, "Repeated flash expires exactly");
    actors.damage(id, 999, 400);
    draw(400);
    before.copyFrom(pipeline.frame());
    const auto frozen = animator.snapshot().frames[0];
    check(!actors.occupied({2, 3}) && frozen.flash && frozen.visible, "Lethal flash never blocks");
    draw(499);
    check(animator.snapshot().frames[0].active == frozen.active &&
              animator.snapshot().frames[0].idle == frozen.idle,
          "Lethal pose stays frozen while other actors advance");
    check(draw(500) && !animator.snapshot().frames[0].visible,
          "Expiry invalidates stationary view and hides body/shadow");
    renderer.compose(map, state.player, expected, &actors, &animator.snapshot());
    check(same(expected, pipeline.frame()) && !draw(500), "No stale corpse on subsequent idle frame");
    draw(std::numeric_limits<std::uint64_t>::max());
    check(!animator.snapshot().frames[0].visible, "Large time jump does not replay terminal flash");
    const auto second = actors.id(1);
    actors.damage(second, 1, 600);
    draw(600);
    actors.remove(second);
    draw(600);
    check(!animator.snapshot().frames[1].visible && !actors.occupied({3, 3}),
          "Explicit removal cancels flash");
    auto none = testActors(map, {3, 5}, {"test"}, {});
    renderer.compose(map, state.player, expected, &none);
    check(same(expected, pipeline.frame()), "No bodies or shadows remain after terminal cleanup");
    auto other = world();
    NpcAnimator foreign(other, {timing});
    rejects([&] { renderer.compose(map, state.player, expected, &actors, &foreign.snapshot()); },
            "Foreign animation rejected");
    auto mismatched = animator.snapshot();
    mismatched.world = other.snapshot().world;
    rejects([&] { pipeline.update(renderer, map, state, &actors, &mismatched); },
            "Cached frame path still rejects foreign snapshots with equal revisions");
    check(pipeline.update(renderer, map, state, &other), "Changing world invalidates cached image");
    check(pipeline.update(renderer, map, state), "Removing world invalidates cached image");
}
void transitions(const SceneRenderer& renderer, const NpcTimings& timing) {
    Frame expected, source, destination;
    for (int action = 0; action < 6; ++action) {
        auto actors = world();
        NpcAnimator animator(actors, {timing});
        FramePipeline pipeline;
        const auto kind = static_cast<Action>(action);
        Player from{{3, 4}, Heading::North}, to = from;
        check(tryAction(map, to, kind), "Transition fixture path");
        MovementState state;
        state.player = to;
        state.transition = {from, to, kind, Transition::duration * 0.5f};
        auto verify = [&](std::uint64_t time) {
            animator.setTime(time);
            pipeline.update(renderer, map, state, &actors, &animator.snapshot());
            if (isStrafe(kind))
                renderer.composeStrafe(map, from, to, 0.5f, expected, &actors, &animator.snapshot());
            else {
                renderer.compose(map, from, source, &actors, &animator.snapshot());
                renderer.compose(map, to, destination, &actors, &animator.snapshot());
                composeTransition(source, destination, kind, 0.5f, expected);
            }
            check(same(expected, pipeline.frame()),
                  "Transition uses same current actor state for both endpoints");
        };
        verify(70);
        actors.move(actors.id(0), {2, 3}, {1, 0}, to.cell);
        verify(70);
        actors.face(actors.id(0), Heading::West);
        verify(70);
        actors.damage(actors.id(0), 7, 70);
        verify(70);
        verify(169);
        verify(170);
        actors.remove(actors.id(1));
        verify(170);
        state.transition.elapsed = Transition::duration;
        pipeline.update(renderer, map, state, &actors, &animator.snapshot());
        renderer.compose(map, to, expected, &actors, &animator.snapshot());
        check(same(expected, pipeline.frame()), "Transition completion never restores removed actor");
    }
}
void occludedFlash(const SceneRenderer& renderer, const NpcTimings& timing) {
    const Map walls({"#######", "#.....#", "#.....#", "#######", "#.....#", "#.....#", "#######"});
    auto hidden = testActors(walls, {3, 5}, {"test"}, {{{0}, {3, 2}, {0, 0}, Heading::South}});
    NpcAnimator animator(hidden, {timing});
    Frame before, after;
    const Player camera{{3, 5}, Heading::North};
    renderer.compose(walls, camera, before, &hidden, &animator.snapshot());
    hidden.damage(hidden.id(0), 7, 100);
    animator.setTime(100);
    renderer.compose(walls, camera, after, &hidden, &animator.snapshot());
    check(same(before, after), "White flash cannot shine through a wall");
    animator.setTime(10000);
    check(!animator.snapshot().frames[0].visible, "Offscreen death expires without being presented");
}
} // namespace
int main() {
    try {
        const auto appearance = testAnimatedNpcAppearance();
        const auto timing = npcTimings(appearance);
        const SceneRenderer renderer(testMaterials(), {appearance});
        checkPresentation(renderer, timing);
        transitions(renderer, timing);
        occludedFlash(renderer, timing);
        std::cout << "Actor presentation checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

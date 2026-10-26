#include "render/frame_pipeline.hpp"
#include "render/transitions.hpp"
#include "support/actor_fixtures.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "support/npc_fixtures.hpp"
#include "support/render_fixtures.hpp"
#include <iostream>
#include <limits>
using namespace blobber;
namespace {
const Map map = combatMap();
ActorWorld world() {
    return testActors(map, {3, 5}, {"test"}, {{{0}, {3, 3}, {0, 0}, Heading::South}});
}
void timing() {
    auto actors = world();
    const auto metadata = npcTimings(testReactionAppearance());
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
void render() {
    auto appearance = testReactionAppearance();
    // Use opaque living ink here so replacement comparisons exclude legitimate
    // translucent blending with the corpse behind it; transparent holes remain.
    for (auto& direction : appearance.animations[0].views)
        for (auto& image : direction)
            for (auto& pixel : image.pixels)
                if (pixel.a)
                    pixel.a = 255;
    rejects([&] { PreparedNpcs oversized({appearance}, {1024, 512u * 1024u * 1024u}); },
            "Reaction preparation respects an explicit small resident budget");
    auto invalid = appearance;
    invalid.animations[4].views[2].clear();
    rejects([&] { PreparedNpcs incomplete({invalid}); }, "Death cannot fall back to living direction");
    const SceneRenderer renderer(testMaterials(), {appearance});
    auto actors = world();
    auto empty = testActors(map, {3, 5}, {"test"}, {});
    NpcAnimator animator(actors, {npcTimings(appearance)});
    FramePipeline pipeline;
    Frame a, b, source, destination;
    const std::array<Player, 4> cameras{{{{3, 5}, Heading::North},
                                         {{3, 1}, Heading::South},
                                         {{5, 3}, Heading::West},
                                         {{1, 3}, Heading::East}}};
    actors.damage(actors.id(0), 7, 1000);
    for (const auto camera : cameras) {
        MovementState state;
        state.player = camera;
        ++state.revision;
        animator.setTime(1600);
        pipeline.update(renderer, map, state, &actors, &animator.snapshot());
        renderer.compose(map, camera, b, &empty);
        check(!same(pipeline.frame(), b), "All four directions show a corpse");
        check(pipeline.picking()->targets().count == 0, "Corpse never becomes a target");
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x)
                check(!pipeline.picking()->pick(x, y), "Corpse pixels cannot be picked");
        animator.setTime(2200);
        check(pipeline.update(renderer, map, state, &actors, &animator.snapshot()) &&
                  same(pipeline.frame(), b),
              "Expiry removes body and shadow in every direction");
    }
    for (int action = 0; action < 6; ++action) {
        const auto kind = static_cast<Action>(action);
        Player from{{3, 4}, Heading::North}, to = from;
        check(tryAction(map, to, kind), "Fixture transition legal");
        MovementState state;
        state.player = to;
        state.revision = action + 10;
        state.transition = {from, to, kind, Transition::duration * .5f};
        for (const auto time : {1000u, 1200u, 1400u, 1600u, 2200u}) {
            animator.setTime(time);
            pipeline.update(renderer, map, state, &actors, &animator.snapshot());
            if (isStrafe(kind))
                renderer.composeStrafe(map, from, to, .5f, b, &actors, &animator.snapshot());
            else {
                renderer.compose(map, from, source, &actors, &animator.snapshot());
                renderer.compose(map, to, destination, &actors, &animator.snapshot());
                composeTransition(source, destination, kind, .5f, b);
            }
            check(same(pipeline.frame(), b), "Transition endpoints share death stage, including expiry");
        }
    }
    const Map wall({"#######", "#.....#", "#.....#", "#######", "#.....#", "#.....#", "#######"});
    auto hidden = testActors(wall, {3, 5}, {"test"}, {{{0}, {3, 2}, {0, 0}, Heading::South}});
    NpcAnimator hiddenAnimator(hidden, {npcTimings(appearance)});
    renderer.compose(wall, cameras[0], a, &hidden, &hiddenAnimator.snapshot());
    hidden.damage(hidden.id(0), 7, 0);
    for (const auto time : {0u, 200u, 400u, 600u, 1199u, 1200u}) {
        hiddenAnimator.setTime(time);
        renderer.compose(wall, cameras[0], b, &hidden, &hiddenAnimator.snapshot());
        check(same(a, b), "Every hidden death stage remains behind wall");
    }
    check(!hiddenAnimator.snapshot().frames[0].visible,
          "Hidden actor expires without first becoming visible");
    // A higher-index corpse must not paint over a lower-index replacement.
    auto overlap = testActors(map, {3, 5}, {"test"},
                              {{{0}, {2, 3}, {0, 0}, Heading::South}, {{0}, {3, 3}, {0, 0}, Heading::South}});
    overlap.damage(overlap.id(1), 7, 0);
    overlap.move(overlap.id(0), {3, 3}, {0, 0}, {3, 5});
    NpcAnimator overlapAnimator(overlap, {npcTimings(appearance)});
    overlapAnimator.setTime(200);
    MovementState state;
    state.player = cameras[0];
    FramePipeline overlapFrames;
    overlapFrames.update(renderer, map, state, &overlap, &overlapAnimator.snapshot());
    const auto targets = overlapFrames.picking()->targets();
    check(targets.count == 1 && targets.ids[0].slot == 0,
          "Living subslot replacement remains visible and pickable");
    auto livingOnly = testActors(map, {3, 5}, {"test"}, {{{0}, {3, 3}, {0, 0}, Heading::South}});
    NpcAnimator livingAnimator(livingOnly, {npcTimings(appearance)});
    livingAnimator.setTime(200);
    PickingBuffer livingPicking;
    renderer.compose(map, cameras[0], a, &livingOnly, &livingAnimator.snapshot(), &livingPicking);
    int checkedPixels = 0;
    for (int y = 1; y + 1 < viewHeight; ++y)
        for (int x = 1; x + 1 < viewWidth; ++x)
            if (livingPicking.slot(x, y) == 0 && livingPicking.slot(x - 1, y) == 0 &&
                livingPicking.slot(x + 1, y) == 0) {
                const auto expected = a[y * viewWidth + x];
                const auto actual = overlapFrames.frame()[y * viewWidth + x];
                check(
                    expected.r == actual.r && expected.g == actual.g && expected.b == actual.b,
                    "Opaque living replacement pixels draw over same-depth corpse regardless of slot order");
                ++checkedPixels;
            }
    check(checkedPixels > 0, "Replacement pixel comparison exercises visible overlap");
    check(overlap.occupied({3, 3}), "Corpse presentation cannot release replacement occupancy");
    overlap.remove(overlap.id(1));
    check(overlap.occupied({3, 3}), "Removing corpse preserves replacement occupancy");
}
void canvasGeometry() {
    auto appearance = testAnimatedNpcAppearance();
    auto& death = appearance.animations[4];
    death.width = 0.9f;
    death.timing = {{1000, 0, 0, 0}, 1};
    for (std::size_t side = 0; side < 4; ++side) {
        const auto& original = appearance.animations[0].views[side][0];
        auto padded = original;
        padded.width = 16;
        padded.pixels.assign(16 * padded.height, BLANK);
        for (int y = 0; y < padded.height; ++y)
            for (int x = 0; x < original.width; ++x)
                padded.pixels[y * padded.width + x + 4] = original.pixels[y * original.width + x];
        death.views[side].push_back(std::move(padded));
    }
    PreparedNpcs prepared({appearance});
    auto actors = world();
    NpcAnimator animator(actors, {npcTimings(appearance)});
    std::array<float, viewWidth> walls;
    walls.fill(std::numeric_limits<float>::infinity());
    Frame living(BLACK), padded(BLACK);
    auto bounds = [](const Frame& frame) {
        std::array<int, 4> b{viewWidth, viewHeight, -1, -1};
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x) {
                const auto c = frame[y * viewWidth + x];
                if (c.r || c.g || c.b) {
                    b[0] = std::min(b[0], x);
                    b[1] = std::min(b[1], y);
                    b[2] = std::max(b[2], x);
                    b[3] = std::max(b[3], y);
                }
            }
        return b;
    };
    const Player camera{{3, 5}, Heading::North};
    prepared.compose(actors, camera, 0, walls, living, &animator.snapshot());
    actors.damage(actors.id(0), 7, 0);
    animator.setTime(100);
    prepared.compose(actors, camera, 0, walls, padded, &animator.snapshot());
    const auto a = bounds(living), b = bounds(padded);
    check(a[2] > a[0] && a[3] > a[1], "Geometry fixture has visible body");
    check(std::abs(a[0] - b[0]) <= 1 && std::abs(a[2] - b[2]) <= 1 && a[1] == b[1] && a[3] == b[3],
          "Wider transparent canvas preserves body scale, ground and pivot within one raster rounding pixel");
}
void clocks() {
    const auto catalog = combatCatalog();
    const auto placements = combatPlacements(map, {{3, 3}, Heading::North}, 1);
    GameSession session(map, catalog, placements, 5, {{3, 3}, Heading::North});
    NpcAnimator animator(session.actors(), {npcTimings(testReactionAppearance())});
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
    NpcAnimator fresh(restarted.actors(), {npcTimings(testReactionAppearance())});
    check(fresh.snapshot().world != animator.snapshot().world &&
              fresh.snapshot().frames[0].clip == NpcAnimation::Idle && fresh.snapshot().frames[0].visible,
          "Restart has independent lifetime and no stale reaction");
}
} // namespace
int main() {
    try {
        timing();
        render();
        clocks();
        canvasGeometry();
        std::cout << "NPC reaction checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

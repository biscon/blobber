#include "render/frame_pipeline.hpp"
#include "render/transitions.hpp"
#include "support/actor_fixtures.hpp"
#include "support/check.hpp"
#include "support/npc_fixtures.hpp"
#include "support/render_fixtures.hpp"
#include "ui/spell_layout.hpp"
#include "ui/status_occlusion.hpp"
#include <iostream>
#include <limits>
using namespace blobber;
namespace {
void statusOcclusion() {
    const auto appearance = testAnimatedNpcAppearance();
    const PreparedNpcs prepared({appearance});
    const Map room({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
    const Player camera{{3, 5}, Heading::North};
    // Deliberately reverse slot/depth order: owner 0 is behind actor 1.
    auto actors = testActors(room, camera.cell, {"test"},
                             {{{0}, {3, 2}, {0, 0}, Heading::South}, {{0}, {3, 3}, {0, 0}, Heading::South}});
    NpcAnimator animator(actors, {npcTimings(appearance)});
    PickingBuffer picking;
    Frame frame;
    std::array<float, viewWidth> walls;
    walls.fill(std::numeric_limits<float>::infinity());
    const auto verify = [&](const NpcAnimationSnapshot& animation) {
        picking.clear();
        prepared.compose(actors, camera, 0, walls, frame, &animation, &picking);
        picking.finish(&actors);
        unsigned blocked = 0, holes = 0;
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x) {
                const auto body = picking.slot(x, y);
                check(picking.badgeVisible(0, x, y) == (body != 1),
                      "Only nearer silhouette pixels occlude the rear icon");
                check(picking.badgeVisible(1, x, y),
                      "Owner and farther silhouettes never obscure the front icon");
                blocked += body == 1;
                holes += body == 0;
            }
        check(blocked > 100 && holes > 10, "Fixture exercises foreground silhouette and visible rear pixels");
    };
    animator.setTime(0);
    verify(animator.snapshot());
    auto traveling = animator.snapshot();
    traveling.frames[1].offsetY = .2f;
    verify(traveling);
    actors.damage(actors.id(1), 999, 1);
    animator.setTime(1);
    verify(animator.snapshot());
    check(picking.targets().count == 1 && picking.targets().ids[0] == actors.id(0),
          "Occluding corpse does not become a selectable target, while the rear actor remains selectable");

    // A synthetic 12x12 tile exercises exact clip coverage, holes, walls and edges.
    picking.clear();
    picking.badgeAnchor(0, 0, 0, 3.f);
    picking.badgeAnchor(1, 0, 0, 1.f);
    picking.badgeAnchor(2, 0, 0, 5.f);
    walls[104] = 2.f;
    picking.wallDepths(walls);
    for (int y = 100; y < 112; ++y)
        for (int x = 100; x < 112; ++x)
            if ((x + y) % 3)
                picking.write(y * viewWidth + x, x < 106 ? 1 : 2);
    std::array<unsigned, 144> painted{};
    forEachStatusClip(picking, 0, {viewX + 100, viewY + 100, 12, 12}, [&](UiRect clip) {
        for (int y = clip.y; y < clip.y + clip.height; ++y)
            for (int x = clip.x; x < clip.x + clip.width; ++x)
                ++painted[(y - viewY - 100) * 12 + x - viewX - 100];
    });
    for (int y = 0; y < 12; ++y)
        for (int x = 0; x < 12; ++x) {
            const bool visible = x != 4 && (x >= 6 || (x + y + 200) % 3 == 0);
            check(painted[y * 12 + x] == unsigned(visible),
                  "Scissor rectangles cover each visible pixel exactly once and no hidden pixels");
        }
    check(!picking.badgeVisible(0, -1, 100) && !picking.badgeVisible(0, 100, viewHeight) &&
              !picking.badgeVisible(UINT16_MAX, 100, 100),
          "Offscreen and invalid owners are rejected");
    picking.clear();
    check(!picking.badgeVisible(0, 100, 100), "Reset discards stale depth metadata");
    picking.badgeAnchor(0, 0, 0, 3.f);
    unsigned draws = 0;
    forEachStatusClip(picking, 0, {viewX + 100, viewY + 100, 12, 12}, [&](UiRect clip) {
        ++draws;
        check(clip.width == 12 && clip.height == 12, "Unobstructed tile uses one full rectangle");
    });
    check(draws == 1, "Unobstructed status needs only one draw");
}
void stableSpellBadges() {
    auto appearance = testAnimatedNpcAppearance();
    // The second idle silhouette is narrower and shorter: visible bounds genuinely move.
    for (auto& view : appearance.animations[0].views)
        for (int y = 0; y < view[1].height; ++y)
            for (int x = 0; x < view[1].width; ++x)
                if (y < 3 || x < 2)
                    view[1].pixels[y * view[1].width + x].a = 0;
    const PreparedNpcs prepared({appearance});
    const Map room({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
    const Player camera{{3, 5}, Heading::North};
    auto actors = testActors(room, camera.cell, {"test"},
                             {{{0}, {3, 2}, {0, 0}, Heading::South}, {{0}, {3, 2}, {1, 0}, Heading::South}});
    NpcAnimator animator(actors, {npcTimings(appearance)});
    PickingBuffer picking;
    Frame frame;
    std::array<float, viewWidth> walls;
    walls.fill(std::numeric_limits<float>::infinity());
    HudSnapshot hud;
    hud.magic.missiles = true;
    hud.magic.affectedCount = 2;
    hud.magic.affected[0] = actors.id(0);
    hud.magic.affected[1] = actors.id(1);
    const auto draw = [&](std::uint64_t time) {
        animator.setTime(time);
        picking.clear();
        prepared.compose(actors, camera, 0, walls, frame, &animator.snapshot(), &picking);
        picking.finish(&actors);
        hud.targets = picking.targets();
        return spellMarkerLayout(hud);
    };
    const auto first = draw(0);
    const auto body = hud.targets.anchors[0];
    const auto second = draw(60);
    check(body.valid && hud.targets.anchors[0].top != body.top,
          "Idle fixture changes visible silhouette bounds");
    for (unsigned i = 0; i < 2; ++i)
        check(first[i] && second[i] && first[i]->x == second[i]->x && first[i]->y == second[i]->y,
              "Badge positions and collision layout remain stable across idle poses");
    check(actors.move(actors.id(0), {2, 2}, {0, 0}, camera.cell).changed, "Badge fixture moves actor");
    const auto moved = draw(120);
    check(moved[0] && moved[0]->x != first[0]->x, "Badge follows actual actor movement");
    walls.fill(0);
    const auto hidden = draw(180);
    check(!hidden[0] && !hidden[1], "Projected badge anchors never reveal fully occluded bodies");
}
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
void spellFloorPreview(const SceneRenderer& renderer) {
    for (int heading = 0; heading < 4; ++heading) {
        MovementState movement;
        movement.player = {{3, 3}, static_cast<Heading>(heading)};
        const auto cell = movement.player.cell + forward(movement.player.heading);
        auto actors = testActors(map, movement.player.cell, {"test"}, {{{0}, cell, {0, 0}, Heading::South}});
        auto area = spellArea(actors, movement.player);
        area.kind = rules::SpellKind::BurningHands;
        Frame before, after, bare, tinted;
        renderer.compose(map, movement.player, bare);
        renderer.compose(map, movement.player, tinted, nullptr, nullptr, nullptr, nullptr, &area);
        PickingBuffer beforePick, afterPick;
        renderer.compose(map, movement.player, before, &actors, nullptr, &beforePick);
        renderer.compose(map, movement.player, after, &actors, nullptr, &afterPick, nullptr, &area);
        std::size_t changed = 0, bodies = 0, opaqueBodies = 0;
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x) {
                const auto i = y * viewWidth + x;
                const auto a = before[i], b = after[i];
                check(beforePick.slot(x, y) == afterPick.slot(x, y),
                      "Floor preview preserves picking exactly");
                const bool equal = a.r == b.r && a.g == b.g && a.b == b.b;
                if (beforePick.slot(x, y) != PickingBuffer::empty) {
                    ++bodies;
                    opaqueBodies += equal;
                }
                const auto ground = bare[i], preview = tinted[i];
                if (ground.r != preview.r || ground.g != preview.g || ground.b != preview.b) {
                    ++changed;
                    check(ground.g > 0 && ground.r == 0 && ground.b == 0,
                          "Preview changes only generated green floor, never wall or ceiling");
                }
            }
        check(changed > 100 && bodies > 0 && opaqueBodies > bodies / 2,
              "Fixture exercises floor tint and body occlusion in every heading");
        FramePipeline pipeline;
        pipeline.update(renderer, map, movement, &actors);
        check(pipeline.update(renderer, map, movement, &actors, nullptr, nullptr, 0, &area),
              "Preview invalidates stationary cache");
        check(!pipeline.update(renderer, map, movement, &actors, nullptr, nullptr, 0, &area),
              "Unchanged preview reuses cache");
        check(pipeline.update(renderer, map, movement, &actors) && same(pipeline.frame(), before),
              "Cancel removes all cached floor tint");
    }
    const Map walls({"#######", "#.....#", "#.....#", "#.##..#", "#.....#", "#.....#", "#######"});
    const Player pose{{3, 4}, Heading::North};
    auto actors = testActors(walls, pose.cell, {"test"}, {});
    const auto blocked = spellArea(actors, pose);
    check(!blocked.eligible[0] && !blocked.eligible[1] && !blocked.eligible[2],
          "Conservative wall/corner rules also suppress diagonal preview cells");
    Frame a, b;
    renderer.compose(walls, pose, a, &actors);
    renderer.compose(walls, pose, b, &actors, nullptr, nullptr, nullptr, &blocked);
    check(same(a, b), "Blocked footprint produces no tint");
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
        spellFloorPreview(renderer);
        stableSpellBadges();
        statusOcclusion();
        std::cout << "Actor presentation checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

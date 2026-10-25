#include "render/frame_pipeline.hpp"
#include "support/actor_fixtures.hpp"
#include "support/check.hpp"
#include "support/npc_fixtures.hpp"
#include "support/render_fixtures.hpp"
#include <iostream>
#include <limits>
using namespace blobber;
int main() {
    try {
        const Map map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
        auto appearance = testNpcAppearance();
        appearance.width = 0.5f;
        PreparedNpcs prepared({appearance});
        const NpcInstance near{{0}, {3, 2}, {0, 1}, Heading::South};
        const NpcInstance far{{0}, {3, 1}, {0, 1}, Heading::South};
        auto nearOnly = testActors(map, {3, 3}, {"test"}, {near});
        auto farOnly = testActors(map, {3, 3}, {"test"}, {far});
        auto both = testActors(map, {3, 3}, {"test"}, {near, far});
        Frame frame, untouched;
        PickingBuffer nearPick, farPick, picking;
        std::array<float, viewWidth> walls;
        walls.fill(std::numeric_limits<float>::infinity());
        const Player camera{{3, 3}, Heading::North};
        prepared.compose(nearOnly, camera, 0, walls, frame, nullptr, &nearPick);
        nearPick.finish(&nearOnly);
        prepared.compose(farOnly, camera, 0, walls, frame, nullptr, &farPick);
        farPick.finish(&farOnly);
        prepared.compose(both, camera, 0, walls, frame, nullptr, &picking);
        picking.finish(&both);
        int foreground = 0, holes = 0, empty = 0;
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x) {
                if (nearPick.slot(x, y) != PickingBuffer::empty) {
                    check(picking.slot(x, y) == 0, "Nearest nontransparent body wins overlap");
                    ++foreground;
                } else if (farPick.slot(x, y) != PickingBuffer::empty) {
                    check(picking.slot(x, y) == 1, "Transparent foreground exposes background picking");
                    ++holes;
                } else {
                    check(!picking.pick(x, y), "Empty pixels and shadows never select");
                    ++empty;
                }
            }
        check(foreground > 0 && holes > 0 && empty > 0, "Representative overlap/holes/background exercised");
        check(picking.targets().count == 2, "Visible list includes both bodies");
        for (std::uint16_t slot = 0; slot < 2; ++slot) {
            int left = viewWidth, top = viewHeight, right = -1, bottom = -1;
            for (int y = 0; y < viewHeight; ++y)
                for (int x = 0; x < viewWidth; ++x)
                    if (picking.slot(x, y) == slot) {
                        left = std::min(left, x);
                        right = std::max(right, x);
                        top = std::min(top, y);
                        bottom = std::max(bottom, y);
                    }
            const auto anchor = picking.targets().anchors[slot];
            check(anchor.valid && anchor.x == (left + right) / 2 && anchor.y == (top + bottom) / 2,
                  "Feedback anchor follows visible body extent including overlap and holes");
        }
        check(!picking.pick(-1, 0) && !picking.pick(viewWidth, 0), "Out-of-bounds picking rejected");
        walls.fill(0);
        picking.clear();
        prepared.compose(both, camera, 0, walls, frame, nullptr, &picking);
        picking.finish(&both);
        check(picking.targets().count == 0, "Wall-clipped bodies excluded");
        check(!picking.targets().anchors[0].valid && !picking.targets().anchors[1].valid,
              "Hidden bodies cannot retain stale feedback anchors");
        walls.fill(std::numeric_limits<float>::infinity());
        for (int x = 0; x < viewWidth / 2; ++x)
            walls[x] = 0;
        picking.clear();
        prepared.compose(both, camera, 0, walls, frame, nullptr, &picking);
        picking.finish(&both);
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth / 2; ++x)
                check(!picking.pick(x, y), "Partially occluded columns excluded");
        NpcAnimator animator(both, {npcTimings(appearance)});
        animator.setTime(700);
        check(bool(both.damage(both.id(0), 7, 700)), "Lethal fixture");
        animator.setTime(700);
        walls.fill(std::numeric_limits<float>::infinity());
        picking.clear();
        prepared.compose(both, camera, 0, walls, frame, &animator.snapshot(), &picking);
        picking.finish(&both);
        check(picking.targets().count == 1 && picking.targets().ids[0].slot == 1,
              "Dead flash excluded from target list");
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x)
                if (nearPick.slot(x, y) != PickingBuffer::empty)
                    check(!picking.pick(x, y), "Dead flash occludes living target behind it");
        animator.setTime(800);
        picking.clear();
        prepared.compose(both, camera, 0, walls, frame, &animator.snapshot(), &picking);
        picking.finish(&both);
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x)
                if (farPick.slot(x, y) != PickingBuffer::empty)
                    check(picking.pick(x, y)->slot == 1, "Flash expiry reveals rear body");

        const auto animated = testAnimatedNpcAppearance();
        const SceneRenderer renderer(testMaterials(), {animated});
        auto actors = testActors(map, {3, 3}, {"test"}, {near, far});
        NpcAnimator animation(actors, {npcTimings(animated)});
        FramePipeline pipeline;
        MovementState state;
        state.player = camera;
        animation.setTime(0);
        check(pipeline.update(renderer, map, state, &actors, &animation.snapshot()), "Initial compose");
        check(pipeline.picking() && pipeline.picking()->targets().count > 0, "Stable frame exposes picking");
        const auto* data = pipeline.picking();
        check(!pipeline.update(renderer, map, state, &actors, &animation.snapshot()) &&
                  pipeline.picking() == data,
              "Idle frame reuses picking");
        Frame plain;
        renderer.compose(map, camera, plain, &actors, &animation.snapshot());
        for (std::size_t i = 0; i < plain.size(); ++i)
            check(plain[i].r == pipeline.frame()[i].r && plain[i].g == pipeline.frame()[i].g &&
                      plain[i].b == pipeline.frame()[i].b,
                  "Picking leaves image pixels unchanged");
        for (const auto action : {Action::Forward, Action::Back, Action::Left, Action::Right,
                                  Action::TurnLeft, Action::TurnRight}) {
            Player to = camera;
            tryAction(map, to, action);
            state.player = to;
            state.transition = {camera, to, action, 0.08f};
            ++state.revision;
            pipeline.update(renderer, map, state, &actors, &animation.snapshot());
            check(!pipeline.picking(), "All transitions suspend picking");
            state.transition.elapsed = Transition::duration;
            pipeline.update(renderer, map, state, &actors, &animation.snapshot());
            check(pipeline.picking(), "Transition completion restores destination picking");
        }
        state.player = camera;
        ++state.revision;
        animation.setTime(60);
        check(pipeline.update(renderer, map, state, &actors, &animation.snapshot()),
              "Animation refreshes composition");
        check(bool(actors.remove(actors.id(0))), "Remove fixture");
        animation.setTime(60);
        pipeline.update(renderer, map, state, &actors, &animation.snapshot());
        for (std::size_t i = 0; i < pipeline.picking()->targets().count; ++i)
            check(pipeline.picking()->targets().ids[i].slot != 0,
                  "Removal invalidates picking in stationary view");
        std::cout << "Picking checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

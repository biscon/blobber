#include "assets/image.hpp"
#include "assets/prepared_loot.hpp"
#include "generation/floor.hpp"
#include "render/frame_pipeline.hpp"
#include "support/check.hpp"
#include "support/fixture_world.hpp"
#include "support/npc_fixtures.hpp"
#include "support/render_fixtures.hpp"
#include <iostream>
using namespace blobber;
namespace {
bool picked(const PickingBuffer* p, unsigned fixture) {
    if (!p)
        return false;
    for (int y = 0; y < viewHeight; ++y)
        for (int x = 0; x < viewWidth; ++x)
            if (auto id = p->pickFixture(x, y))
                if (id->slot == fixture)
                    return true;
    return false;
}
void run() {
    auto materials = testMaterials();
    materials.doors[0] = blobber::Material(8, 8, std::vector<Color>(64, Color{220, 180, 30, 255}));
    materials.doors[1] = blobber::Material(8, 8, std::vector<Color>(64, Color{180, 220, 30, 255}));
    OwnedImage icon(GenImageColor(32, 32, BLANK));
    ImageDrawCircle(&icon.value, 16, 16, 10, {240, 240, 240, 255});
    auto loot = prepareLoot(icon.value);
    auto appearance = testNpcAppearance();
    std::vector<NpcTimings> timings{npcTimings(appearance)};
    SceneRenderer renderer(prepareScene(materials), PreparedNpcs({appearance}), {}, std::move(loot));
    auto cat = fixtureCatalog();
    for (unsigned turn = 0; turn < 4; ++turn) {
        const auto rotate = [&](Cell c) { return generation::rotate(c, 9, turn); };
        std::vector<std::string> rows(9, std::string(9, '#'));
        for (int y = 1; y < 8; ++y)
            for (int x = 1; x < 8; ++x) {
                const auto c = rotate({x, y});
                rows[c.y][c.x] = (y == 4 && x != 4) ? '#' : '.';
            }
        const Map map(rows);
        const Player spawn{rotate({4, 6}), Heading(turn)};
        FloorFixtures fixtures;
        fixtures.discovery = true;
        fixtures.count = 3;
        fixtures.entries[0] = {rotate({4, 4}), Heading(turn), FixtureKind::Door, 2};
        fixtures.entries[1] = {spawn.cell, Heading(turn), FixtureKind::Key, 2};
        fixtures.entries[2] = {rotate({4, 2}), Heading(turn), FixtureKind::Downstairs, 0};
        NpcWorld placements(map, spawn.cell, {"goblin"},
                            {{{0}, rotate({4, 2}), {0, 0}, Heading((turn + 2) % 4)}});
        auto owner = std::make_unique<GameSession>(map, cat, placements, 5, spawn, false, timings,
                                                   RecoveryLevel{}, fixtures);
        auto& g = *owner;
        NpcAnimator animator(g.actors(), timings);
        animator.setTime(0);
        FramePipeline frames;
        const auto update = [&] {
            return frames.update(renderer, g.geometry(), g.movement(), &g.actors(), &animator.snapshot(),
                                 nullptr, 0, nullptr, &g.loot().snapshot());
        };
        check(update() && picked(frames.picking(), 0) && picked(frames.picking(), 1),
              "Prepared closed door and at-feet key are pickable in all headings");
        check(frames.picking()->targets().count == 0 && !picked(frames.picking(), 2),
              "Door occludes actor bodies and remote fixture picking");
        check(g.interact(fixtureRequest(g, 1)).changed && update() && !picked(frames.picking(), 1),
              "Stationary key collection invalidates frame and picking");
        check(!update(), "Unchanged fixture frame remains cached");
        g.setAnimated(false);
        g.move(Action::Forward);
        update();
        Frame closed;
        closed.copyFrom(frames.frame());
        check(g.interact(fixtureRequest(g, 0)).changed && update() && !same(closed, frames.frame()),
              "Stationary door opening refreshes prepared frame");
        check(frames.picking()->targets().count == 1 && picked(frames.picking(), 2),
              "Opening reveals both actor and farther stair marker");
        check(!update(), "Open door frame caches normally");
        // Strafe endpoints/reversal use exactly the same opened geometry.
        const Player from = spawn, to{spawn.cell + right(spawn.heading), spawn.heading};
        Frame a, b, c;
        renderer.compose(g.geometry(), from, a, &g.actors());
        renderer.composeStrafe(g.geometry(), from, to, 0, b, &g.actors());
        check(same(a, b), "Fixture strafe starts at exact endpoint");
        renderer.composeStrafe(g.geometry(), from, to, .375f, a, &g.actors());
        renderer.composeStrafe(g.geometry(), to, from, .625f, b, &g.actors());
        check(same(a, b), "Fixture strafe reverses consistently");
        renderer.compose(g.geometry(), to, c, &g.actors());
        renderer.composeStrafe(g.geometry(), from, to, 1, b, &g.actors());
        check(same(c, b), "Fixture strafe finishes at exact endpoint");
    }
}
} // namespace
int main() {
    try {
        run();
        std::cout << "Fixture render contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

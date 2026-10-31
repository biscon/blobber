#include "automation/world_visual.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include "support/world_visibility_fixtures.hpp"
#include "ui/status_occlusion.hpp"
#include <iostream>
using namespace blobber;
int main() {
    try {
        test::VisibilityFixture fixture;
        world_assets::CpuAssets assets(fixture.publication());
        const auto catalog = creationCatalog(true);
        const auto map = test::worldRoom(9, 9);
        MovementState movement;
        movement.player = {{4, 6}, Heading::North};
        const NpcWorld placements(
            map, movement.player.cell, {"goblin"},
            {{{0}, {4, 4}, {0, 0}, Heading::South}, {{0}, {4, 3}, {0, 0}, Heading::South}});
        ActorWorld actors(map, catalog, placements);
        NpcAnimationSnapshot animation;
        animation.count = 2;
        animation.world = actors.snapshot().world;
        animation.actorRevision = actors.snapshot().revision;
        animation.frames[1].active = 1;
        auto scene = world_render::prepare(map);
        world_render::VisibilityPipeline pipeline(scene, assets);
        world_render::FrameInput input{map, movement, &actors, &animation};
        check(pipeline.update(input), "First visibility prepares");
        check(pipeline.picking()->targets().count == 2, "Transparent hole reveals rear actor");
        check(!pipeline.update(input), "Unchanged snapshot caches visibility");
        unsigned frontPixels = 0, rearPixels = 0;
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x) {
                const auto id = pipeline.picking()->pick(x, y);
                if (id)
                    (id->slot ? rearPixels : frontPixels)++;
            }
        check(frontPixels > 100 && rearPixels > 10, "Both silhouettes have meaningful coverage");
        const auto frontAnchor = pipeline.picking()->targets().anchors[0];
        check(frontAnchor.valid, "Visible body publishes anchor");
        const auto& bill = pipeline.frame().billboards[pipeline.frame().count - 1];
        auto attached = world_render::attachment(pipeline.frame().camera, bill, bill.release);
        check(attached.valid && attached.position.y > 0, "Attachment follows shared registered transform");
        auto stale = animation;
        stale.actorRevision++;
        rejects([&] { pipeline.update({map, movement, &actors, &stale}); },
                "Stale animation rejects even with cached revision");
        stale = animation;
        stale.frames[0].offsetX = std::numeric_limits<float>::quiet_NaN();
        rejects([&] { pipeline.update({map, movement, &actors, &stale}); }, "Nonfinite travel rejects");
        for (unsigned clip = 0; clip < npcAnimationCount; ++clip)
            for (unsigned frame = 0; frame < assets.publication.appearances[0].clips[clip].timing.count;
                 ++frame) {
                animation.frames[0].clip = NpcAnimation(clip);
                animation.frames[0].active = std::uint8_t(frame);
                animation.revision++;
                pipeline.update(input);
                check(pipeline.picking()->targets().anchors[0].valid,
                      "All clips retain visible registered geometry");
            }
        animation.frames[0].clip = NpcAnimation::Idle;
        animation.frames[0].active = 0;
        const std::array<unsigned, 4> expectedSides{1, 3, 0, 2};
        for (unsigned heading = 0; heading < 4; ++heading) {
            actors.face(actors.id(0), Heading(heading));
            animation.actorRevision = actors.snapshot().revision;
            animation.revision++;
            pipeline.update(input);
            for (unsigned i = 0; i < pipeline.frame().count; ++i)
                if (pipeline.frame().billboards[i].id == 0)
                    check(pipeline.frame().billboards[i].texture ==
                              assets.publication.appearances[0].clips[0].textures[expectedSides[heading]][0],
                          "Facing sectors select the independently expected source direction");
        }
        animation.frames[0].clip = NpcAnimation::Idle;
        animation.revision++;
        check(actors.damage(actors.id(0), 10000, 0).died, "Fixture actor dies");
        animation.actorRevision = actors.snapshot().revision;
        animation.frames[0].clip = NpcAnimation::Death;
        animation.revision++;
        pipeline.update(input);
        for (unsigned i = 0; i < pipeline.picking()->targets().count; ++i)
            check(pipeline.picking()->targets().ids[i].slot != 0, "Corpse is never a target");
        bool corpse = false;
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x)
                corpse |= pipeline.picking()->slot(x, y) == 0;
        check(corpse, "Corpse remains an occluder");
        movement.transition = {movement.player, {{5, 6}, Heading::North}, Action::Right, 0, .16f};
        pipeline.update(input);
        check(!pipeline.picking(), "Transition suppresses interaction");
        movement.transition.elapsed = .16f;
        pipeline.update(input);
        bool floor = false;
        for (int y = 200; y < viewHeight; ++y)
            floor |= bool(pipeline.groundCell(input, 250, y));
        check(floor && !pipeline.groundCell(input, 250, 10),
              "Ground uses floor intersection and spell validation");
        LootSnapshot loot;
        loot.world = 99;
        loot.stacks[0] = {{4, 6}, {0}, 1, true, 7};
        pipeline.update({map, movement, &actors, &animation, &loot});
        check(pipeline.lootVisible({99, 0, 7}) && !pipeline.lootVisible({99, 0, 6}),
              "Loot visibility checks generation");
        watching = true;
        allocations = 0;
        for (unsigned i = 0; i < 40; ++i) {
            animation.revision++;
            animation.frames[1].active = std::uint8_t(i % 2);
            pipeline.update({map, movement, &actors, &animation, &loot});
        }
        watching = false;
        check(allocations == 0, "Prepared visibility frame processing never allocates C++ storage");
        // Closed fixture proxy occludes; its immutable base map stays prepared.
        FloorFixtures fixtures;
        fixtures.count = 1;
        fixtures.entries[0] = {{4, 5}, Heading::North, FixtureKind::Door};
        FloorGeometry geometry(map, fixtures);
        pipeline.update({geometry.view(), movement, &actors, &animation});
        check(pipeline.picking()->targets().count == 0, "Closed proxy hides actors");
        check(pipeline.picking()->pickFixture(250, 150).has_value(), "Proxy surface picks fixture identity");
        check(pipeline.bytes() < world_render::viewScratchBudget, "Double-buffered visibility fits budget");
        test::WorldFixture incompletePublication;
        world_assets::CpuAssets incompleteAssets(world_assets::load(incompletePublication.directory));
        rejects([&] { world_render::VisibilityPipeline incomplete(scene, incompleteAssets); },
                "Missing future loot/fixture sprites fail preparation before normal frames");
        // Status clipping compares the complete surface at each pixel and excludes its owner.
        PickingBuffer depthPicking;
        depthPicking.badgeAnchor(0, 100, 100, 3.f);
        depthPicking.writeDepth(100 * viewWidth + 100, PickingBuffer::empty, 4.f, true);
        check(depthPicking.badgeVisible(0, 100, 100), "Farther wall leaves status icon visible");
        depthPicking.writeDepth(100 * viewWidth + 100, 1, 2.f);
        check(!depthPicking.badgeVisible(0, 100, 100), "Nearer body clips status pixel");
        depthPicking.writeDepth(100 * viewWidth + 100, 0, 3.f);
        check(depthPicking.badgeVisible(0, 100, 100), "Own silhouette does not clip its icon");
        depthPicking.writeDepth(100 * viewWidth + 100, PickingBuffer::empty, 2.f, true);
        check(!depthPicking.badgeVisible(0, 100, 100), "Nearer opaque pixel clips status icon");
        depthPicking.clear();
        depthPicking.badgeAnchor(0, 100, 100, 3.f);
        for (int y = 100; y < 106; ++y)
            for (int x = 100; x < 110; ++x)
                if (x != 105)
                    depthPicking.writeDepth(std::size_t(y) * viewWidth + x, PickingBuffer::empty, 2.f, true);
        unsigned healthPixels = 0;
        forEachDepthClip(depthPicking, 0, {viewX + 100, viewY + 100, 46, 6},
                         [&](UiRect clip) { healthPixels += clip.width * clip.height; });
        check(healthPixels == 37 * 6, "Health clipping preserves holes and excludes nearer opaque pixels");
        // Independent adjacent-wall distance oracle, including all four headings.
        const Map box({"###", "#.#", "###"});
        auto boxScene = world_render::prepare(box);
        world_render::VisibilityPipeline boxPipeline(boxScene, assets);
        MovementState pose;
        pose.player.cell = {1, 1};
        for (auto heading : {Heading::North, Heading::East, Heading::South, Heading::West}) {
            pose.player.heading = heading;
            boxPipeline.update({box, pose});
            check(std::abs(boxPipeline.frame().picking.depth(100 * viewWidth + 250) - .9f) < 1e-5f,
                  "Rasterized wall matches independent camera/setback distance");
        }
        // The windowless adapter and live command controller use identical snapshots.
        GameSession game(map, catalog, placements, 5, movement.player, false, fixture.timings);
        NpcAnimator gameAnimator(game.actors(), fixture.timings);
        auto liveScene = world_render::prepare(map);
        world_render::VisibilityPipeline live(liveScene, assets);
        automation::WorldVisualContext context{assets, fixture.timings};
        auto visual = automation::makeWorldVisual(&context, map, game);
        CommandController commands(game, fixture.timings);
        for (unsigned step = 0; step < 24; ++step) {
            if (step == 3)
                game.move(Action::TurnLeft);
            game.update(.01, false);
            gameAnimator.setTime(game.timeMs(), &game.effects());
            live.update({game.geometry(), game.movement(), &game.actors(), &gameAnimator.snapshot(),
                         &game.loot().snapshot()});
            const auto& actual = visual->sample(map, game);
            const auto expected = live.picking() ? live.picking()->targets() : TargetSnapshot{};
            check(actual.count == expected.count && actual.world == expected.world &&
                      actual.revision == expected.revision,
                  "Windowless and live visibility tokens agree");
            for (unsigned i = 0; i < actual.count; ++i)
                check(sameActor(actual.ids[i], expected.ids[i]), "Windowless/live target identity");
            commands.refresh(expected, game.movement().transition.active(), false, true);
            for (unsigned i = 0; i < actual.count; ++i) {
                PartyCommand c;
                c.character = {0};
                c.action = PartyAction::Ranged;
                c.target = actual.ids[i];
                c.world = game.actors().snapshot().world;
                c.turn = game.combat().snapshot().turn;
                auto reason = game.movement().transition.active()
                                  ? CommandReason::Moving
                                  : game.combat().validate(c, game.movement().player);
                if (reason == CommandReason::Ready)
                    reason = targetVisibility(c, catalog, actual, game.actors().snapshot());
                check(commands.validate(c) == reason, "Live and windowless command eligibility agrees");
            }
        }
        watching = true;
        allocations = 0;
        for (unsigned i = 0; i < 12; ++i) {
            game.update(.01, false);
            (void)visual->sample(map, game);
        }
        watching = false;
        check(allocations == 0, "World visual observations allocate no C++ storage after preparation");
        const auto otherMap = test::worldRoom(9, 9);
        rejects([&] { live.update({otherMap, game.movement(), &game.actors(), &gameAnimator.snapshot()}); },
                "Same-sized foreign map requires replacement view");
        std::cout << "World visibility, coverage, lifecycle and allocation checks passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}

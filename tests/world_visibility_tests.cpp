#include "automation/world_visual.hpp"
#include "game/perception.hpp"
#include "support/actor_fixtures.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "support/creation_fixtures.hpp"
#include "support/creature_occlusion_checks.hpp"
#include "support/hybrid_fixtures.hpp"
#include "support/moving_visibility_checks.hpp"
#include "support/world_visibility_fixtures.hpp"
#include "ui/status_occlusion.hpp"
#include <cmath>
#include <iostream>
using namespace blobber;
namespace {
void spritesAtDoorLeaves() {
    unsigned frontLeafSamples = 0;
    test::VisibilityFixture fixture;
    auto publication = fixture.publication();
    publication.appearances[0].width = .4f;
    publication.appearances[0].height = .3f;
    for (auto& clip : publication.appearances[0].clips)
        clip.width = .7f;
    world_assets::CpuAssets assets(std::move(publication));
    const auto catalog = hybridCatalog(false);
    const auto map = test::worldRoom(9, 9);
    for (unsigned h = 0; h < 4; ++h)
        for (int slot = 0; slot < 4; ++slot) {
            const auto heading = Heading(h);
            const auto f = forward(heading);
            const Cell doorCell{4, 4}, subcell{slot % 2, slot / 2};
            const Player pose{doorCell + f * -1, heading};
            FloorFixtures fixtures;
            fixtures.count = 1;
            fixtures.entries[0] = {doorCell, heading, FixtureKind::Door};
            NpcWorld places(map, pose.cell, {"goblin"}, {{{0}, doorCell + f, subcell, Heading((h + 2) % 4)}});
            GameSession game(map, catalog, places, 5, pose, false, fixture.timings, {}, fixtures);
            game.setAnimated(false);
            check(bool(game.interact(game.nearbyFixture())), "Door sprite fixture opens normally");
            const auto id = game.actors().id(0);
            check(bool(game.actors().walk(id, doorCell, subcell, pose.cell, 0, 600)),
                  "Door fixture rat walks into its threshold subslot");
            const auto scene = world_render::prepare(game.geometry());
            world_render::VisibilityPipeline pipeline(scene, assets);
            NpcAnimator animator(game.actors(), fixture.timings);
            auto doors = game.doors();
            for (unsigned angle : {0u, 150u, 300u}) {
                doors.now = angle;
                doors.until[0] = 300;
                ++doors.revision;
                for (unsigned time : {0u, 150u, 300u, 450u, 600u}) {
                    animator.setTime(time);
                    world_render::FrameInput input{game.geometry(),      game.movement(), &game.actors(),
                                                   &animator.snapshot(), nullptr,         &doors};
                    input.now = time;
                    watching = true;
                    allocations = 0;
                    pipeline.update(input);
                    watching = false;
                    check(allocations == 0, "Door-clipping walking views allocate no C++ storage");
                    bool found = false;
                    for (unsigned n = 0; n < pipeline.frame().count; ++n) {
                        const auto& b = pipeline.frame().billboards[n];
                        if (b.id != id.slot || !b.alive)
                            continue;
                        found = true;
                        const auto actual = actorPosition(*game.actors().actor(id), time);
                        check(std::abs(b.feet.x - actual.x) < 1e-6f &&
                                  std::abs(b.feet.z - actual.y) < 1e-6f && std::abs(b.width - .7f) < 1e-6f &&
                                  std::abs(b.height - .3f) < 1e-6f &&
                                  std::abs(b.shadowRadius - .4f * .45f) < 1e-6f,
                              "Door traversal preserves the animated anchor, dimensions and shadow");
                        const auto attached = world_render::attachment(pipeline.frame().camera, b, b.release);
                        check(std::abs(attached.position.y - (b.feet.y + .3f * (b.anchorY - b.release.y))) <
                                  1e-6f,
                              "Door clipping does not move registered attachments");
                        const auto& camera = pipeline.frame().camera;
                        const auto foot = world_render::project(camera, b.feet);
                        for (int y = 0; y < viewHeight; ++y)
                            for (int x = 0; x < viewWidth; ++x) {
                                if (pipeline.frame().picking.slot(x, y) != id.slot)
                                    continue;
                                const auto pixel = std::size_t(y) * viewWidth + x;
                                if (b.depth > pipeline.frame().picking.opaqueDepth(pixel) + 1e-5f) {
                                    ++frontLeafSamples;
                                    check(pipeline.frame().picking.badgeVisible(id.slot, x, y),
                                          "An owner-bound badge shares its body's in-front leaf relation");
                                }
                                const float lateral = (x + .5f - foot.x) * b.depth / world_render::focal();
                                const world_render::Vec point{b.feet.x + camera.right.x * lateral,
                                                              b.feet.y + (foot.y - y - .5f) * b.depth /
                                                                             world_render::focal(),
                                                              b.feet.z + camera.right.z * lateral};
                                for (unsigned i = 0; i < pipeline.frame().doorCount; ++i) {
                                    const auto& box = pipeline.frame().doors[i];
                                    if (box.leaf)
                                        continue;
                                    const float dx = point.x - box.origin.x, dz = point.z - box.origin.z;
                                    const float u = (dx * box.x.x + dz * box.x.z) /
                                                    (box.x.x * box.x.x + box.x.z * box.x.z),
                                                v = (dx * box.z.x + dz * box.z.z) /
                                                    (box.z.x * box.z.x + box.z.z * box.z.z);
                                    check(!(u > 1e-4f && u < 1 - 1e-4f && v > 1e-4f && v < 1 - 1e-4f &&
                                            point.y > box.origin.y + 1e-4f &&
                                            point.y < box.origin.y + box.y.y - 1e-4f),
                                          "No pickable body sample remains inside a static jamb or lintel");
                                }
                            }
                    }
                    if (angle == 300 && time == 600)
                        check(found, "Rat retains a visible billboard in the settled open threshold");
                }
            }
        }
    check(frontLeafSamples > 0, "Door fixtures exercise bodies and badges in front of nearer leaf depth");
    // Near the frustum edge, moving away from a jamb must not move the body axis
    // out of view and recreate a locked-facing/unselectable-target situation.
    FloorFixtures fixtures;
    fixtures.count = 1;
    fixtures.entries[0] = {{3, 4}, Heading::North, FixtureKind::Door};
    NpcWorld places(map, {3, 5}, {"goblin"}, {{{0}, {3, 3}, {1, 1}, Heading::South}});
    GameSession edge(map, catalog, places, 5, {{3, 5}, Heading::North}, false, fixture.timings, {}, fixtures);
    edge.setAnimated(false);
    check(bool(edge.interact(edge.nearbyFixture())), "Edge fixture opens door");
    edge.restorePose({{4, 5}, Heading::North}, false);
    const auto id = edge.actors().id(0);
    check(bool(edge.actors().move(id, {3, 4}, {1, 1}, {4, 5})) &&
              bool(edge.actors().move(id, {3, 5}, {1, 1}, {4, 5})) &&
              bool(edge.actors().walk(id, {3, 4}, {1, 1}, {4, 5}, 0, 600)),
          "Edge fixture walks beside the jamb at the viewport boundary");
    const auto scene = world_render::prepare(edge.geometry());
    world_render::VisibilityPipeline pipeline(scene, assets);
    NpcAnimator animator(edge.actors(), fixture.timings);
    for (unsigned time : {450u, 455u, 465u, 480u, 490u}) {
        animator.setTime(time);
        pipeline.update(
            {edge.geometry(), edge.movement(), &edge.actors(), &animator.snapshot(), nullptr, &edge.doors()});
        bool found = false;
        const auto original = actorPosition(*edge.actors().actor(id), time);
        const auto originalPoint =
            world_render::project(pipeline.frame().camera, {original.x, 0, original.y});
        check(originalPoint.x >= 2 && originalPoint.x <= viewWidth - 2,
              "Edge fixture starts from an on-screen actor axis");
        for (unsigned i = 0; i < pipeline.frame().count; ++i) {
            const auto& b = pipeline.frame().billboards[i];
            if (b.id == id.slot && b.alive) {
                found = true;
                const auto point = world_render::project(pipeline.frame().camera, b.feet);
                check(point.x >= 2 && point.x <= viewWidth - 2,
                      "Door clipping preserves the on-screen target axis");
            }
        }
        check(found, "Door-edge clipping retains the visible moving target");
        check(pipeline.frame().picking.targets().count == 1,
              "Door-edge target remains selectable through the actual pixel/alpha gate");
    }
}
void diagonalCombatTarget() {
    test::VisibilityFixture fixture;
    world_assets::CpuAssets assets(fixture.publication());
    const auto catalog = hybridCatalog(false);
    const auto map = hybridRoom();
    const auto scene = world_render::prepare(map);
    world_render::VisibilityPipeline pipeline(scene, assets);
    for (unsigned h = 0; h < 4; ++h)
        for (int side : {-1, 1}) {
            const Player pose{{6, 6}, Heading(h)};
            const auto front = pose.cell + forward(pose.heading),
                       diagonal = front + right(pose.heading) * side;
            Cell offscreen{};
            bool found = false;
            for (int sub = 0; sub < 4 && !found; ++sub) {
                offscreen = {sub % 2, sub / 2};
                found = !inPartyViewport(pose.cell, pose.heading, diagonal, offscreen);
            }
            check(found, "Each orientation includes an offscreen diagonal subslot");
            NpcWorld places(
                map, pose.cell, {"goblin"},
                {{{0}, front, {0, 0}, Heading::South}, {{0}, diagonal, offscreen, Heading::South}});
            GameSession game(map, catalog, places, 5, pose, false);
            for (unsigned i = 0; i < 100 && !(game.combat().snapshot().awaitingAction &&
                                              game.combat().snapshot().activeCharacter->index == 0);
                 ++i) {
                if (game.combat().snapshot().awaitingAction)
                    game.submit(turnCommand(game));
                else
                    game.update(.5, false);
            }
            check(game.combat().snapshot().awaitingAction &&
                      game.combat().snapshot().activeCharacter->index == 0,
                  "Visibility fixture reaches fighter decision");
            // Prepare storage before watching the changed camera and actor set.
            pipeline.update({game.geometry(), game.movement(), &game.actors()});
            game.actors().damage(game.actors().id(0), 1000, game.timeMs());
            watching = true;
            allocations = 0;
            game.update(0, false);
            for (unsigned i = 0; i < 6 && game.combat().snapshot().phase == CombatPhase::Facing; ++i) {
                game.update(.16, false);
                pipeline.update({game.geometry(), game.movement(), &game.actors()});
            }
            pipeline.update({game.geometry(), game.movement(), &game.actors()});
            watching = false;
            check(allocations == 0, "Automatic diagonal facing and rendered views allocate no C++ storage");
            const auto targets = pipeline.frame().picking.targets();
            check(targets.count == 1 && sameActor(targets.ids[0], game.actors().id(1)),
                  "Surviving goblin body is a rendered/pickable target after handoff");
            CommandController commands(game);
            commands.refresh(targets, false, false, true);
            auto command = turnCommand(game, PartyAction::Melee);
            command.target = targets.ids[0];
            check(commands.validate(command) == CommandReason::Ready,
                  "Live command gate accepts the newly visible surviving goblin");
        }
}
void alphaAnchorsAndWalls() {
    test::VisibilityFixture fixture(1, 2, true);
    auto publication = fixture.publication();
    publication.appearances[0].clips[0].width = 1.f;
    world_assets::CpuAssets assets(std::move(publication));
    const auto catalog = creationCatalog(true);
    const auto map = test::worldRoom(9, 9);
    auto scene = world_render::prepare(map);
    world_render::VisibilityPipeline pipeline(scene, assets);
    const Cell cells[]{{1, 4}, {4, 1}, {7, 4}, {4, 7}};
    const Cell players[]{{1, 6}, {2, 1}, {7, 2}, {6, 7}};
    const Cell subcells[]{{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    for (unsigned h = 0; h < 4; ++h) {
        MovementState movement;
        movement.player = {players[h], Heading(h)};
        NpcWorld placements(map, players[h], {"goblin"}, {{{0}, cells[h], subcells[h], Heading(h)}});
        ActorWorld actors(map, catalog, placements);
        pipeline.update({map, movement, &actors});
        const world_render::Billboard* billboard = nullptr;
        for (unsigned i = 0; i < pipeline.frame().count; ++i)
            if (pipeline.frame().billboards[i].id == 0)
                billboard = &pipeline.frame().billboards[i];
        check(billboard && billboard->width == 1.f && billboard->height == .7f,
              "Wide actor retains authored dimensions beside a wall");
        const auto& b = *billboard;
        const auto position = actorPosition(*actors.actor(actors.id(0)), 0);
        check(std::abs(b.feet.x - position.x) < 1e-6f && std::abs(b.feet.z - position.y) < 1e-6f,
              "Clipping preserves the authoritative subcell ground anchor");
        unsigned samples = 0;
        const auto foot = world_render::project(pipeline.frame().camera, b.feet);
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x) {
                if (pipeline.frame().picking.slot(x, y) != 0)
                    continue;
                ++samples;
                const float lateral = (x + .5f - foot.x) * b.depth / world_render::focal();
                const auto right = pipeline.frame().camera.right;
                const float wx = b.feet.x + right.x * lateral, wz = b.feet.z + right.z * lateral;
                check(wx >= 1 - 1e-5f && wx <= 8 + 1e-5f && wz >= 1 - 1e-5f && wz <= 8 + 1e-5f,
                      "Pickable silhouette never extends inside neighboring walls");
            }
        check(samples > 0, "Wall clipping preserves the exposed part of a wide creature");
        const auto canonical = assets.publication.appearances[0].clips[0].textures[0][0];
        const auto& texture = assets.publication.textures[canonical];
        const float top = float(texture.canvas.y + assets.bounds[canonical].top) / texture.canvas.height;
        const auto head = world_render::project(
            pipeline.frame().camera,
            {b.feet.x, b.feet.y + b.height * (assets.publication.anchors[canonical].y - top), b.feet.z});
        check(top > 0 && pipeline.picking()->targets().anchors[0].badgeTop == int(std::round(head.y)),
              "Transparent canvas margin preserves the full-size badge registration");
    }
    // The reported mixed undead pair keeps distinct front/rear slots even though
    // their projections may overlap. Exercise moving into a used slot.
    const auto undead = testMonsterCatalog({"skeleton", "zombie"});
    NpcWorld placements(map, {4, 6}, {"skeleton", "zombie"}, {{{0}, {4, 4}, {0, 0}}, {{1}, {4, 4}, {0, 1}}});
    ActorWorld actors(map, undead, placements);
    check(actors.walk(actors.id(1), {4, 4}, {0, 0}, {4, 6}, 0).error == ActorError::Occupied,
          "mixed undead pair cannot walk into the same logical subcell");
}
} // namespace
int main() {
    try {
        test::movingVisibilityWork();
        test::creatureIntersectionMasks();
        test::creatureMaskExposure();
        spritesAtDoorLeaves();
        diagonalCombatTarget();
        alphaAnchorsAndWalls();
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
            check(actual.candidateCount == expected.candidateCount, "Windowless/live candidate count");
            for (unsigned i = 0; i < actual.candidateCount; ++i)
                check(sameActor(actual.candidates[i], expected.candidates[i]),
                      "Windowless/live candidate identity");
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

#include "game/perception.hpp"
#include "render/world_visibility.hpp"
#include "support/allocations.hpp"
#include "support/architecture_fixtures.hpp"
#include "support/check.hpp"
#include <iostream>
#include <limits>
using namespace blobber;
int main() try {
    test::ArchitectureFixture fixture(true);
    auto publication = fixture.publication();
    publication.verify();
    check(publication.packs[0].daylightFog == std::array<float, 3>{10.f, 4.5f, 1.f},
          "Unconfigured packs preserve legacy daylight fog");
    {
        test::ArchitectureFixture hazeFixture(false, false, {4.f, 12.f, .22f});
        const auto haze = hazeFixture.publication();
        haze.verify();
        check(haze.packs[0].daylightFog == std::array<float, 3>{4.f, 12.f, .22f},
              "Capped pack atmosphere survives publication");
        for (const auto badFog : {std::array<float, 3>{-1, 12, .22f},
                                  {4, 0, .22f},
                                  {4, 65, .22f},
                                  {4, 12, 1.01f},
                                  {4, 12, -.01f},
                                  {std::numeric_limits<float>::infinity(), 12, .22f}}) {
            auto invalidFog = haze;
            invalidFog.packs[0].daylightFog = badFog;
            rejects([&] { world_assets::CpuAssets a(invalidFog); }, "Invalid fog profile rejects");
        }
    }
    check(publication.meshes[2].cutout, "Cutout mesh publication roundtrips");
    {
        test::ArchitectureFixture waterFixture(false, true);
        auto water = waterFixture.publication();
        water.verify();
        check(water.meshes[2].water && !water.meshes[2].cutout, "Opaque water publication roundtrips");
        auto invalidWater = water;
        invalidWater.meshes[2].cutout = true;
        rejects([&] { world_assets::CpuAssets a(invalidWater); }, "Animated cutout water rejects");
        invalidWater = water;
        invalidWater.meshes[2] = water.readMesh(2);
        invalidWater.meshes[2].vertices[0][6] = .3f;
        rejects([&] { world_assets::CpuAssets a(invalidWater); },
                "Water UV outside first atlas frame rejects");
    }
    check(publication.packs[0].viewDistance == 16, "Other packs retain the 16-cell clip distance");
    {
        test::ArchitectureFixture distant(false, false, {4, 12, .22f}, 40);
        const auto extended = distant.publication();
        check(extended.packs[0].viewDistance == 40, "Extended view distance survives publication");
        for (float badDistance : {15.f, 41.f, std::numeric_limits<float>::infinity()}) {
            auto invalid = extended;
            invalid.packs[0].viewDistance = badDistance;
            rejects([&] { world_assets::CpuAssets a(invalid); }, "Invalid view distance rejects");
        }
        world_assets::CpuAssets farAssets(extended);
        const Map farMap(test::architectureRows(48, {40, 24}), test::architectureWorld(48, {40, 24}));
        const auto scene = world_render::prepare(farMap, {}, {}, {}, &farAssets);
        MovementState movement;
        movement.player = {{7, 40}, Heading::East};
        movement.transition = {{{7, 40}, Heading::North}, movement.player, Action::TurnRight, .08f, .16f};
        const auto camera = world_render::camera(movement, farMap, scene.farClip);
        world_render::DrawList draws;
        world_render::cull(scene, camera, draws);
        bool house = false;
        for (unsigned i = 0; i < draws.count; ++i)
            house |= scene.ranges[draws.ranges[i]].painted;
        check(house, "Extended diagonal frustum includes architecture beyond old candidate radius");
        check(draws.tested <= world_render::buildingCullChunks, "Extended candidate storage is bounded");
        check(!world_render::sameCamera(camera, world_render::camera(movement, farMap)),
              "Clip distance participates in camera cache identity");
        world_render::VisibilityPipeline visibility(scene, farAssets);
        movement.player = {{40, 4}, Heading::South};
        movement.transition = {};
        visibility.update({farMap, movement});
        const auto& frame = visibility.frame();
        const auto wall = world_render::project(frame.camera, {40.5f, .8f, 24.2f});
        const auto pixel = std::size_t(int(wall.y)) * viewWidth + int(wall.x);
        check(std::abs(frame.picking.opaqueDepth(pixel) - wall.depth) < .02f,
              "CPU raster retains distant house beyond 16 cells");
        movement.player = {{40, 4}, Heading::North};
        const auto away = world_render::camera(movement, farMap, scene.farClip);
        check(!world_render::visible(away, 39, -39, 2, 1, 0, 2),
              "Geometry beyond extended far plane remains clipped");
        allocations = 0;
        watching = true;
        for (unsigned i = 0; i < 4; ++i) {
            movement.player.heading = Heading(i);
            visibility.update({farMap, movement}, false);
        }
        watching = false;
        check(!allocations, "Extended CPU visibility reuses prepared storage");
    }
    world_assets::CpuAssets assets(publication);
    const auto definition = test::sceneryWorld();
    const auto rows = test::sceneryRows();
    const Map map(rows, definition);
    for (unsigned turn = 0; turn < 4; ++turn) {
        auto decorative = definition;
        decorative.scenery[0].blocking = false;
        decorative.scenery[0].facing = Heading(turn);
        const Map loose(rows, decorative);
        const auto cell = decorative.scenery[0].anchor;
        check(!loose.movementBlocked(cell) && !loose.npcBlocked(cell) &&
                  loose.canStep(cell + forward(Heading(turn)), cell),
              "Decorative scenery has no movement or NPC reservation in any orientation");
        check(loose.definition().fingerprint() != map.definition().fingerprint(),
              "Blocking behavior participates in world identity");
        const auto visible = world_render::prepare(loose, {}, {}, {}, &assets);
        check(visible.meshBytes() > 0, "Nonblocking model still prepares visible geometry");
        decorative.scenery.push_back(decorative.scenery[0]);
        rejects([&] { Map overlap(rows, decorative); }, "Decorative visual bases still reject overlap");
    }
    {
        auto suspended = definition;
        for (auto& surface : suspended.surfaces)
            surface.ceiling = 8;
        auto ceiling = suspended.scenery[0];
        ceiling.blocking = false;
        ceiling.clearance = 1.25f;
        suspended.scenery.push_back(ceiling);
        const Map layered(rows, suspended);
        check(layered.movementBlocked(ceiling.anchor), "Ceiling overlap preserves floor furniture collision");
        suspended.scenery.erase(suspended.scenery.begin());
        const Map overhead(rows, suspended);
        check(!overhead.movementBlocked(ceiling.anchor) && !overhead.npcBlocked(ceiling.anchor),
              "Ceiling geometry alone reserves no party or NPC cells");
        auto ground = suspended;
        ground.scenery[0].clearance = 0;
        check(ground.fingerprint() != suspended.fingerprint(), "Clearance participates in world identity");
        for (float bad : {-1.f, .5f, 2.f, std::numeric_limits<float>::quiet_NaN()}) {
            suspended.scenery[0].clearance = bad;
            rejects([&] { Map invalid(rows, suspended); }, "Invalid or ceiling-exceeding clearance rejects");
        }
    }
    check(map.movementBlocked({7, 6}) && map.npcBlocked({7, 6}) && !map.solid({7, 6}) &&
              !map.buildingOwned({7, 6}),
          "Scenery blocks movement without becoming terrain");
    check(lineOfSight(map, {7, 4}, {7, 8}) && map.connected({7, 4}),
          "Scenery leaves cell sight and surrounding paths open");
    check(map.canStep({6, 5}, {7, 5}) && !map.canStep({7, 5}, {7, 6}),
          "Overhang does not enlarge physical reservation");
    const std::array<Cell, 4> rotatedBases{{{8, 6}, {7, 7}, {6, 6}, {7, 5}}};
    for (unsigned h = 0; h < 4; ++h) {
        auto rotated = definition;
        rotated.scenery[0].width = 2;
        rotated.scenery[0].facing = Heading(h);
        const Map placed(rows, rotated);
        check(placed.movementBlocked(rotatedBases[h]) && !placed.solid(rotatedBases[h]),
              "Cardinal rotation moves the rectangular base without adding terrain");
    }
    auto bad = definition;
    bad.scenery.push_back(bad.scenery[0]);
    rejects([&] { Map m(rows, bad); }, "Overlapping bases reject");
    bad = definition;
    bad.scenery[0].anchor = {0, 0};
    rejects([&] { Map m(rows, bad); }, "Solid/outside base rejects");
    bad.scenery[0].anchor = {std::numeric_limits<int>::max(), 6};
    rejects([&] { Map m(rows, bad); }, "Invalid anchor rejects before footprint arithmetic");
    bad = definition;
    bad.scenery[0].width = 8;
    rejects([&] { Map m(rows, bad); }, "Oversized physical footprint rejects");
    bad = definition;
    bad.scenery.resize(sceneryCapacity + 1);
    rejects([&] { Map m(rows, bad); }, "Scenery capacity rejects");
    bad = definition;
    bad.scenery[0].width = 2;
    bad.surfaces[6 * 12 + 8].floor = 1;
    rejects([&] { Map m(rows, bad); }, "Unlevel base rejects");
    bad = definition;
    bad.decorations.push_back({{7, 6}, Heading::North, DecorationKind::Cage});
    rejects([&] { Map m(rows, bad); }, "Furniture and scenery reservations cannot overlap");
    bad = definition;
    bad.scenery[0].model = "missing";
    const Map missing(rows, bad);
    rejects([&] { world_render::prepare(missing, {}, {}, {}, &assets); }, "Missing model rejects");
    check(!assets.residency().meshes, "Preparation failure releases dependencies");
    rejects([&] { world_render::prepare(map); }, "Scenery requires prepared assets");
    bad = definition;
    bad.scenery[0].facing = Heading::West;
    check(bad.fingerprint() != definition.fingerprint(), "Scenery transform enters world identity");
    auto invalid = publication;
    invalid.meshes[2].cutout = false;
    rejects([&] { world_assets::CpuAssets a(invalid); }, "Opaque mesh cannot use cutout publication");
    invalid = publication;
    invalid.meshes[0].cutout = true;
    rejects([&] { world_assets::CpuAssets a(invalid); }, "Cutout mesh requires alpha data");
    {
        const auto lease = assets.acquire(assets.requirements("architecture"));
        const auto scene = world_render::prepare(map, {}, {}, {}, &assets);
        check(scene.cullRadius == world_render::buildingCullRadius && scene.chunks[0].width >= 9,
              "Scenery overhang expands chunk and uses architecture candidate coverage");
        bool floor = false;
        for (const auto& range : scene.ranges)
            for (unsigned i = range.first; i < range.first + range.count; ++i) {
                const auto& v = scene.vertices[scene.indices[i]];
                floor |= !range.painted && v.normal.y == 1 && v.position.x == 7 && v.position.z == 6;
            }
        check(floor, "Floor remains under scenery base");
        world_render::VisibilityPipeline visibility(scene, assets);
        MovementState movement;
        for (const auto heading : {Heading::South, Heading::North}) {
            movement.player = {{7, heading == Heading::South ? 4 : 8}, heading};
            visibility.update({map, movement});
            const auto& frame = visibility.frame();
            const auto hole = world_render::project(frame.camera, {7.5f, .5f, 6.5f});
            const auto leaf = world_render::project(frame.camera, {7.0f, .5f, 6.5f});
            check(frame.picking.depth(std::size_t(int(hole.y)) * viewWidth + int(hole.x)) > hole.depth + .5f,
                  "Transparent opening preserves background depth from both sides");
            check(std::abs(frame.picking.opaqueDepth(std::size_t(int(leaf.y)) * viewWidth + int(leaf.x)) -
                           leaf.depth) < .02f,
                  "Covered foliage contributes opaque depth");
        }
        movement.player = {{7, 4}, Heading::South};
        visibility.update({map, movement}, false);
        allocations = 0;
        watching = true;
        for (unsigned i = 0; i < 4; ++i) {
            movement.transition = {
                movement.player, {{6, 4}, Heading::South}, Action::Right, .04f + .02f * i, .16f};
            visibility.update({map, movement}, false);
            movement.transition = {};
            visibility.update({map, movement}, false);
        }
        watching = false;
        check(!allocations, "Prepared scenery CPU frames allocate nothing");
        check(!visibility.update({map, movement}), "Idle scenery retains cached visibility");
    }
    check(!assets.residency().meshes && !assets.residency().alphaBytes,
          "Scenery and alpha release after final lease");
    std::cout << "Scenery placement, cutout publication/depth and allocation checks passed\n";
} catch (const std::exception& e) {
    watching = false;
    std::cerr << e.what() << '\n';
    return 1;
}

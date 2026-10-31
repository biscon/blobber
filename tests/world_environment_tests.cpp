#include "game/navigation.hpp"
#include "game/perception.hpp"
#include "game/session.hpp"
#include "render/world_effects.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include "support/probe_allocations.hpp"
#include "support/world_visibility_fixtures.hpp"
#include "world/environment_maps.hpp"
#include <iostream>
using namespace blobber;
int main(int argc, char**) {
    try {
        const std::vector<std::string> rows{"#########", "#.......#", "#.......#", "#.......#", "#.......#",
                                            "#.......#", "#.......#", "#.......#", "#########"};
        WorldDefinition d;
        d.surfaces.resize(81);
        for (auto& s : d.surfaces)
            s.ceiling = 8;
        for (int y = 1; y <= 3; ++y)
            for (int x = 1; x < 8; ++x)
                d.surfaces[y * 9 + x].floor = 1;
        d.surfaces[2 * 9 + 6].floor = 4;
        d.surfaces[6 * 9 + 3].ceiling = 4;
        d.surfaces[6 * 9 + 4].floor = 1;
        Map map(rows, d);
        check(map.canStep({3, 4}, {3, 3}) && map.canStep({3, 3}, {3, 4}),
              "Quarter-cell step works both ways");
        check(!map.canStep({5, 2}, {6, 2}) && !map.canStep({3, 6}, {4, 6}),
              "High ledge and low aperture reject");
        for (unsigned h = 0; h < 4; ++h) {
            Player p{{3, 4}, Heading(h)};
            const Action actions[]{Action::Forward, Action::Left, Action::Back, Action::Right};
            check(tryAction(map, p, actions[h]) && p.cell == Cell{3, 3}, "All headings share edge traversal");
        }
        Player high{{5, 2}, Heading::East};
        check(!tryAction(map, high, Action::Forward), "Player rejects high ledge");
        check(!combatAdjacent(map, {5, 2}, {6, 2}), "Melee cannot reach high ledge");
        const auto catalog = creationCatalog(true);
        NpcWorld placements(map, {3, 7}, {"goblin"}, {{{0}, {3, 4}, {0, 0}, Heading::North}});
        ActorWorld actors(map, catalog, placements);
        check(actors.walk(actors.id(0), {3, 3}, {0, 0}, {3, 7}, 0).changed, "Actor walks legal step");
        check(std::abs(actorElevation(*actors.actor(actors.id(0)), map, 300) - .125f) < 1e-6f,
              "Actor height interpolates continuously");
        actors.damage(actors.id(0), 10000, 300);
        check(std::abs(actorElevation(*actors.actor(actors.id(0)), map, 1000) - .125f) < 1e-6f,
              "Death freezes step elevation");
        NpcWorld navPlacements(map, {3, 7}, {"goblin"}, {{{0}, {5, 2}, {0, 0}, Heading::East}});
        ActorWorld navActors(map, catalog, navPlacements);
        Navigation navigation(map, navActors);
        navigation.request(0, {5, 2}, {6, 2}, false);
        navigation.update({3, 7});
        const auto impossible = navigation.take(0);
        check(impossible.ready && !impossible.found, "Navigation rejects isolated high ledge");
        navigation.request(0, {3, 4}, {3, 3}, false);
        navigation.update({3, 7});
        const auto legal = navigation.take(0);
        check(legal.ready && legal.found && legal.step == Cell{3, 3}, "Navigation agrees on legal step");
        check(!navActors.move(navActors.id(0), {6, 2}, {0, 0}, {3, 7}).changed,
              "Checked actor move rejects illegal BFS edge");
        auto roof = d;
        roof.surfaces[4 * 9 + 6].floor = 4;
        Map openSight(rows, roof);
        check(lineOfSight(openSight, {2, 4}, {6, 4}), "Height-varying ray passes tall aperture");
        roof.surfaces[4 * 9 + 4].ceiling = 4;
        Map lowRoof(rows, roof);
        check(!lineOfSight(lowRoof, {2, 4}, {6, 4}) && !lineOfSight(lowRoof, {6, 4}, {2, 4}),
              "Ceiling lintel blocks sloped sight symmetrically");
        const NpcWorld empty(map, {3, 4}, {"goblin"}, {});
        GameSession game(map, catalog, empty, 5, {{3, 4}, Heading::North}, false);
        game.move(Action::Forward);
        game.restorePose({{3, 6}, Heading::East}, true);
        check(!game.movement().transition.active(), "Preparation pose restore clears old interpolation");
        game.move(Action::Forward);
        check(game.movement().blocked, "Session revalidates clearance after restore");
        auto corner = d;
        corner.surfaces[3 * 9 + 3].floor = 4;
        Map cornerBlock(rows, corner);
        check(!lineOfSight(cornerBlock, {3, 4}, {4, 3}) && !lineOfSight(cornerBlock, {4, 3}, {3, 4}),
              "Height-aware supercover blocks both corner directions");
        auto blocked = d;
        for (int y = 1; y < 8; ++y)
            blocked.surfaces[y * 9 + 4].floor = 3;
        Map ledge(rows, blocked);
        check(!lineOfSight(ledge, {2, 4}, {6, 4}) && !lineOfSight(ledge, {6, 4}, {2, 4}),
              "Raised floor blocks sight symmetrically");
        auto invalid = d;
        invalid.surfaces[0].floor = 33;
        rejects([&] { Map m(rows, invalid); }, "Elevation bounds reject");
        invalid = d;
        invalid.surfaces[4 * 9 + 4].ceiling = 3;
        rejects([&] { Map m(rows, invalid); }, "Insufficient cell clearance rejects");
        const auto hub = sanctuaryMap();
        check(hub.flat() && hub.connected({4, 4}), "Outdoor hub retains flat connectivity");
        check(hub.surface({3, 4}).ceiling == noCeiling && hub.surface({4, 4}).ceiling == 5,
              "Hub roof ends at courtyard");
        auto hs = world_render::prepare(hub);
        check(hs.outdoor && hs.decals.size() == 2, "Sky and wall cutouts prepare");
        for (std::size_t i = 0; i < hs.indices.size(); i += 3) {
            const auto a = hs.vertices[hs.indices[i]], b = hs.vertices[hs.indices[i + 1]],
                       c = hs.vertices[hs.indices[i + 2]];
            const auto u = world_render::Vec{b.position.x - a.position.x, b.position.y - a.position.y,
                                             b.position.z - a.position.z};
            const auto v = world_render::Vec{c.position.x - a.position.x, c.position.y - a.position.y,
                                             c.position.z - a.position.z};
            const float dot = (u.y * v.z - u.z * v.y) * a.normal.x + (u.z * v.x - u.x * v.z) * a.normal.y +
                              (u.x * v.y - u.y * v.x) * a.normal.z;
            check(dot >= 0, "Prop/world triangle winding agrees with normals");
        }
        bool benchFront = false;
        for (const auto& v : hs.vertices)
            benchFront |= std::abs(v.position.x - 1.2f) < 1e-5f && v.position.z > 6 && v.position.z < 7 &&
                          v.position.y <= .2f && v.normal.x > .9f;
        check(benchFront, "Bench front faces outward toward the courtyard");
        invalid = hub.definition();
        invalid.decorations.push_back(invalid.decorations[0]);
        rejects([&] { Map m(rows, invalid); }, "Overlapping decals reject");
        invalid = hub.definition();
        invalid.decorations.back().height = .5f;
        rejects([&] { Map m(rows, invalid); }, "Tall nonblocking prop rejects");
        test::VisibilityFixture fixture;
        world_assets::CpuAssets assets(fixture.publication());
        auto unavailable = d;
        unavailable.surfaces[4 * 9 + 4].floorMaterial = 31;
        Map badMaterial(rows, unavailable);
        const auto badScene = world_render::prepare(badMaterial);
        rejects([&] { world_render::VisibilityPipeline rejected(badScene, assets); },
                "Windowless preparation rejects unavailable material references");
        auto scene = world_render::prepare(map);
        world_render::VisibilityPipeline visibility(scene, assets);
        MovementState movement;
        movement.player = {{3, 5}, Heading::North};
        world_render::FrameInput input{map, movement};
        visibility.update(input);
        const auto p = world_render::project(visibility.frame().camera, {3.5f, .25f, 3.5f});
        check(visibility.groundCell(input, int(p.x), int(p.y)) == Cell{3, 3},
              "Ground picking finds raised floor");
        Controller controller({{3, 4}, Heading::North});
        controller.submit(map, Action::Forward);
        controller.update(map, .08f);
        check(std::abs(world_render::camera(controller.state(), map).position.y - .625f) < 1e-5f,
              "Camera follows step easing");
        world_render::EffectsPipeline effects(assets);
        SpellArea area;
        area.ground.revision = 1;
        area.ground.areas[0].active = true;
        area.ground.areas[0].cell = {3, 3};
        area.ground.weapons[0] = {{3, 3}, true};
        input.area = &area;
        effects.update(input, visibility.frame());
        check(effects.frame().count == 2 && std::abs(effects.frame().packets[0].points[0].y - .2515f) < 1e-5f,
              "Ground effect follows raised surface");
        FloorFixtures doors;
        doors.count = 1;
        doors.entries[0] = {{4, 3}, Heading::East, FixtureKind::Door};
        FloorGeometry geometry(map, doors);
        std::array<world_render::DoorBox, world_render::doorBoxCapacity> boxes{};
        world_render::doorBoxes(geometry.view(), nullptr, boxes, 0, 0, 0);
        check(std::abs(boxes[0].origin.y - .25f) < 1e-5f && std::abs(boxes[0].y.y - 1.75f) < 1e-5f,
              "Door pivot and frame fit raised aperture");
        const auto env = rendererHeightMap();
        check(!env.flat(), "Review fixture is genuinely elevated");
        watching = true;
        allocations = 0;
        for (unsigned i = 0; i < 60; ++i) {
            movement.player.heading = Heading(i % 4);
            visibility.update(input);
            (void)visibility.groundCell(input, 250, 230);
        }
        watching = false;
        check(allocations == 0, "Height visibility/picking allocate no warmed C++ storage");
        if (argc > 1) {
            std::vector<std::string> large(128, std::string(128, '.'));
            for (int i = 0; i < 128; ++i)
                large[0][i] = large[127][i] = large[i][0] = large[i][127] = '#';
            WorldDefinition w;
            w.surfaces.resize(128 * 128);
            for (unsigned i = 0; i < w.surfaces.size(); ++i) {
                w.surfaces[i].floor = i % 2;
                w.surfaces[i].ceiling = 8;
            }
            for (int y = 1; y < 127; ++y) {
                w.decorations.push_back({{1, y}, Heading::West, DecorationKind::Crest});
                w.decorations.push_back({{126, y}, Heading::East, DecorationKind::Vine});
                w.decorations.push_back(
                    {{1, y}, Heading::West, DecorationKind::Bench, 0, .5f, 0, .7f, .2f, .2f});
            }
            for (int x = 2; x < 6; ++x)
                w.decorations.push_back({{x, 1}, Heading::North, DecorationKind::Crest});
            for (int x = 2; x < 4; ++x)
                w.decorations.push_back(
                    {{x, 1}, Heading::North, DecorationKind::Urn, 0, .5f, 0, .2f, .25f, .2f});
            Map stress(large, std::move(w));
            auto built = world_render::prepare(stress);
            check(built.meshBytes() > 1000000, "Dense-height stress retains geometry");
            rejects([&] { world_render::prepare(stress, {}, {}, {10, 10, 1000}); },
                    "Geometry exhaustion rejects");
            check(built.decals.size() == 256, "Maximum decoration records retained");
            std::cout << "Dense height mesh bytes: " << built.meshBytes() << '\n';
        }
        std::cout << "World environment tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}

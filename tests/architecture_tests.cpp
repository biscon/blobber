#include "game/perception.hpp"
#include "render/world_scene.hpp"
#include "support/allocations.hpp"
#include "support/architecture_fixtures.hpp"
#include "support/check.hpp"
#include "world/sanctuary_city.hpp"
#include <fstream>
#include <iostream>
#include <limits>
using namespace blobber;
int main() try {
    test::ArchitectureFixture fixture;
    auto publication = fixture.publication();
    publication.verify();
    check(publication.packs[0].buildings[0].parts.size() == 2, "Multipart model publication roundtrip");
    world_assets::CpuAssets assets(publication);
    check(!assets.residency().meshes, "Architecture catalog opens without payloads");
    const auto rows = test::architectureRows();
    const auto def = test::architectureWorld();
    Map map(rows, def);
    check(map.buildingOwned({7, 7}) && map.movementBlocked({7, 7}) && map.npcBlocked({7, 7}),
          "Building gameplay blockers");
    check(!lineOfSight(map, {7, 6}, {7, 8}) && map.connected({2, 2}),
          "Sight blocks, surrounding street connects");
    auto bad = def;
    bad.buildings[0].anchor = {2, 2};
    rejects([&] { Map m(rows, bad); }, "Non-solid building backing rejected");
    bad = def;
    bad.buildings.push_back(bad.buildings[0]);
    rejects([&] { Map m(rows, bad); }, "Overlapping footprint rejected");
    bad = def;
    bad.buildings[0].width = 8;
    rejects([&] { Map m(rows, bad); }, "Oversized footprint rejected");
    bad = def;
    bad.buildings.resize(buildingCapacity + 1);
    rejects([&] { Map m(rows, bad); }, "Placement capacity rejected");
    bad = def;
    bad.decorations.push_back({{7, 6}, Heading::South, DecorationKind::Vine});
    rejects([&] { Map m(rows, bad); }, "Decoration on replaced wall rejected");
    bad.decorations[0].cell = {std::numeric_limits<int>::max(), 6};
    bad.decorations[0].face = Heading::East;
    rejects([&] { Map m(rows, bad); }, "Invalid decoration coordinate rejects before neighbor arithmetic");
    bad = def;
    bad.buildings[0].model = "missing";
    Map missing(rows, bad);
    rejects([&] { world_render::prepare(missing, {}, {}, {}, &assets); }, "Missing stable model ID rejected");
    check(!assets.residency().meshes, "Failed preparation releases model lease");
    rejects([&] { world_render::prepare(map); }, "Building cannot silently lose its visual shell");
    for (unsigned heading = 0; heading < 4; ++heading) {
        auto d = def;
        d.buildings[0].facing = Heading(heading);
        Map rotated(rows, d);
        auto scene = world_render::prepare(rotated, {}, {}, {}, &assets);
        unsigned painted = 0;
        for (const auto& r : scene.ranges)
            painted += r.painted;
        check(painted == 2, "One range for each painted model material");
        const auto f = forward(Heading(heading));
        const float expectedX = 7.5f + .3f * f.x, expectedZ = 7.5f + .3f * f.y;
        bool recess = false;
        for (const auto& r : scene.ranges)
            for (unsigned n = r.first; n < r.first + r.count; ++n) {
                const auto& v = scene.vertices[scene.indices[n]];
                if (r.painted && v.normal.x == f.x && v.normal.z == f.y)
                    recess |= (f.x ? std::abs(v.position.x - expectedX)
                                   : std::abs(v.position.z - expectedZ)) < 1e-5f;
                if (!r.painted && v.normal.y == 0)
                    check(!(v.position.x >= 7 && v.position.x <= 8 && v.position.z >= 7 && v.position.z <= 8),
                          "Backing terrain faces suppressed");
            }
        check(recess, "Rotation preserves recessed face");
        check(scene.chunks[0].high >= 2.1f && scene.chunks[0].width >= 9, "Roof expands anchor chunk bounds");
        world_render::DrawList list;
        MovementState movement;
        movement.player = {{7, 4}, Heading::South};
        const auto camera = world_render::camera(movement);
        world_render::cull(scene, camera, list);
        check(list.count > 0, "Building visible from approach");
        allocations = 0;
        watching = true;
        for (unsigned i = 0; i < 40; ++i)
            world_render::cull(scene, camera, list);
        watching = false;
        check(allocations == 0, "Prepared architecture culling allocates nothing");
    }
    // A roof can enter the far frustum while its anchor chunk is still outside.
    auto reaching = publication;
    for (unsigned i = 0; i < reaching.meshes.size(); ++i)
        reaching.meshes[i] = reaching.readMesh(i);
    for (auto& v : reaching.meshes[1].vertices)
        if (v[0] < 0)
            v[0] = -4;
    world_assets::CpuAssets reachAssets(reaching);
    const Map distant(test::architectureRows(48, {32, 24}), test::architectureWorld(48, {32, 24}));
    const auto reachScene = world_render::prepare(distant, {}, {}, {}, &reachAssets);
    MovementState approach;
    approach.player = {{13, 24}, Heading::East};
    const auto camera = world_render::camera(approach);
    check(!world_render::visible(camera, 32, 24, 8, 8, 0, 2.1f), "Anchor chunk is beyond far clipping");
    world_render::DrawList reachDraws;
    world_render::cull(reachScene, camera, reachDraws);
    bool roofIncluded = false;
    for (unsigned i = 0; i < reachDraws.count; ++i)
        roofIncluded |= reachScene.ranges[reachDraws.ranges[i]].painted;
    check(roofIncluded, "Geometry-derived bounds retain roof entering frustum");
    const Map diagonal(test::architectureRows(48, {32, 8}), test::architectureWorld(48, {32, 8}));
    const auto diagonalScene = world_render::prepare(diagonal, {}, {}, {}, &reachAssets);
    approach.player = {{7, 9}, Heading::East};
    approach.transition = {{{7, 9}, Heading::North}, approach.player, Action::TurnRight, .08f, .16f};
    const auto turnCamera = world_render::camera(approach);
    check(int(std::floor((turnCamera.position.x + 24) / 8)) < 4,
          "Diagonal overhang owner lies outside the old search neighborhood");
    world_render::cull(diagonalScene, turnCamera, reachDraws);
    roofIncluded = false;
    for (unsigned i = 0; i < reachDraws.count; ++i)
        roofIncluded |= diagonalScene.ranges[reachDraws.ranges[i]].painted;
    check(roofIncluded, "Diagonal turn retains far-corner roof overhang");
    for (unsigned h = 0; h < 4; ++h) {
        auto d = test::architectureWorld();
        d.buildings[0].facing = Heading(h);
        d.buildings[0].width = 2;
        auto rr = test::architectureRows();
        const auto cell = d.buildings[0].cell(1, 0);
        rr[cell.y][cell.x] = '#';
        Map rotated(rr, d);
        check(rotated.buildingOwned(cell), "Rotated multicell reservation follows model");
        d.surfaces[cell.y * 12 + cell.x].floor = 1;
        rejects([&] { Map unlevel(rr, d); }, "Unlevel foundation rejects");
    }
    world_render::Limits limits;
    limits.quads = 1;
    rejects([&] { world_render::prepare(map, {}, {}, limits, &assets); },
            "Geometry capacity rejects rather than omits");
    auto invalid = publication;
    invalid.meshes[0] = invalid.readMesh(0);
    invalid.meshes[0].vertices[0][0] = 4.1f;
    rejects([&] { world_assets::CpuAssets a(invalid); }, "Architecture envelope rejects");
    invalid = publication;
    invalid.packs[0].buildings[0].parts = {511};
    rejects([&] { world_assets::CpuAssets a(invalid); }, "Invalid mesh reference rejects");
    invalid = publication;
    invalid.packs[0].buildings[0].parts = {0, 0};
    rejects([&] { world_assets::CpuAssets a(invalid); }, "Duplicate mesh part rejects");
    invalid = publication;
    invalid.packs[0].buildings.resize(33);
    rejects([&] { world_assets::CpuAssets a(invalid); }, "Catalog capacity rejects");
    {
        const auto one = assets.acquire(assets.requirements("architecture"));
        const auto two = assets.acquire(assets.requirements("architecture"));
        check(assets.residency().meshes == 2, "Active/staged meshes share payloads");
    }
    check(!assets.residency().meshes && !assets.residency().alphaBytes,
          "Last lease releases architecture payloads");
    {
        std::fstream file(publication.payload, std::ios::binary | std::ios::in | std::ios::out);
        file.seekp(std::streamoff(publication.meshes[1].offset));
        char byte = 42;
        file.write(&byte, 1);
    }
    rejects([&] { auto lease = assets.acquire(assets.requirements("architecture")); },
            "Corrupt selected model rejects");
    check(!assets.residency().meshes, "Corrupt part rolls back earlier acquired part");
    const auto city = sanctuaryCityReview();
    check(city.map.connected(city.spawn.cell) && city.map.definition().buildings.size() == 2,
          "Authored review route connects");
    check(city.map.canStep({7, 7}, {8, 7}) && city.map.canStep({8, 7}, {8, 8}) &&
              city.map.canStep({7, 7}, {7, 8}),
          "Review corner retains a two-cell turning street");
    std::cout << "Architecture CPU/publication checks passed\n";
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}

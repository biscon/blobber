#include "assets/blueprint_art.hpp"
#include "assets/blueprint_blockouts.hpp"
#include "render/world_doors.hpp"
#include "render/world_visibility.hpp"
#include "support/allocations.hpp"
#include "support/blueprint_fixtures.hpp"
#include "support/check.hpp"
#include "support/fixture_world.hpp"
#include "support/world_fixtures.hpp"
#include <iostream>
#include <limits>
using namespace blobber;
namespace bp = blueprints;
namespace {
void doorSweeps() {
    const auto catalog = test::blueprintCatalog();
    const auto rules = fixtureCatalog();
    for (unsigned turn = 0; turn < 4; ++turn) {
        const auto scene = bp::assemble(catalog, "kitchen-hall", turn);
        const NpcWorld empty(scene.map, scene.spawn.cell, {"goblin"}, {});
        auto game = std::make_unique<GameSession>(scene.map, rules, empty, 5, scene.spawn, false,
                                                  std::vector<NpcTimings>{}, RecoveryLevel{}, scene.fixtures);
        for (unsigned i = 0; i < scene.fixtures.count; ++i) {
            const auto& f = scene.fixtures.entries[i];
            game->restorePose({f.cell + forward(f.facing), bp::rotate(f.facing, 2)}, true);
            game->discover();
            check(bool(game->interact(fixtureRequest(*game, i))), "Door sweep fixture opens normally");
            for (unsigned sample = 0; sample <= 4; ++sample) {
                std::array<world_render::DoorBox, world_render::doorBoxCapacity> boxes;
                const auto count = world_render::doorBoxes(game->geometry(), &game->doors(), boxes, 0, 0, 0);
                for (unsigned b = 0; b < count; ++b)
                    if (boxes[b].leaf && boxes[b].id == PickingBuffer::fixtureBase + i)
                        for (const auto& p : world_render::corners(boxes[b]))
                            check(p.x >= f.cell.x - .00001f && p.x <= f.cell.x + 1.00001f &&
                                      p.z >= f.cell.y - .00001f && p.z <= f.cell.y + 1.00001f,
                                  "Actual door swing stays inside reserved threshold in every orientation");
                game->update(.075, false);
            }
        }
    }
}
} // namespace
int main() try {
    doorSweeps();
    // Resizing a furnishing must release its old cells, including after rotation.
    const auto original = test::blueprintCatalog();
    auto compact = original;
    compact.kits[0].size = {1, 1};
    compact.kits[0].height = .15f;
    for (unsigned turn = 0; turn < 4; ++turn) {
        const auto before = bp::assemble(original, "kitchen", turn);
        const auto after = bp::assemble(compact, "kitchen", turn);
        const auto released = before.map.definition().scenery.front().cell(0, 1);
        check(before.map.movementBlocked(released) && !after.map.movementBlocked(released) &&
                  !after.map.npcBlocked(released),
              "Compact furniture releases the former footprint for party and NPC movement");
    }
    test::WorldFixture storage;
    auto c = test::blueprintCatalog();
    // Own kit fixtures cover every blockout primitive without loading game content.
    const std::array<const char*, 15> names{
        "hearth",       "work-surface", "pottery-shelves", "table-and-benches",
        "sleeping-bay", "footlocker",   "weapon-storage",  "armor-display",
        "barrel",       "crate-large",  "crate-small",     "grain-sack",
        "tapestry",     "brazier",      "chandelier"};
    const std::array<Cell, 15> sizes{{{1, 1},
                                      {1, 1},
                                      {1, 1},
                                      {1, 2},
                                      {1, 2},
                                      {1, 1},
                                      {2, 1},
                                      {1, 1},
                                      {1, 1},
                                      {1, 1},
                                      {1, 1},
                                      {1, 1},
                                      {1, 1},
                                      {1, 1},
                                      {3, 3}}};
    const std::array<float, 15> heights{.5f,  .38f, .82f, .34f, .6f,   .2f,  .8f, .8f,
                                        .34f, .23f, .14f, .17f, 1.62f, .38f, 2.f};
    for (unsigned i = 0; i < names.size(); ++i) {
        bp::Kit k;
        std::copy_n(names[i], std::strlen(names[i]), k.id.begin());
        k.size = sizes[i];
        k.height = heights[i];
        k.blocking = (i < 7 && i != 2) || i == 13;
        k.closedWall = i == 2;
        k.clearance = i == 14 ? 1.25f : 0;
        c.kits[c.kitCount++] = k;
    }
    world_assets::Writer writer(storage.directory);
    std::vector<unsigned char> rgba(8 * 8 * 4, 255);
    const auto tex = std::uint16_t(writer.add("terrain", 8, 8, false, rgba));
    world_assets::Pack pack;
    pack.id = "underkeep";
    pack.materialCount = 3;
    pack.materials.fill(tex);
    pack.door = pack.lockedDoor = pack.doorEdge = pack.sky =
        std::uint16_t(writer.add("door", 8, 8, false, rgba, 0, {}, false));
    const auto cutout = std::uint16_t(writer.add("cutout", 8, 8, true, rgba));
    pack.crest = pack.vine = cutout;
    for (const auto* id : {"loot/bag", "fixture/key-bronze", "fixture/key-silver", "fixture/upstairs",
                           "fixture/downstairs", "fixture/endpoint", "fixture/threshold"})
        writer.add(id, 8, 8, true, rgba);
    std::vector<world_assets::Pack> packs{pack};
    std::vector<world_assets::PropMesh> meshes;
    bp::appendBlockouts(c, writer, packs, meshes);
    check(packs.size() == 2 && packs[0].buildings.empty() && packs[0].id == "underkeep",
          "Review pack leaves source binding unchanged");
    rejects([&] { bp::appendBlockouts(c, writer, packs, meshes); },
            "Duplicate review pack rejects before adding assets");
    // A generated named-model publication exercises the finished path without game artwork.
    packs[0].buildings = packs[1].buildings;
    packs[0].materials = packs[1].materials;
    packs[0].materialCount = packs[1].materialCount;
    for (unsigned i = 0; i < c.kitCount; ++i) {
        bp::validateKitArt(c.kits[i], meshes[i]);
        auto low = meshes[i];
        low.vertices[0][1] = c.kits[i].clearance - .01f;
        rejects([&] { bp::validateKitArt(c.kits[i], low); },
                "Geometry cannot intrude below promised clearance");
        for (unsigned axis = 0; axis < 3; ++axis) {
            auto bad = meshes[i];
            bad.vertices[0][axis] = axis == 1 ? c.kits[i].height + .01f : 4;
            rejects([&] { bp::validateKitArt(c.kits[i], bad); }, "Kit overflow rejects on each axis");
        }
    }
    auto bad = meshes[0];
    bad.vertices[0][0] = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { bp::validateKitArt(c.kits[0], bad); }, "Nonfinite kit data rejects");
    bad = meshes[0];
    bad.vertices[0][1] = -.01f;
    rejects([&] { bp::validateKitArt(c.kits[0], bad); }, "Below-floor geometry rejects");
    bad = meshes[0];
    bad.vertices.resize(6003, bad.vertices[0]);
    rejects([&] { bp::validateKitArt(c.kits[0], bad); }, "Kit triangle budget rejects");
    for (unsigned field : {3u, 6u}) {
        auto invalid = meshes;
        invalid[0].vertices[0][field] = 2;
        rejects([&] { writer.finish({}, 0, packs, invalid); }, "Publication rejects invalid normal/UV");
    }
    writer.finish({}, 0, packs, meshes);
    auto publication = world_assets::load(storage.directory);
    publication.verify();
    for (unsigned turn = 0; turn < 4; ++turn) {
        const auto finished = bp::assemble(c, "kitchen-hall", turn, 0, "underkeep");
        const auto blockout = bp::assemble(c, "kitchen-hall", turn);
        check(finished.map.definition().assetPack == "underkeep", "Finished assembly selects requested pack");
        for (int y = 0; y < finished.map.height(); ++y)
            for (int x = 0; x < finished.map.width(); ++x)
                check(finished.map.movementBlocked({x, y}) == blockout.map.movementBlocked({x, y}),
                      "Art selection preserves collision");
        bp::validatePublishedArt(c, finished, publication);
    }
    const auto finished = bp::assemble(c, "kitchen", 0, 0, "underkeep");
    auto missing = publication;
    missing.packs[0].buildings.clear();
    rejects([&] { bp::validatePublishedArt(c, finished, missing); }, "Missing finished kit never falls back");
    missing.packs.clear();
    rejects([&] { bp::validatePublishedArt(c, finished, missing); }, "Missing art pack rejects");
    auto split = publication;
    split.packs[0].buildings[0].parts.push_back(split.packs[0].buildings[0].parts[0]);
    rejects([&] { bp::validatePublishedArt(c, finished, split); }, "Unexpected multipart kit rejects");
    world_assets::CpuAssets assets(publication);
    {
        const auto lease = assets.acquire(world_assets::allAssets(publication));
        for (unsigned i = 0; i < c.kitCount; ++i)
            for (unsigned t = 0; t < 4; ++t) {
                std::vector<std::string> rows(13, std::string(13, '.'));
                for (int n = 0; n < 13; ++n)
                    rows[0][n] = rows[12][n] = rows[n][0] = rows[n][12] = '#';
                WorldDefinition world;
                world.assetPack = "underkeep";
                world.surfaces.resize(169);
                for (auto& surface : world.surfaces) {
                    surface.floor = 1;
                    surface.ceiling = 9;
                }
                const auto& kit = c.kits[i];
                const Heading heading = Heading(t);
                world.scenery.push_back(
                    {kit.id.data(), {6, 6}, {0, 0}, heading, unsigned(kit.size.x), unsigned(kit.size.y)});
                const Map map(rows, world);
                const auto scene = world_render::prepare(map, {}, {}, {}, &assets);
                const auto r = right(heading), f = forward(heading);
                unsigned vertices = 0;
                for (const auto& range : scene.ranges)
                    if (range.painted)
                        for (unsigned v = range.first; v < range.first + range.count; ++v) {
                            const auto p = scene.vertices[scene.indices[v]].position;
                            const float x = (p.x - 6.5f) * r.x + (p.z - 6.5f) * r.y,
                                        z = -(p.x - 6.5f) * f.x - (p.z - 6.5f) * f.y;
                            check(x >= -.50001f && x <= kit.size.x - .49999f && z >= -.50001f &&
                                      z <= kit.size.y - .49999f && p.y >= .24999f &&
                                      p.y <= .25f + kit.height + .00001f,
                                  "Prepared rotated model obeys footprint and raised floor anchor");
                            ++vertices;
                        }
                check(vertices >= 36, "Prepared blockout contains visible geometry");
                world_render::VisibilityPipeline visibility(scene, assets);
                MovementState movement;
                movement.player = {{6, 10}, Heading::North};
                visibility.update({map, movement});
                allocations = 0;
                watching = true;
                for (unsigned turn = 0; turn < 4; ++turn) {
                    movement.player.heading = Heading(turn);
                    visibility.update({map, movement});
                }
                watching = false;
                check(!allocations, "Prepared blueprint CPU rendering allocates nothing");
                check(!visibility.update({map, movement}), "Idle blockout view stays cached");
                if (i == 0 && t == 0)
                    rejects(
                        [&] { world_render::prepare(map, {}, {}, {1, 65536, 64 * 1024 * 1024}, &assets); },
                        "Scene capacity failure rejects blockout preparation");
            }
    }
    check(!assets.residency().meshes, "Blockout assets release after lease lifetime");
    std::cout
        << "Owned blockout publication, 36 kit orientations, bounds, lifetimes and CPU allocations passed\n";
    return 0;
} catch (const std::exception& e) {
    watching = false;
    std::cerr << e.what() << '\n';
    return 1;
}

#include "render/world_visibility.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/world_fixtures.hpp"
#include <iostream>
#include <limits>
using namespace blobber;
namespace {
world_assets::Publication fixture(const std::filesystem::path& path) {
    world_assets::Writer writer(path);
    std::vector<unsigned char> pixels(8 * 8 * 4, 255);
    world_assets::Pack pack;
    pack.id = "underkeep";
    pack.materialCount = 4;
    for (unsigned i = 0; i < 3; ++i)
        pack.materials[i] = std::uint16_t(writer.add("material" + std::to_string(i), 8, 8, false, pixels));
    const auto door = writer.add("door", 8, 8, false, pixels, 0, {}, false);
    pack.door = pack.lockedDoor = pack.doorEdge = pack.sky = std::uint16_t(door);
    for (unsigned y = 0; y < 8; ++y)
        for (unsigned x = 0; x < 8; ++x)
            if (x > 1 && x < 6 && y > 1 && y < 6)
                pixels[(y * 8 + x) * 4 + 3] = 0;
    const auto decal = writer.add("cutout", 8, 8, true, pixels, 16, {}, false);
    pack.crest = pack.vine = std::uint16_t(decal);
    pack.decorations.fill(std::uint16_t(decal));
    // Own padded moss: visible rows 2..5, not the full transparent canvas.
    for (unsigned y = 0; y < 8; ++y)
        for (unsigned x = 0; x < 8; ++x)
            pixels[(y * 8 + x) * 4 + 3] = y >= 2 && y < 6 ? 255 : 0;
    const auto moss = writer.add("moss", 8, 8, true, pixels, 16, {}, false);
    pack.decorations[unsigned(DecorationKind::DampA)] = std::uint16_t(moss);
    pack.decorations[unsigned(DecorationKind::DampB)] = std::uint16_t(moss);
    for (const auto* name : {"loot/bag", "fixture/key-bronze", "fixture/key-silver", "fixture/upstairs",
                             "fixture/downstairs", "fixture/endpoint", "fixture/threshold"})
        writer.add(name, 8, 8, true, pixels);
    std::fill(pixels.begin(), pixels.end(), 255);
    pack.materials[3] = std::uint16_t(writer.add("atlas", 8, 8, false, pixels));
    std::vector<world_assets::PropMesh> meshes;
    for (unsigned i = 0; i < 2; ++i) {
        world_assets::PropMesh mesh;
        mesh.name = i ? "open" : "closed";
        mesh.texture = pack.materials[3];
        const float h = i ? .7f : .3f;
        mesh.vertices = {{{-.3f, 0, .2f, 0, 0, 1, 0, 1}}, {{.3f, 0, .2f, 0, 0, 1, 1, 1}},
                         {{.3f, h, .2f, 0, 0, 1, 1, 0}},  {{-.3f, 0, .2f, 0, 0, 1, 0, 1}},
                         {{.3f, h, .2f, 0, 0, 1, 1, 0}},  {{-.3f, h, .2f, 0, 0, 1, 0, 0}}};
        meshes.push_back(mesh);
    }
    pack.models = {0, 1, 0, 0};
    writer.finish({}, 0, {pack}, meshes);
    return world_assets::load(path);
}
unsigned chestPixels(const world_render::WorldFrame& frame) {
    unsigned count = 0;
    for (int y = 0; y < viewHeight; ++y)
        for (int x = 0; x < viewWidth; ++x)
            count += bool(frame.picking.pickContainer(x, y));
    return count;
}
} // namespace
int main() {
    try {
        test::WorldFixture storage;
        auto publication = fixture(storage.directory);
        check(publication.meshes.size() == 2 && publication.packs[0].models[1] == 1,
              "Prepared mesh bindings round trip");
        world_assets::CpuAssets assets(publication);
        const auto assetsLease = assets.acquire(world_assets::allAssets(assets.publication));
        WorldDefinition world;
        world.assetPack = "underkeep";
        Decoration chest{{4, 3}, Heading::North, DecorationKind::Chest, 0, .5f, 0, .76f, .36f, .58f};
        chest.container = 0;
        world.decorations.push_back(chest);
        world.decorations.push_back(
            {{7, 1}, Heading::North, DecorationKind::WebA, 0, .5f, .82f, .7f, .34f, .01f});
        world.decorations.push_back(
            {{3, 1}, Heading::North, DecorationKind::DampA, 0, .5f, .25f, .75f, .45f, .01f});
        world.decorations.push_back(
            {{4, 1}, Heading::North, DecorationKind::DampB, 0, .5f, .25f, .75f, .45f, .01f});
        const std::vector<std::string> rows{"#########", "#.......#", "#.......#", "#.......#", "#.......#",
                                            "#.......#", "#.......#", "#.......#", "#########"};
        const Map map(rows, world);
        auto floating = world;
        floating.decorations[1].cell = {4, 1};
        rejects([&] { Map invalid(rows, floating); }, "Web must attach to both corner walls");
        auto scene = world_render::prepare(map, {}, {}, {}, &assets);
        unsigned variants = 0;
        for (const auto& r : scene.ranges)
            if (r.variant != noContainer) {
                check(r.painted, "Props bypass face shading");
                ++variants;
            }
        check(variants == 2, "Both chest variants prepared once");
        check(scene.decals.size() == 3 &&
                  scene.decals[0].vertices[0].position.x != scene.decals[0].vertices[1].position.x &&
                  scene.decals[0].vertices[0].position.z != scene.decals[0].vertices[1].position.z,
              "Web spans corner, not camera-facing");
        for (unsigned i : {1u, 2u}) {
            const auto& v = scene.decals[i].vertices;
            const float inkBottom = v[3].position.y - .75f * (v[3].position.y - v[0].position.y);
            check(std::abs(inkBottom) < 1e-5f, "Padded moss visible ink meets the floor");
        }
        for (const auto& r : scene.ranges)
            if (r.variant != noContainer)
                for (unsigned i = r.first; i < r.first + r.count; ++i) {
                    const auto p = scene.vertices[scene.indices[i]].position;
                    check(std::abs(p.x - 4.5f) <= .15001f && p.y >= 0 &&
                              p.y <= (r.variant == 1 ? .35001f : .15001f) && p.z >= 3.0124f && p.z < 3.2f,
                          "Both half-size chest states share a floor anchor tucked toward the wall");
                }
        auto raisedWorld = world;
        raisedWorld.surfaces.resize(rows.size() * rows[0].size());
        for (auto& surface : raisedWorld.surfaces) {
            surface.floor = 1;
            surface.ceiling = 5;
        }
        const Map raised(rows, raisedWorld);
        const auto raisedScene = world_render::prepare(raised, {}, {}, {}, &assets);
        for (unsigned i : {1u, 2u}) {
            const auto& v = raisedScene.decals[i].vertices;
            check(std::abs(v[3].position.y - .75f * (v[3].position.y - v[0].position.y) - .25f) < 1e-5f,
                  "Moss follows raised local floor");
        }
        world_render::VisibilityPipeline pipeline(scene, assets);
        MovementState movement;
        movement.player = {{4, 5}, Heading::North};
        LootSnapshot loot;
        loot.world = 42;
        loot.containerCount = 1;
        loot.containers[0].cell = {4, 3};
        const world_render::FrameInput input{map, movement, nullptr, nullptr, &loot};
        check(pipeline.update(input), "Closed prop view prepared");
        const auto closed = chestPixels(pipeline.frame());
        check(closed > 10 && pipeline.containerVisible({42, 0}) && !pipeline.containerVisible({43, 0}),
              "Checked container visibility");
        watching = true;
        allocations = 0;
        loot.containers[0].opened = true;
        ++loot.revision;
        const bool changed = pipeline.update(input);
        watching = false;
        check(changed && allocations == 0, "Opening changes prepared view without C++ allocations");
        check(chestPixels(pipeline.frame()) > closed, "CPU picking switches to open geometry");
        watching = true;
        allocations = 0;
        const bool idle = pipeline.update(input);
        watching = false;
        check(!idle && allocations == 0, "Idle decorated view stays cached without C++ allocations");
        auto decoded = publication;
        for (unsigned i = 0; i < decoded.meshes.size(); ++i)
            decoded.meshes[i] = publication.readMesh(i);
        auto corrupt = decoded;
        corrupt.meshes[0].vertices[0][6] = std::numeric_limits<float>::quiet_NaN();
        rejects([&] { world_assets::CpuAssets invalid(corrupt); }, "Invalid mesh UV rejected before use");
        corrupt = decoded;
        for (auto& v : corrupt.meshes[0].vertices)
            v[0] += .6f;
        rejects([&] { world_assets::CpuAssets invalid(corrupt); },
                "Model must fit its reserved movement footprint");
        corrupt = decoded;
        corrupt.meshes[0].vertices.resize(6003);
        rejects([&] { world_assets::CpuAssets invalid(corrupt); }, "Over-cap mesh rejected");
        rejects([&] { world_render::prepare(map, {}, {}, {1, 65536, 64 * 1024 * 1024}, &assets); },
                "Decorated scene obeys geometry budget");
        std::cout << "Underkeep prepared meshes, picking and allocation checks passed\n";
        return 0;
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}

#pragma once
#include "support/world_fixtures.hpp"
namespace blobber::test {
struct ArchitectureFixture {
    WorldFixture storage;
    ArchitectureFixture() {
        world_assets::Writer writer(storage.directory);
        std::vector<unsigned char> rgba(4 * 4 * 4, 255);
        const auto opaque = std::uint16_t(writer.add("door", 4, 4, false, rgba, 0, {}, false));
        const auto cutout = std::uint16_t(writer.add("cutout", 4, 4, true, rgba, 0, {}, false));
        const auto stone = std::uint16_t(writer.add("stone", 4, 4, false, rgba));
        rgba[0] = 80;
        rgba[1] = 120;
        const auto roof = std::uint16_t(writer.add("roof", 4, 4, false, rgba));
        for (const char* name :
             {"loot/bag", "fixture/key-bronze", "fixture/key-silver", "fixture/endpoint", "fixture/threshold",
              "effect/arrow", "effect/tip", "effect/feathers", "effect/slash", "effect/glint",
              "effect/grease", "effect/web", "effect/weapon", "effect/preview", "effect/white"})
            writer.add(name, 4, 4, true, rgba);
        world_assets::Pack pack;
        pack.id = "architecture";
        pack.materialCount = 3;
        pack.materials[0] = stone;
        pack.materials[1] = roof;
        pack.materials[2] = stone;
        pack.door = pack.lockedDoor = pack.doorEdge = pack.sky = opaque;
        pack.crest = pack.vine = cutout;
        std::vector<world_assets::PropMesh> meshes(2);
        for (unsigned i = 0; i < 2; ++i) {
            auto& mesh = meshes[i];
            mesh.name = i ? "roof" : "shell";
            mesh.texture = i ? roof : stone;
            mesh.architecture = true;
        }
        // Owned closed shell with a set-back front face, tall roof and overhang.
        auto quad = [&](unsigned part, std::array<std::array<float, 3>, 4> p, std::array<float, 3> n) {
            for (unsigned i : {0u, 1u, 2u, 0u, 2u, 3u})
                meshes[part].vertices.push_back({p[i][0], p[i][1], p[i][2], n[0], n[1], n[2],
                                                 i == 1 || i == 2 ? 1.f : 0.f, i >= 2 ? 1.f : 0.f});
        };
        quad(0, {{{-.5f, 0, -.3f}, {-.5f, 2, -.3f}, {.5f, 2, -.3f}, {.5f, 0, -.3f}}}, {0, 0, -1});
        quad(0, {{{.5f, 0, .5f}, {.5f, 2, .5f}, {-.5f, 2, .5f}, {-.5f, 0, .5f}}}, {0, 0, 1});
        quad(0, {{{-.5f, 0, .5f}, {-.5f, 2, .5f}, {-.5f, 2, -.3f}, {-.5f, 0, -.3f}}}, {-1, 0, 0});
        quad(0, {{{.5f, 0, -.3f}, {.5f, 2, -.3f}, {.5f, 2, .5f}, {.5f, 0, .5f}}}, {1, 0, 0});
        quad(1, {{{-1, 2.1f, -1}, {-1, 2.1f, 1}, {1, 2.1f, 1}, {1, 2.1f, -1}}}, {0, 1, 0});
        // Underside matters when looking up from the street.
        quad(1, {{{-1, 2, -1}, {1, 2, -1}, {1, 2, 1}, {-1, 2, 1}}}, {0, -1, 0});
        pack.buildings.push_back({"house", {0, 1}});
        world_assets::Appearance actor;
        actor.id = "goblin";
        const auto sprite = std::uint16_t(writer.add("actor", 4, 4, true, rgba));
        actor.clips[0].timing = {{{100, 0, 0, 0}}, 1};
        for (auto& side : actor.clips[0].textures)
            side[0] = sprite;
        writer.finish({actor}, 0, {pack}, meshes);
    }
    world_assets::Publication publication() const { return world_assets::load(storage.directory); }
};
inline std::vector<std::string> architectureRows(int size = 12, Cell anchor = {7, 7}) {
    std::vector<std::string> rows(size, std::string(size, '.'));
    for (int i = 0; i < size; ++i)
        rows[0][i] = rows[size - 1][i] = rows[i][0] = rows[i][size - 1] = '#';
    rows[anchor.y][anchor.x] = '#';
    return rows;
}
inline WorldDefinition architectureWorld(int size = 12, Cell anchor = {7, 7}) {
    WorldDefinition w;
    w.assetPack = "architecture";
    w.surfaces.resize(std::size_t(size) * size);
    for (auto& s : w.surfaces) {
        s.ceiling = noCeiling;
        s.environment = Environment::Daylight;
    }
    w.buildings.push_back({"house", anchor, {0, 0}, Heading::North, 1, 1});
    return w;
}
// Five-cell gate joined to generated stone boundary piers; no production art.
inline world_assets::Publication gatePublication(world_assets::Publication publication) {
    for (unsigned part = 0; part < publication.meshes.size(); ++part) {
        publication.meshes[part] = publication.readMesh(part);
        for (auto& v : publication.meshes[part].vertices) {
            v[0] = v[0] < 0 ? -2.5f : 2.5f;
            v[2] = v[2] < 0 ? (part ? -.5f : -.3f) : 1.5f;
            v[1] = v[1] == 0 ? 0 : v[1] > 2 ? 2.9f : 2.8f;
        }
    }
    return publication;
}
inline WorldDefinition gateWorld() {
    auto world = architectureWorld();
    world.buildings[0].footprint = {-2, 0};
    world.buildings[0].width = 5;
    world.buildings[0].depth = 2;
    return world;
}
inline std::vector<std::string> gateRows() {
    auto rows = architectureRows();
    for (int y = 7; y <= 8; ++y)
        for (int x = 4; x <= 10; ++x)
            rows[y][x] = '#';
    return rows;
}
} // namespace blobber::test

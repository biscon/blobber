#pragma once
#include "assets/world_assets.hpp"
namespace blobber::test {
// Tiny owned geometry for integration/allocation fixtures. No production art dependency.
inline void finishWithUnderkeep(world_assets::Writer& writer,
                                const std::vector<world_assets::Appearance>& actors,
                                std::uint64_t definitions, unsigned door, unsigned locked, unsigned crest,
                                unsigned vine) {
    world_assets::Pack dungeon;
    dungeon.materialCount = 3;
    dungeon.materials[0] = 0;
    dungeon.materials[1] = 1;
    dungeon.materials[2] = 2;
    dungeon.door = dungeon.doorEdge = dungeon.sky = std::uint16_t(door);
    dungeon.lockedDoor = std::uint16_t(locked);
    dungeon.crest = std::uint16_t(crest);
    dungeon.vine = std::uint16_t(vine);
    auto underkeep = dungeon;
    underkeep.id = "underkeep";
    underkeep.decorations.fill(std::uint16_t(crest));
    underkeep.models.fill(0);
    world_assets::PropMesh mesh;
    mesh.name = "owned-box";
    mesh.texture = 0;
    constexpr float corners[8][3]{{-.3f, 0, -.25f},   {.3f, 0, -.25f},   {.3f, 0, .25f},   {-.3f, 0, .25f},
                                  {-.3f, .3f, -.25f}, {.3f, .3f, -.25f}, {.3f, .3f, .25f}, {-.3f, .3f, .25f}};
    constexpr unsigned faces[6][4]{{3, 2, 1, 0}, {0, 1, 5, 4}, {1, 2, 6, 5},
                                   {2, 3, 7, 6}, {3, 0, 4, 7}, {4, 5, 6, 7}};
    constexpr float normals[6][3]{{0, -1, 0}, {0, 0, -1}, {1, 0, 0}, {0, 0, 1}, {-1, 0, 0}, {0, 1, 0}};
    constexpr float uv[4][2]{{0, 1}, {1, 1}, {1, 0}, {0, 0}};
    for (unsigned f = 0; f < 6; ++f)
        for (unsigned k : {0u, 2u, 1u, 0u, 3u, 2u}) {
            const auto& p = corners[faces[f][k]];
            const auto& n = normals[f];
            mesh.vertices.push_back({p[0], p[1], p[2], n[0], n[1], n[2], uv[k][0], uv[k][1]});
        }
    auto city = dungeon;
    city.id = "sanctuary-city";
    auto shell = mesh;
    shell.name = "owned-city-shell";
    shell.architecture = true;
    for (const char* name :
         {"sanctuary", "dungeon-entrance", "inn", "provisioner", "smithy", "low-house", "low-house-hip-ivy",
          "tall-house-ivy", "low-house-hip", "tall-house", "city-gate", "boundary-wall", "courtyard-garden",
          "lane-garden", "east-garden", "market-produce", "market-grain", "fountain"})
        city.buildings.push_back({name, {1}});
    writer.finish(actors, definitions, {dungeon, underkeep, city}, {mesh, shell});
}
} // namespace blobber::test

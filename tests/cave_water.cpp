#include "../tools/cave_review.hpp"
#include "support/check.hpp"
#include <algorithm>
#include <cmath>

using namespace blobber;
// A generated painting has smooth ramps across each far edge and a fine interior ripple.
void checkCaveWater(const generation::CaveFloor& cave) {
    world_assets::Publication art;
    art.textures.emplace_back();
    art.packs.emplace_back();
    auto& pack = art.packs.front();
    pack.id = "underkeep";
    pack.buildings.push_back({"grain-sack", {0}});
    world_assets::PropMesh bag;
    bag.name = "test-sack";
    bag.architecture = true;
    bag.vertices = {{{0, 0, 0, 0, 1, 0, 0, 0}}, {{0, 0, 1, 0, 1, 0, 0, 1}}, {{1, 0, 1, 0, 1, 0, 1, 1}}};
    art.meshes.push_back(bag);
    constexpr unsigned extent = cave_review::waterSourceSize;
    constexpr unsigned side = world_assets::WaterCurrent::tileSize;
    std::vector<unsigned char> source(extent * extent * 4, 255);
    for (unsigned y = 0; y < extent; ++y)
        for (unsigned x = 0; x < extent; ++x) {
            auto* pixel = source.data() + (y * extent + x) * 4;
            pixel[0] = x / 2;
            pixel[1] = y / 2;
            pixel[2] = x % 2 ? 120 : 130;
        }
    cave_review::bindBlockouts(art, source);
    const auto& preview = art.packs.back();
    const auto material = art.current.texture;
    check(material < art.textures.size() && art.textures[material].width == 256 &&
              art.textures[material].height == 256 && !art.textures[material].clamp,
          "Pool needs one repeating wall-density tile per cell");
    const auto pixels = art.read(material);
    check(pixels.size() == world_assets::mipBytes(side, side), "Missing current mip chain");
    auto sample = [&](unsigned x, unsigned y, unsigned c) { return pixels[(y * side + x) * 4 + c]; };
    for (unsigned i = 0; i < side; ++i) {
        check(std::abs(sample(0, i, 0) - sample(side - 1, i, 0)) <= 1 &&
                  std::abs(sample(i, 0, 1) - sample(i, side - 1, 1)) <= 1,
              "Current texture has a jump at its repeat boundary");
    }
    check(sample(140, 100, 2) == 130 && sample(141, 100, 2) == 120,
          "Native-resolution detail lost in water preparation");
    unsigned occupied = 0;
    for (const auto& s : cave.map.definition().scenery) {
        if (!s.blocking)
            continue;
        occupied += s.width * s.depth;
        const auto& model = preview.buildings.at(preview.building(s.model).index);
        for (const auto part : model.parts) {
            const auto& mesh = art.meshes.at(part);
            check(mesh.texture == material && !mesh.water, "Pool strip lost the shared current material");
        }
    }
    check(occupied == 12, "Water coverage differs from collision");
}

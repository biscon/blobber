#include "assets/current_tile.hpp"
#include "support/check.hpp"
#include "support/world_fixtures.hpp"
#include <algorithm>
#include <cmath>

using namespace blobber;
// A generated painting has smooth ramps across each far edge and a fine interior ripple.
void checkCaveWater() {
    constexpr unsigned extent = world_assets::waterSourceSize;
    constexpr unsigned side = world_assets::WaterCurrent::tileSize;
    std::vector<unsigned char> source(extent * extent * 4, 255);
    for (unsigned y = 0; y < extent; ++y)
        for (unsigned x = 0; x < extent; ++x) {
            auto* pixel = source.data() + (y * extent + x) * 4;
            pixel[0] = x / 2;
            pixel[1] = y / 2;
            pixel[2] = x % 2 ? 120 : 130;
        }
    test::WorldFixture fixture;
    world_assets::Writer writer(fixture.directory);
    const auto tile = world_assets::prepareCurrentTile(source);
    const auto id = writer.add("current", side, side, false, tile);
    writer.markCurrent(id);
    writer.finish({});
    auto art = world_assets::load(fixture.directory);
    check(art.textures.at(id).current, "Current binding lost on disk");
    const auto pixels = art.read(id);
    check(pixels.size() == world_assets::mipBytes(side, side), "Missing current mip chain");
    auto sample = [&](unsigned x, unsigned y, unsigned c) { return pixels[(y * side + x) * 4 + c]; };
    for (unsigned i = 0; i < side; ++i) {
        check(std::abs(sample(0, i, 0) - sample(side - 1, i, 0)) <= 1 &&
                  std::abs(sample(i, 0, 1) - sample(i, side - 1, 1)) <= 1,
              "Current texture has a jump at its repeat boundary");
    }
    check(sample(140, 100, 2) == 130 && sample(141, 100, 2) == 120,
          "Native-resolution detail lost in water preparation");
    // Tamper with an otherwise valid current descriptor. Filtering must never be applied
    // to an unrelated sprite or material size.
    world_assets::Writer invalid(fixture.directory);
    const auto tiny = invalid.add("wrong-size", 1, 1, false, {20, 30, 40, 255});
    bool rejected = false;
    try {
        invalid.markCurrent(tiny);
        invalid.finish({});
        (void)world_assets::load(fixture.directory);
    } catch (const std::exception&) {
        rejected = true;
    }
    check(rejected, "Invalid current tile size was admitted");
}

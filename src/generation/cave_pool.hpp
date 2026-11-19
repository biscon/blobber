#pragma once
#include "world/grid.hpp"
namespace blobber::generation {
// Four rectangles cover the 5x3 basin around the solid 3x1 rib. Every
// coordinate refers to the same basin center for mesh placement and collision.
struct PoolStrip {
    const char* model;
    Cell center;
    unsigned width;
};
constexpr std::array<PoolStrip, 4> cavePool{{{"pool-north", {0, -1}, 5},
                                             {"pool-south", {0, 1}, 5},
                                             {"pool-west", {-2, 0}, 1},
                                             {"pool-east", {2, 0}, 1}}};
} // namespace blobber::generation

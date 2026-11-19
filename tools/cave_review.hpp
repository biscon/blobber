#pragma once
#include "assets/world_assets.hpp"
#include "generation/caves.hpp"
#include <iosfwd>

namespace blobber {
class WorldRenderer;
}
namespace blobber::cave_review {
void biomeChange(WorldRenderer&, const generation::CaveFloor&, const std::filesystem::path&,
                 std::uint32_t seed);
void checkPreparedDressing(const world_assets::CpuAssets&, const generation::CaveFloor&);
void writePlan(std::ostream&, const generation::CaveFloor&, std::uint32_t seed, bool fallback);
} // namespace blobber::cave_review

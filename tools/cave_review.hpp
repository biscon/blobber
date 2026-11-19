#pragma once
#include "assets/world_assets.hpp"
#include "generation/caves.hpp"
#include <iosfwd>

namespace blobber::cave_review {
constexpr unsigned waterOverlap = 32;
constexpr unsigned waterSourceSize = world_assets::WaterCurrent::tileSize + waterOverlap;
void bindBlockouts(world_assets::Publication&, const std::vector<unsigned char>& waterRgba);
void writePlan(std::ostream&, const generation::CaveFloor&, std::uint32_t seed, bool fallback);
} // namespace blobber::cave_review

#pragma once
#include "assets/world_assets.hpp"

namespace blobber::world_assets {
constexpr unsigned waterOverlap = 32;
constexpr unsigned waterSourceSize = WaterCurrent::tileSize + waterOverlap;
// Offline RGBA authoring input. Output is one repeatable native-resolution tile.
std::vector<unsigned char> prepareCurrentTile(const std::vector<unsigned char>& source);
} // namespace blobber::world_assets

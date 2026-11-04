#pragma once
#include "assets/image.hpp"
namespace blobber {
// 4x2 source order matches MapMarker (excluding None). Startup-only preparation.
inline constexpr int mapIconColumns = 4, mapIconRows = 2, mapIconCount = 8;
inline constexpr int mapIconSize = 48, mapIconPadding = 2, mapIconStride = 52;
inline constexpr int mapIconAtlasWidth = mapIconColumns * mapIconStride;
inline constexpr int mapIconAtlasHeight = mapIconRows * mapIconStride;
OwnedImage prepareMapIconAtlas(const Image& rgbaSheet);
} // namespace blobber

#pragma once
#include "rules/creation.hpp"
namespace blobber {
struct PortraitRegion {
    int x, y, width, height;
};
constexpr int portraitAtlasWidth = 2048, smallPortraitHeight = 1024, largePortraitHeight = 4096;
constexpr std::size_t portraitResidentBytes =
    std::size_t(portraitAtlasWidth) * (smallPortraitHeight + largePortraitHeight) * 4;
constexpr std::size_t portraitWorkspaceLimit = 96u * 1024 * 1024;
constexpr PortraitRegion portraitRegion(unsigned i, bool large) noexcept {
    return large ? PortraitRegion{int(i % 4) * 436 + 2, int(i / 4) * 652 + 2, 432, 648}
                 : PortraitRegion{int(i % 9) * 220 + 2, int(i / 9) * 256 + 2, 216, 252};
}
} // namespace blobber

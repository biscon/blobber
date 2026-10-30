#pragma once
#include "probe/scene.hpp"
#include "render/npc_sprites.hpp"
#include <filesystem>
namespace blobber::probe {
struct Pixels {
    int width = 0, height = 0;
    std::vector<Color> colors;
    Image image() const noexcept {
        return {const_cast<Color*>(colors.data()), width, height, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    }
};
struct Art {
    std::array<Pixels, 6> surfaces; // wall, floor, ceiling, door, banner, sky
    NpcAppearance npc;
    std::uint64_t fingerprint = 1469598103934665603ull;
    std::size_t bytes = 0;
};
Art loadArt(bool shipped, const std::filesystem::path& root, const Scene&);
Pixels premultiplied(const NpcImage&, unsigned char cutoff);
} // namespace blobber::probe

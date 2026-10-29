#include "assets/creator_art.hpp"
#include "assets/image.hpp"
#include <algorithm>
#include <fstream>
#include <stdexcept>
namespace blobber {
void prepareCreatorArt(const std::filesystem::path& source, const std::filesystem::path& output) {
    static_assert(creatorArtNames.size() <= 36);
    // Atlas + worst-case QOI encoder; masters are released before export.
    static_assert(std::size_t(creatorAtlasSize) * creatorAtlasSize * 9 + 22 < creatorWorkspaceLimit);
    std::filesystem::create_directories(output);
    OwnedImage atlas(GenImageColor(creatorAtlasSize, creatorAtlasSize, BLANK));
    if (!atlas.value.data)
        throw std::runtime_error("Cannot allocate creator atlas");
    for (unsigned i = 0; i < creatorArtNames.size(); ++i) {
        const auto path = source / (std::string(creatorArtNames[i]) + ".png");
        if (std::filesystem::file_size(path) > 8u * 1024 * 1024)
            throw std::runtime_error("Creator master exceeds 8 MiB");
        std::ifstream input(path, std::ios::binary);
        std::array<unsigned char, 26> bytes{};
        input.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
        auto u32 = [&](unsigned n) {
            return (std::uint32_t(bytes[n]) << 24) | (std::uint32_t(bytes[n + 1]) << 16) |
                   (std::uint32_t(bytes[n + 2]) << 8) | bytes[n + 3];
        };
        if (!input || u32(0) != 0x89504e47 || u32(4) != 0x0d0a1a0a || u32(8) != 13 || u32(12) != 0x49484452 ||
            u32(16) < 324 || u32(20) < 324 || u32(16) > 2048 || u32(20) > 2048 || bytes[24] != 8 ||
            (bytes[25] != 2 && bytes[25] != 6))
            throw std::runtime_error("Invalid bounded creator PNG: " + path.string());
        OwnedImage image(LoadImage(path.string().c_str()));
        if (!image.value.data)
            throw std::runtime_error("Cannot load creator art");
        const int side = std::min(image.value.width, image.value.height);
        ImageCrop(&image.value, {float((image.value.width - side) / 2),
                                 float((image.value.height - side) / 2), float(side), float(side)});
        ImageResize(&image.value, creatorTileSize, creatorTileSize);
        if (!image.value.data)
            throw std::runtime_error("Cannot prepare creator art");
        const int x = 2 + int(i % 6) * creatorTileStride, y = 2 + int(i / 6) * creatorTileStride;
        ImageDrawImage(&atlas.value, image.value, x, y, WHITE);
        for (int py = -2; py < creatorTileSize + 2; ++py)
            for (int px = -2; px < creatorTileSize + 2; ++px)
                if (px < 0 || py < 0 || px >= creatorTileSize || py >= creatorTileSize)
                    ImageDrawPixel(&atlas.value, x + px, y + py,
                                   GetImageColor(image.value, std::clamp(px, 0, creatorTileSize - 1),
                                                 std::clamp(py, 0, creatorTileSize - 1)));
    }
    if (!ExportImage(atlas.value, (output / "creator.qoi").string().c_str()))
        throw std::runtime_error("Cannot export creator atlas");
}
} // namespace blobber

#include "assets/portrait_preparation.hpp"
#include "assets/image.hpp"
#include <algorithm>
#include <fstream>
#include <stdexcept>
namespace blobber {
void preparePortraits(const std::filesystem::path& source, const std::filesystem::path& output) {
    std::filesystem::create_directories(output);
    SetTraceLogLevel(LOG_WARNING);
    std::ofstream manifest(output / "portraits.tsv");
    if (!manifest)
        throw std::runtime_error("Cannot write portrait manifest");
    manifest << "version\tid\trace\tfile\tsmall_x\tsmall_y\tlarge_x\tlarge_y\tcrop_center_x\tcrop_center_y\n";
    // QOI encoder uses at most five bytes/pixel plus 22 bytes. One 32 MiB
    // atlas plus that 40 MiB buffer fits the approved 96 MiB phase budget.
    static_assert(std::size_t(portraitAtlasWidth) * largePortraitHeight * 9 + 22 < portraitWorkspaceLimit);
    for (bool big : {false, true}) {
        OwnedImage prepared(
            GenImageColor(portraitAtlasWidth, big ? largePortraitHeight : smallPortraitHeight, BLANK));
        if (!prepared.value.data)
            throw std::runtime_error("Cannot allocate portrait atlas");
        auto& atlas = prepared.value;
        for (std::uint8_t i = 0; i < rules::portraitCount; ++i) {
            const auto& p = *rules::portrait({i});
            const auto path = source / p.file;
            // Decode only bounded production masters. Source files are preparation inputs, not frame assets.
            if (std::filesystem::file_size(path) > 8u * 1024 * 1024)
                throw std::runtime_error("Portrait master exceeds 8 MiB file limit");
            std::ifstream header(path, std::ios::binary);
            std::array<unsigned char, 26> bytes{};
            header.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
            const unsigned char signature[]{137, 80, 78, 71, 13, 10, 26, 10};
            auto dimension = [&](int i) {
                return (std::uint32_t(bytes[i]) << 24) | (std::uint32_t(bytes[i + 1]) << 16) |
                       (std::uint32_t(bytes[i + 2]) << 8) | bytes[i + 3];
            };
            if (!header || !std::equal(std::begin(signature), std::end(signature), bytes.begin()) ||
                dimension(8) != 13 || dimension(12) != 0x49484452 || dimension(16) < 432 ||
                dimension(16) > 2048 || dimension(20) < 648 || dimension(20) > 2048 || bytes[24] != 8 ||
                (bytes[25] != 2 && bytes[25] != 6))
                throw std::runtime_error("Invalid or oversized portrait PNG header: " + path.string());
            OwnedImage master(LoadImage(path.string().c_str()));
            if (!master.value.data || master.value.width < 432 || master.value.height < 648 ||
                master.value.width > 2048 || master.value.height > 2048)
                throw std::runtime_error("Portrait master dimensions outside 432x648..2048x2048: " +
                                         path.string());
            {
                const auto r = portraitRegion(i, big);
                const float ratio = float(r.width) / r.height;
                Rectangle crop{0, 0, float(master.value.width), float(master.value.height)};
                if (crop.width / crop.height > ratio) {
                    const auto w = crop.height * ratio;
                    crop.x = (crop.width - w) * .5f;
                    crop.width = w;
                } else {
                    const auto h = crop.width / ratio;
                    crop.y = (crop.height - h) * .5f;
                    crop.height = h;
                }
                OwnedImage image(ImageFromImage(master.value, crop));
                if (!image.value.data)
                    throw std::runtime_error("Cannot allocate portrait crop");
                ImageResize(&image.value, r.width, r.height);
                if (!image.value.data || image.value.width != r.width || image.value.height != r.height)
                    throw std::runtime_error("Cannot resize portrait crop");
                ImageDrawImage(&atlas, image.value, r.x, r.y, WHITE);
                for (int y = -2; y < r.height + 2; ++y)
                    for (int x = -2; x < r.width + 2; ++x)
                        if (x < 0 || x >= r.width || y < 0 || y >= r.height)
                            ImageDrawPixel(&atlas, r.x + x, r.y + y,
                                           GetImageColor(image.value, std::clamp(x, 0, r.width - 1),
                                                         std::clamp(y, 0, r.height - 1)));
            }
            const auto a = portraitRegion(i, false), b = portraitRegion(i, true);
            if (!big)
                manifest << 1 << '\t' << p.id << '\t' << p.race << '\t' << p.file << '\t' << a.x << '\t'
                         << a.y << '\t' << b.x << '\t' << b.y << "\t0.5\t0.5\n";
        }
        if (!ExportImage(atlas,
                         (output / (big ? "portraits-large.qoi" : "portraits-small.qoi")).string().c_str()))
            throw std::runtime_error("Cannot export portrait atlas");
    }
    manifest.flush();
    if (!manifest)
        throw std::runtime_error("Cannot flush portrait manifest");
}
} // namespace blobber

#include "assets/prepared_loot.hpp"
#include "assets/image.hpp"
#include "assets/prepared_binary.hpp"
namespace blobber {
PreparedLoot prepareLoot(Image sheet, const std::array<Image, PreparedLoot::icons - 1>& fixtures) {
    if (!sheet.data || sheet.width < 9 || sheet.height < 9 || sheet.width > 4096 || sheet.height > 4096)
        throw std::invalid_argument("Invalid loot bag image");
    PreparedLoot out;
    out.pixels.reserve(PreparedLoot::icons * PreparedLoot::pixelsPerIcon);
    for (int icon = 0; icon < PreparedLoot::icons; ++icon)
        for (int size = PreparedLoot::minSize; size <= PreparedLoot::maxSize; ++size) {
            OwnedImage part(ImageCopy(icon && fixtures[icon - 1].data ? fixtures[icon - 1] : sheet));
            ImageResize(&part.value, size, size);
            ImageFormat(&part.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            const auto* p = static_cast<const Color*>(part.value.data);
            out.pixels.insert(out.pixels.end(), p, p + size * size);
        }
    return out;
}
void savePreparedLoot(const PreparedLoot& loot, const std::filesystem::path& path) {
    if (loot.pixels.size() != PreparedLoot::icons * PreparedLoot::pixelsPerIcon)
        throw std::invalid_argument("Incomplete prepared loot");
    prepared_io::Writer out(path, "BLBLOOT2", 1, 0);
    out.bytes(loot.pixels.data(), loot.pixels.size() * sizeof(Color));
    out.finish();
}
PreparedLoot loadPreparedLoot(const std::filesystem::path& path) {
    prepared_io::Reader in(path, "BLBLOOT2", 1, 0, {4 * 1024 * 1024, 4 * 1024 * 1024});
    PreparedLoot loot;
    constexpr auto count = PreparedLoot::icons * PreparedLoot::pixelsPerIcon;
    in.claim(count * sizeof(Color));
    loot.pixels.resize(count);
    in.bytes(loot.pixels.data(), count * sizeof(Color));
    in.finish();
    return loot;
}
} // namespace blobber

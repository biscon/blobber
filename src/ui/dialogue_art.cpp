#include "assets/prepared_image.hpp"
#include "ui/art.hpp"
#include <fstream>
#include <stdexcept>

namespace blobber {
void UiArt::prepareDialogue(const std::vector<std::string>& files) {
    if (files.size() > dialogueOverlayCapacity)
        throw std::runtime_error("Dialogue exceeds 16 overlay images");
    // Stage leases transactionally; failure releases every newly acquired texture.
    std::array<GpuLease<TextureHandle>, dialogueOverlayCapacity> staged;
    std::size_t bytes = 0;
    for (unsigned i = 0; i < files.size(); ++i) {
        try {
            const std::filesystem::path path(files[i]);
            if (path.extension() != ".png" || std::filesystem::file_size(path) > 32 * 1024 * 1024)
                throw std::runtime_error("Overlay requires a PNG of at most 32 MiB");
            // Validate dimensions before decoding; malformed/oversized input cannot
            // force an unbounded decoded allocation before the preparation budget check.
            std::ifstream input(path, std::ios::binary);
            std::array<unsigned char, 24> header{};
            input.read(reinterpret_cast<char*>(header.data()), header.size());
            constexpr unsigned char signature[]{137, 80, 78, 71, 13, 10, 26, 10};
            if (!input || !std::equal(std::begin(signature), std::end(signature), header.begin()) ||
                header[8] || header[9] || header[10] || header[11] != 13 || header[12] != 'I' ||
                header[13] != 'H' || header[14] != 'D' || header[15] != 'R')
                throw std::runtime_error("Invalid overlay PNG header");
            const auto dimension = [&](unsigned at) {
                return (std::uint32_t(header[at]) << 24) | (std::uint32_t(header[at + 1]) << 16) |
                       (std::uint32_t(header[at + 2]) << 8) | header[at + 3];
            };
            const auto w = dimension(16), h = dimension(20);
            if (!w || !h || w > 2048 || h > 2048)
                throw std::runtime_error("Overlay dimensions must be 1..2048");
            bytes += std::size_t(w) * h * 4;
            if (bytes > 32 * 1024 * 1024)
                throw std::runtime_error("Dialogue overlays exceed 32 MiB decoded RGBA");
            auto image = preparedImage(path);
            if (!image.value.data || image.value.width != int(w) || image.value.height != int(h))
                throw std::runtime_error("Cannot decode overlay PNG");
            ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            if (!image.value.data)
                throw std::runtime_error("Cannot prepare overlay RGBA");
            staged[i] = GpuLease<TextureHandle>(resources_, resources_.createTexture(image.value));
            SetTextureFilter(resources_.texture(staged[i].get()), TEXTURE_FILTER_BILINEAR);
        } catch (const std::exception& e) {
            throw std::runtime_error("Dialogue overlay " + files[i] + ": " + e.what());
        }
    }
    dialogue_ = std::move(staged);
}
void UiArt::clearDialogue() noexcept {
    for (auto& texture : dialogue_)
        texture.reset();
}
void UiArt::dialogueOverlay(DialogueOverlayId id, UiRect destination, Color tint) const {
    if (!id || !dialogue_[id.index])
        return;
    const auto& texture = resources_.texture(dialogue_[id.index].get());
    const float scale =
        std::min(float(destination.width) / texture.width, float(destination.height) / texture.height);
    constexpr float native = float(gameWidth) / artWidth;
    const float w = texture.width * scale, h = texture.height * scale;
    DrawTexturePro(texture, {0, 0, float(texture.width), float(texture.height)},
                   {(destination.x + (destination.width - w) * .5f) * native,
                    (destination.y + (destination.height - h) * .5f) * native, w * native, h * native},
                   {0, 0}, 0, tint);
}
} // namespace blobber

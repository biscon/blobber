#include "assets/npc_loader.hpp"
#include "assets/image.hpp"
#include <algorithm>
#include <memory>
#include <stdexcept>
namespace blobber {
namespace {
NpcImage decodeImage(const NpcImageReference& ref, std::size_t& pixelCount, std::size_t& highWater) {
    NpcImage result;
    if (ref.path.empty())
        return result;
    result.anchorX = ref.anchorX;
    result.anchorY = ref.anchorY;
    const auto& path = ref.path;
    OwnedImage image(LoadImage(path.string().c_str()));
    if (!image.value.data)
        throw std::runtime_error("Missing or invalid NPC image: " + path.string());
    result.width = image.value.width;
    result.height = image.value.height;
    if (result.width <= 0 || result.height <= 0 || result.width > npcMaxImageDimension ||
        result.height > npcMaxImageDimension)
        throw std::runtime_error("NPC image exceeds 2048 pixel dimension: " + path.string());
    const std::size_t count = static_cast<std::size_t>(result.width) * result.height;
    if (count > npcPixelBudget - pixelCount)
        throw std::runtime_error("NPC source images exceed 512 MiB source bound");
    // The decoded image, raylib color copy and owning vector overlap briefly.
    const auto peak = (pixelCount + 3 * count) * sizeof(Color);
    requirePreparationBytes(0, peak, PreparationLimits{}.loadingBytes);
    highWater = std::max(highWater, peak);
    pixelCount += count;
    std::unique_ptr<Color, decltype(&UnloadImageColors)> colors(LoadImageColors(image.value),
                                                                UnloadImageColors);
    if (!colors)
        throw std::runtime_error("Cannot decode NPC image: " + path.string());
    result.pixels.assign(colors.get(), colors.get() + count);
    return result;
}
} // namespace
LoadedNpcs loadNpcs(const std::filesystem::path& file, const Map& map, Cell spawn) {
    auto parsed = parseNpcMetadata(file, map, spawn);
    LoadedNpcs out{std::move(parsed.world), {}, parsed.definitionFingerprint, 0};
    std::size_t pixels = 0;
    for (const auto& definition : parsed.definitions) {
        NpcAppearance a;
        a.width = definition.width;
        a.height = definition.height;
        a.attachments = definition.attachments;
        a.alphaCutoff = definition.alphaCutoff;
        a.sampling = definition.areaSampling ? ImageSampling::Area : ImageSampling::Nearest;
        for (std::size_t side = 0; side < 4; ++side)
            a.views[side] = decodeImage(definition.views[side], pixels, out.sourceDecodeHighWater);
        for (std::size_t clip = 0; clip < npcAnimationCount; ++clip) {
            a.animations[clip].timing = definition.animations[clip].timing;
            a.animations[clip].width = definition.animations[clip].width;
            for (std::size_t side = 0; side < 4; ++side)
                for (const auto& ref : definition.animations[clip].views[side])
                    a.animations[clip].views[side].push_back(
                        decodeImage(ref, pixels, out.sourceDecodeHighWater));
        }
        validateNpcAppearance(a);
        out.appearances.push_back(std::move(a));
    }
    return out;
}
} // namespace blobber

#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
namespace blobber {
struct Options {
    bool noAudio = false, development = false, supersampling = true, uncapped = false, help = false;
    bool sanctuaryCityReview = false;
    int width = 1920, height = 1080;
    std::filesystem::path assets, campaignSave, settings;
    std::optional<std::uint32_t> seed;
};
Options parseOptions(int argc, char** argv);
// A restore never consumes entropy; explicit overrides remain reproducible.
template <class Entropy>
std::uint32_t chooseCampaignSeed(std::optional<std::uint32_t> overrideSeed,
                                 std::optional<std::uint32_t> savedSeed, Entropy entropy) {
    return savedSeed ? *savedSeed : overrideSeed ? *overrideSeed : std::uint32_t(entropy());
}
} // namespace blobber

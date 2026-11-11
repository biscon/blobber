#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
namespace blobber {
class DistributionSmoke;
enum class SmokePhase { None, Create, Continue };
struct Options {
    bool noAudio = false, development = false, supersampling = true, uncapped = false, help = false;
    bool sanctuaryCityReview = false, ambientOcclusion = true;
    int width = 1920, height = 1080;
    // Editable runtime assets and offline-generated output have independent roots in Debug.
    std::filesystem::path assets, preparedAssets, campaignSave, settings, dialogueDir;
    std::optional<std::uint32_t> seed;
    SmokePhase smokePhase = SmokePhase::None;
    std::filesystem::path smokeOutput;
    DistributionSmoke* smoke = nullptr; // Borrowed only by run(); outlives all application loops.
};
Options parseOptions(int argc, char** argv);
// A restore never consumes entropy; explicit overrides remain reproducible.
template <class Entropy>
std::uint32_t chooseCampaignSeed(std::optional<std::uint32_t> overrideSeed,
                                 std::optional<std::uint32_t> savedSeed, Entropy entropy) {
    return savedSeed ? *savedSeed : overrideSeed ? *overrideSeed : std::uint32_t(entropy());
}
} // namespace blobber

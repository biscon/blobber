#include "app/options.hpp"
#include "raylib.h"
#include <charconv>
#include <cstring>
#include <stdexcept>
#include <string>
namespace blobber {
Options parseOptions(int argc, char** argv) {
    Options result;
    result.assets = std::filesystem::path(GetApplicationDirectory()) / "assets";
    result.preparedAssets = result.assets / "prepared";
#ifdef BLOBBER_SOURCE_ASSETS
    result.assets = BLOBBER_SOURCE_ASSETS;
#endif
    auto dimension = [](const char* text) {
        std::size_t consumed = 0;
        const int value = std::stoi(text, &consumed);
        if (text[consumed] != '\0')
            throw std::invalid_argument("Invalid window dimension");
        return value;
    };
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--assets" && i + 1 < argc) {
            result.assets = argv[++i];
            result.preparedAssets = result.assets / "prepared";
        } else if (arg == "--dialogue-dir" && i + 1 < argc)
            result.dialogueDir = std::filesystem::absolute(argv[++i]);
        else if (arg == "--sanctuary-city-review")
            result.sanctuaryCityReview = true;
        else if (arg == "--distribution-smoke" && i + 1 < argc) {
            const std::string phase = argv[++i];
            if (phase != "create" && phase != "continue")
                throw std::invalid_argument("Distribution smoke expects create or continue");
            result.smokePhase = phase == "create" ? SmokePhase::Create : SmokePhase::Continue;
        } else if (arg == "--smoke-output" && i + 1 < argc)
            result.smokeOutput = std::filesystem::absolute(argv[++i]).lexically_normal();
        else if (arg == "--no-audio")
            result.noAudio = true;
        else if (arg == "--uncapped")
            result.uncapped = true;
        else if (arg == "--settings" && i + 1 < argc)
            result.settings = std::filesystem::absolute(argv[++i]);
        else if (arg == "--campaign-save" && i + 1 < argc)
            result.campaignSave = std::filesystem::absolute(argv[++i]);
        else if (arg == "--supersampling" && i + 1 < argc) {
            const std::string value = argv[++i];
            if (value != "on" && value != "off")
                throw std::invalid_argument("Supersampling expects on or off");
            result.supersampling = value == "on";
        } else if (arg == "--ambient-occlusion" && i + 1 < argc) {
            const std::string value = argv[++i];
            if (value != "on" && value != "off")
                throw std::invalid_argument("Ambient occlusion expects on or off");
            result.ambientOcclusion = value == "on";
        } else if (arg == "--development")
            result.development = true;
        else if (arg == "--size" && i + 2 < argc) {
            result.width = dimension(argv[++i]);
            result.height = dimension(argv[++i]);
        } else if (arg == "--seed" && i + 1 < argc) {
            const char* value = argv[++i];
            std::uint32_t seed = 0;
            const auto parsed = std::from_chars(value, value + std::strlen(value), seed);
            if (parsed.ec != std::errc{} || parsed.ptr != value + std::strlen(value))
                throw std::runtime_error("Seed must be an unsigned 32-bit integer");
            result.seed = seed;
        } else if (arg == "--help")
            result.help = true;
        else
            throw std::runtime_error("Unknown or incomplete argument: " + arg);
    }
    if (result.smokePhase != SmokePhase::None) {
        if (result.smokeOutput.empty() || !result.campaignSave.empty() || !result.settings.empty() ||
            !result.dialogueDir.empty() || result.sanctuaryCityReview || result.development || result.seed ||
            result.width != 1920 || result.height != 1080)
            throw std::invalid_argument("Distribution smoke requires default paths, seed and presentation");
        for (int i = 1; i < argc; ++i)
            if (std::string(argv[i]) == "--assets")
                throw std::invalid_argument("Distribution smoke requires default asset discovery");
#ifdef BLOBBER_SOURCE_ASSETS
        throw std::invalid_argument("Distribution smoke requires packaged Release asset paths");
#endif
        result.noAudio = true;
        result.seed = 5;
    } else if (!result.smokeOutput.empty())
        throw std::invalid_argument("--smoke-output requires --distribution-smoke");
    if (result.sanctuaryCityReview && !result.campaignSave.empty())
        throw std::invalid_argument("City review does not use campaign saves");
    if (result.width < 320 || result.height < 240)
        throw std::runtime_error("Window must be at least 320 x 240");
    return result;
}
} // namespace blobber

#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
namespace blobber {
struct Options {
    bool development = false, supersampling = true, help = false;
    int width = 1920, height = 1080;
    std::filesystem::path assets, campaignSave;
    std::optional<std::uint32_t> seed;
};
Options parseOptions(int argc, char** argv);
} // namespace blobber

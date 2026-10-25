#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
namespace blobber {
struct Options {
    int width = 1280, height = 720;
    std::filesystem::path assets, captureDirectory;
    bool help = false, uiDemo = false, combatDemo = false;
    std::optional<std::uint32_t> seed;
};
Options parseOptions(int argc, char** argv);
} // namespace blobber

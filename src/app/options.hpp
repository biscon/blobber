#pragma once
#include <filesystem>
namespace blobber {
struct Options {
    int width = 1280, height = 720;
    std::filesystem::path assets, captureDirectory;
    bool help = false, uiDemo = false;
};
Options parseOptions(int argc, char** argv);
} // namespace blobber

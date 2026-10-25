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
    auto dimension = [](const char* text) {
        std::size_t consumed = 0;
        const int value = std::stoi(text, &consumed);
        if (text[consumed] != '\0')
            throw std::invalid_argument("Invalid window dimension");
        return value;
    };
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--assets" && i + 1 < argc)
            result.assets = argv[++i];
        else if (arg == "--capture-reactions" && i + 1 < argc) {
            result.captureReactionsOnly = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--capture-hud" && i + 1 < argc) {
            result.captureHudOnly = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--capture" && i + 1 < argc)
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        else if (arg == "--size" && i + 2 < argc) {
            result.width = dimension(argv[++i]);
            result.height = dimension(argv[++i]);
        } else if (arg == "--ui-demo")
            result.uiDemo = true;
        else if (arg == "--combat-demo")
            result.combatDemo = true;
        else if (arg == "--seed" && i + 1 < argc) {
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
    if (result.uiDemo && result.combatDemo)
        throw std::runtime_error("Choose either --ui-demo or --combat-demo");
    if (result.width < 320 || result.height < 240)
        throw std::runtime_error("Window must be at least 320 x 240");
    return result;
}
} // namespace blobber

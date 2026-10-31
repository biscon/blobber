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
        else if (arg == "--campaign-save" && i + 1 < argc)
            result.campaignSave = std::filesystem::absolute(argv[++i]);
        else if (arg == "--supersampling" && i + 1 < argc) {
            const std::string value = argv[++i];
            if (value != "on" && value != "off")
                throw std::invalid_argument("Supersampling expects on or off");
            result.supersampling = value == "on";
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
    if (result.width < 320 || result.height < 240)
        throw std::runtime_error("Window must be at least 320 x 240");
    return result;
}
} // namespace blobber

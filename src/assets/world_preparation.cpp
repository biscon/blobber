#include "assets/world_preparation.hpp"
#include "assets/npc_sheet.hpp"
#include "assets/preparation_limits.hpp"
#include "render/image_sampling.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace blobber {
std::vector<double> npcPreparationResolutions(const std::filesystem::path& file,
                                              const std::vector<std::string>& types) {
    std::vector<double> result(types.size(), 1);
    if (!std::filesystem::exists(file))
        return result;
    if (std::filesystem::file_size(file) > 1024 * 1024)
        throw std::runtime_error("NPC preparation profile exceeds 1 MiB");
    std::ifstream input(file);
    const auto config = nlohmann::json::parse(input);
    if (!config.is_object() || config.size() != 2 || !config.contains("version") ||
        !config["version"].is_number_integer() || config["version"] != 1 || !config.contains("actors") ||
        !config["actors"].is_object())
        throw std::runtime_error("Invalid NPC preparation profile");
    for (const auto& [name, value] : config["actors"].items()) {
        const auto type = std::find(types.begin(), types.end(), name);
        if (type == types.end() || !value.is_number() || !npcResolutionSupported(value.get<double>()))
            throw std::runtime_error("Invalid NPC preparation resolution: " + name);
        result[std::size_t(type - types.begin())] = value.get<double>();
    }
    return result;
}
void limitWorldSprite(OwnedImage& image, unsigned maximum) {
    const auto& source = image.value;
    if (!source.data || source.width <= 0 || source.height <= 0 || source.width > 4096 ||
        source.height > 4096 || !maximum || maximum > 4096)
        throw std::runtime_error("Invalid world sprite dimensions");
    const double scale = std::min(1., double(maximum) / std::max(source.width, source.height));
    if (scale == 1)
        return;
    const int width = std::max(1, int(std::round(source.width * scale)));
    const int height = std::max(1, int(std::round(source.height * scale)));
    const auto count = std::size_t(source.width) * source.height;
    requirePreparationBytes(count * 8 + std::size_t(width) * height * 4,
                            std::size_t(source.width + 1) * (source.height + 1) * sizeof(double) * 4,
                            PreparationLimits{}.loadingBytes);
    std::unique_ptr<Color, decltype(&UnloadImageColors)> colors(LoadImageColors(source), UnloadImageColors);
    if (!colors)
        throw std::runtime_error("Cannot decode world sprite");
    AreaSampler sampler(colors.get(), source.width, source.height, source.width);
    OwnedImage output(GenImageColor(width, height, BLANK));
    if (!output.value.data)
        throw std::runtime_error("Cannot prepare world sprite");
    auto* target = static_cast<Color*>(output.value.data);
    const double dx = double(source.width) / width, dy = double(source.height) / height;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            target[y * width + x] = sampler.sample(x * dx, y * dy, (x + 1) * dx, (y + 1) * dy);
    std::swap(image.value, output.value);
}
} // namespace blobber

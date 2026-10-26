// Offline sprite-sheet extraction. All poses are authored by imagegen; this tool
// applies one uniform scale per sheet and registered ground/body pivots only.
#include "assets/npc_sheet.hpp"
#include "assets/fingerprint.hpp"
#include "assets/image.hpp"
#include "render/preparation_limits.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
namespace fs = std::filesystem;
using Json = nlohmann::json;
namespace blobber {
void prepareNpcAnimation(const fs::path& configPath, const NpcSheetOptions& options) {
    if ((options.sampling != ImageSampling::Nearest && options.sampling != ImageSampling::Area) ||
        (options.resolution != 1 && options.resolution != 0.5))
        throw std::runtime_error("Sheet profile requires nearest/area and resolution 1 or 0.5");
    const fs::path root = configPath.parent_path();
    const fs::path destination = options.outputDirectory.empty() ? root : options.outputDirectory;
    if (fs::file_size(configPath) > 1024 * 1024)
        throw std::runtime_error("Sheet configuration exceeds 1 MiB");
    Json config;
    std::ifstream(configPath) >> config;
    Json frames = Json::object();
    Json provenance{{"version", 1},
                    {"fingerprint", "fnv1a64"},
                    {"sampling", options.sampling == ImageSampling::Area ? "area" : "nearest"},
                    {"resolution", options.resolution},
                    {"registration", config},
                    {"registration_fingerprint", fingerprintFile(configPath)},
                    {"sources", Json::object()},
                    {"outputs", Json::object()}};
    if (!config.is_array() || config.size() > 96)
        throw std::runtime_error("Sheet configuration must be an array of at most 96 sheets");
    fs::create_directories(destination / "frames");
    for (const auto& entry : config) {
        const auto name = entry.at("name").get<std::string>();
        if (name.empty() || name == "." || name == ".." || name.find_first_of("/\\") != std::string::npos)
            throw std::runtime_error("Invalid sheet name");
        blobber::OwnedImage sheet(LoadImage((root / "sheets" / (name + ".png")).string().c_str()));
        if (!sheet.value.data)
            throw std::runtime_error("Cannot load sheet " + name);
        if (sheet.value.width > 4096 || sheet.value.height > 4096)
            throw std::runtime_error("Sheet exceeds 4096 pixel dimension: " + name);
        provenance["sources"][name + ".png"] = fingerprintFile(root / "sheets" / (name + ".png"));
        std::unique_ptr<Color, decltype(&UnloadImageColors)> source(LoadImageColors(sheet.value),
                                                                    UnloadImageColors);
        if (!source)
            throw std::runtime_error("Cannot decode sheet " + name);
        const int split = entry.at("split_x"), rows = entry.value("rows", 1);
        if (rows != 1 && rows != 2)
            throw std::runtime_error("Sheet rows must be 1 or 2: " + name);
        const int splitY = entry.value("split_y", sheet.value.height / 2);
        const double authoredScale = entry.at("scale");
        const double scale = authoredScale * options.resolution;
        const int authoredWidth = entry.value("canvas_width", 384);
        const int authoredHeight = entry.value("canvas_height", 536);
        if (authoredWidth <= 0 || authoredWidth > 2048 || authoredHeight <= 0 || authoredHeight > 2048 ||
            !std::isfinite(authoredScale) || authoredScale <= 0 || authoredScale > 2 ||
            std::floor(authoredWidth * options.resolution) != authoredWidth * options.resolution ||
            std::floor(authoredHeight * options.resolution) != authoredHeight * options.resolution)
            throw std::runtime_error("Invalid sheet size or nonintegral scaled canvas: " + name);
        const int canvasWidth = static_cast<int>(authoredWidth * options.resolution);
        const int canvasHeight = static_cast<int>(authoredHeight * options.resolution);
        const double anchorX = entry.value("anchor_x", authoredWidth * 0.5) * options.resolution;
        const double anchorY = entry.value("anchor_y", 512.0) * options.resolution;
        if (canvasWidth <= 0 || canvasWidth > 2048 || canvasHeight <= 0 || canvasHeight > 2048 ||
            !std::isfinite(anchorX) || !std::isfinite(anchorY) || anchorX < 0 || anchorX > canvasWidth ||
            anchorY <= 0 || anchorY > canvasHeight ||
            (entry.contains("ground_y") &&
             (!entry.at("ground_y").is_array() ||
              entry.at("ground_y").size() != static_cast<std::size_t>(rows * 2))))
            throw std::runtime_error("Invalid sheet canvas/registration: " + name);
        if (split <= 0 || split >= sheet.value.width ||
            (rows == 2 && (splitY <= 0 || splitY >= sheet.value.height)) || !std::isfinite(scale) ||
            scale <= 0 || scale > 2 || entry.at("pivot_x").size() != static_cast<std::size_t>(rows * 2))
            throw std::runtime_error("Invalid sheet geometry: " + name);
        for (int f = 0; f < rows * 2; ++f) {
            const int originX = f % 2 ? split : 0, originY = f >= 2 ? splitY : 0;
            const int cellHeight =
                rows == 1 ? sheet.value.height : (f >= 2 ? sheet.value.height - splitY : splitY);
            const int width = f % 2 ? sheet.value.width - split : split;
            const double pivot = entry.at("pivot_x")[f];
            if (!std::isfinite(pivot) || pivot < 0 || pivot > width)
                throw std::runtime_error("Invalid pivot: " + name);
            auto pixel = [&](int x, int y) {
                return source.get()[(originY + y) * sheet.value.width + originX + x];
            };
            int ground = 0;
            for (int y = 0; y < cellHeight; ++y)
                for (int x = 0; x < width; ++x)
                    if (pixel(x, y).a > 15)
                        ground = y + 1;
            if (!ground)
                throw std::runtime_error("Empty sprite: " + name);
            const double registeredGround =
                entry.contains("ground_y") ? entry.at("ground_y")[f].get<double>() : ground;
            if (!std::isfinite(registeredGround) || registeredGround <= 0 || registeredGround > cellHeight ||
                registeredGround <= ground - 0.5)
                throw std::runtime_error("Invalid ground or visible pixels below ground: " + name);
            std::optional<AreaSampler> area;
            const auto imageBytes =
                static_cast<std::size_t>(sheet.value.width) * sheet.value.height * sizeof(Color) * 2 +
                static_cast<std::size_t>(canvasWidth) * canvasHeight * sizeof(Color);
            const auto scratchBytes =
                options.sampling == ImageSampling::Area
                    ? static_cast<std::size_t>(width + 1) * (cellHeight + 1) * sizeof(double) * 4
                    : 0;
            requirePreparationBytes(imageBytes, scratchBytes, PreparationLimits{}.loadingBytes);
            if (options.loadingHighWater)
                *options.loadingHighWater = std::max(*options.loadingHighWater, imageBytes + scratchBytes);
            if (options.sampling == ImageSampling::Area)
                area.emplace(source.get() + originY * sheet.value.width + originX, width, cellHeight,
                             sheet.value.width, 15);
            for (int y = 0; y < cellHeight; ++y)
                for (int x = 0; x < width; ++x) {
                    if (pixel(x, y).a <= 15)
                        continue;
                    const double dx = anchorX + (x + 0.5 - pivot) * scale;
                    const double dy = anchorY + (y + 0.5 - registeredGround) * scale;
                    if (dx < 0 || dx >= canvasWidth || dy < 0 || dy >= canvasHeight)
                        throw std::runtime_error("Sprite would clip output canvas: " + name);
                    if (area && (dx - scale * .5 < 0 || dx + scale * .5 > canvasWidth ||
                                 dy - scale * .5 < 0 || dy + scale * .5 > canvasHeight))
                        throw std::runtime_error("Filtered sprite footprint would clip output canvas: " +
                                                 name);
                }
            blobber::OwnedImage output(GenImageColor(canvasWidth, canvasHeight, BLANK));
            if (!output.value.data)
                throw std::runtime_error("Cannot allocate output");
            ImageFormat(&output.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            auto* target = static_cast<Color*>(output.value.data);
            for (int y = 0; y < canvasHeight; ++y)
                for (int x = 0; x < canvasWidth; ++x) {
                    if (area) {
                        // The output row center must remain above the registered ground.
                        if (y + 0.5 < anchorY)
                            target[y * canvasWidth + x] = area->sample(
                                pivot + (x - anchorX) / scale, registeredGround + (y - anchorY) / scale,
                                pivot + (x + 1 - anchorX) / scale,
                                registeredGround + (y + 1 - anchorY) / scale);
                        continue;
                    }
                    const int sx = static_cast<int>(std::floor(pivot + (x + 0.5 - anchorX) / scale));
                    const int sy =
                        static_cast<int>(std::floor(registeredGround + (y + 0.5 - anchorY) / scale));
                    if (sx >= 0 && sx < width && sy >= 0 && sy < cellHeight)
                        target[y * canvasWidth + x] = pixel(sx, sy);
                }
            const std::string file = name + "-" + std::to_string(f) + ".png";
            if (!ExportImage(output.value, (destination / "frames" / file).string().c_str()))
                throw std::runtime_error("Cannot export " + file);
            provenance["outputs"][file] = {{"width", canvasWidth},
                                           {"height", canvasHeight},
                                           {"fingerprint", fingerprintFile(destination / "frames" / file)}};
            frames[name].push_back({{"image", "animation/frames/" + file},
                                    {"anchor", {anchorX / canvasWidth, anchorY / canvasHeight}}});
            std::cout << file << ": source ground=" << registeredGround << ", scale=" << scale << '\n';
        }
    }
    std::ofstream out(destination / "frames.json");
    out << frames.dump(2) << '\n';
    if (!out)
        throw std::runtime_error("Cannot write frame manifest");
    std::ofstream metadata(destination / "derivation.json");
    metadata << provenance.dump(2) << '\n';
    if (!metadata)
        throw std::runtime_error("Cannot write derivation metadata");
}
} // namespace blobber

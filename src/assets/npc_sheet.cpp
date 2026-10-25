// Offline sprite-sheet extraction. All poses are authored by imagegen; this tool
// applies one uniform scale per sheet and registered ground/body pivots only.
#include "assets/npc_sheet.hpp"
#include "assets/image.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace fs = std::filesystem;
using Json = nlohmann::json;
namespace blobber {
void prepareNpcAnimation(const fs::path& configPath) {
    const fs::path root = configPath.parent_path();
    Json config;
    std::ifstream(configPath) >> config;
    Json frames = Json::object();
    if (!config.is_array())
        throw std::runtime_error("Sheet configuration must be an array");
    fs::create_directories(root / "frames");
    for (const auto& entry : config) {
        const auto name = entry.at("name").get<std::string>();
        blobber::OwnedImage sheet(LoadImage((root / "sheets" / (name + ".png")).string().c_str()));
        if (!sheet.value.data)
            throw std::runtime_error("Cannot load sheet " + name);
        std::unique_ptr<Color, decltype(&UnloadImageColors)> source(LoadImageColors(sheet.value),
                                                                    UnloadImageColors);
        if (!source)
            throw std::runtime_error("Cannot decode sheet " + name);
        const int split = entry.at("split_x"), rows = entry.value("rows", 1);
        if (rows != 1 && rows != 2)
            throw std::runtime_error("Sheet rows must be 1 or 2: " + name);
        const int splitY = entry.value("split_y", sheet.value.height / 2);
        const double scale = entry.at("scale");
        const int canvasWidth = entry.value("canvas_width", 384);
        const int canvasHeight = entry.value("canvas_height", 536);
        const double anchorX = entry.value("anchor_x", canvasWidth * 0.5);
        const double anchorY = entry.value("anchor_y", 512.0);
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
            for (int y = 0; y < cellHeight; ++y)
                for (int x = 0; x < width; ++x) {
                    if (pixel(x, y).a <= 15)
                        continue;
                    const double dx = anchorX + (x + 0.5 - pivot) * scale;
                    const double dy = anchorY + (y + 0.5 - registeredGround) * scale;
                    if (dx < 0 || dx >= canvasWidth || dy < 0 || dy >= canvasHeight)
                        throw std::runtime_error("Sprite would clip output canvas: " + name);
                }
            blobber::OwnedImage output(GenImageColor(canvasWidth, canvasHeight, BLANK));
            if (!output.value.data)
                throw std::runtime_error("Cannot allocate output");
            ImageFormat(&output.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            auto* target = static_cast<Color*>(output.value.data);
            for (int y = 0; y < canvasHeight; ++y)
                for (int x = 0; x < canvasWidth; ++x) {
                    const int sx = static_cast<int>(std::floor(pivot + (x + 0.5 - anchorX) / scale));
                    const int sy =
                        static_cast<int>(std::floor(registeredGround + (y + 0.5 - anchorY) / scale));
                    if (sx >= 0 && sx < width && sy >= 0 && sy < cellHeight)
                        target[y * canvasWidth + x] = pixel(sx, sy);
                }
            const std::string file = name + "-" + std::to_string(f) + ".png";
            if (!ExportImage(output.value, (root / "frames" / file).string().c_str()))
                throw std::runtime_error("Cannot export " + file);
            frames[name].push_back({{"image", "animation/frames/" + file},
                                    {"anchor", {anchorX / canvasWidth, anchorY / canvasHeight}}});
            std::cout << file << ": source ground=" << registeredGround << ", scale=" << scale << '\n';
        }
    }
    std::ofstream out(root / "frames.json");
    out << frames.dump(2) << '\n';
    if (!out)
        throw std::runtime_error("Cannot write frame manifest");
}
} // namespace blobber

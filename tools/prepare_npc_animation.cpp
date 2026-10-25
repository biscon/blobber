// Offline sprite-sheet extraction. All poses are authored by imagegen; this tool
// applies one uniform scale per sheet and registered ground/body pivots only.
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
int main(int argc, char** argv) {
    try {
        if (argc != 2)
            throw std::runtime_error("Usage: blobber_prepare_npc_animation SHEETS_JSON");
        SetTraceLogLevel(LOG_WARNING);
        const fs::path configPath(argv[1]), root = configPath.parent_path();
        Json config;
        std::ifstream(configPath) >> config;
        Json frames = Json::object();
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
            const double scale = entry.at("scale");
            if (split <= 0 || split >= sheet.value.width || (rows != 1 && rows != 2) ||
                sheet.value.height % rows || !std::isfinite(scale) || scale <= 0 || scale > 2 ||
                entry.at("pivot_x").size() != static_cast<std::size_t>(rows * 2))
                throw std::runtime_error("Invalid sheet geometry: " + name);
            const int cellHeight = sheet.value.height / rows;
            for (int f = 0; f < rows * 2; ++f) {
                const int originX = f % 2 ? split : 0, originY = f / 2 * cellHeight;
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
                for (int y = 0; y < cellHeight; ++y)
                    for (int x = 0; x < width; ++x) {
                        if (pixel(x, y).a <= 15)
                            continue;
                        const double dx = 192 + (x + 0.5 - pivot) * scale;
                        const double dy = 512 + (y + 0.5 - ground) * scale;
                        if (dx < 0 || dx >= 384 || dy < 0 || dy >= 536)
                            throw std::runtime_error("Sprite would clip output canvas: " + name);
                    }
                blobber::OwnedImage output(GenImageColor(384, 536, BLANK));
                if (!output.value.data)
                    throw std::runtime_error("Cannot allocate output");
                ImageFormat(&output.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                auto* target = static_cast<Color*>(output.value.data);
                for (int y = 0; y < 536; ++y)
                    for (int x = 0; x < 384; ++x) {
                        const int sx = static_cast<int>(std::floor(pivot + (x + 0.5 - 192) / scale));
                        const int sy = static_cast<int>(std::floor(ground + (y + 0.5 - 512) / scale));
                        if (sx >= 0 && sx < width && sy >= 0 && sy < cellHeight)
                            target[y * 384 + x] = pixel(sx, sy);
                    }
                const std::string file = name + "-" + std::to_string(f) + ".png";
                if (!ExportImage(output.value, (root / "frames" / file).string().c_str()))
                    throw std::runtime_error("Cannot export " + file);
                frames[name].push_back(
                    {{"image", "animation/frames/" + file}, {"anchor", {0.5, 512.0 / 536}}});
                std::cout << file << ": source ground=" << ground << ", scale=" << scale << '\n';
            }
        }
        std::ofstream out(root / "frames.json");
        out << frames.dump(2) << '\n';
        if (!out)
            throw std::runtime_error("Cannot write frame manifest");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

#include "assets/npc_derivatives.hpp"
#include "assets/fingerprint.hpp"
#include "assets/image.hpp"
#include "assets/npc_sheet.hpp"
#include "render/npc_sprites.hpp"
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>

namespace blobber {
namespace {
using Json = nlohmann::json;
namespace fs = std::filesystem;
Json read(const fs::path& path) {
    if (fs::file_size(path) > 1024 * 1024)
        throw std::runtime_error("NPC JSON exceeds 1 MiB");
    std::ifstream in(path);
    return Json::parse(in);
}
fs::path relative(const std::string& name) {
    const fs::path p(name);
    if (name.empty() || name.size() > 4096 || p.is_absolute())
        throw std::runtime_error("Derivative inputs require relative paths within the definition directory");
    for (const auto& part : p)
        if (part == "..")
            throw std::runtime_error("Derivative input path escapes definition directory");
    return p;
}
Json resize(const fs::path& source, const fs::path& destination, float anchorY, unsigned char cutoff,
            std::size_t* highWater) {
    OwnedImage original(LoadImage(source.string().c_str()));
    if (!original.value.data || original.value.width <= 0 || original.value.height <= 0 ||
        original.value.width > npcMaxImageDimension || original.value.height > npcMaxImageDimension ||
        original.value.width % 2 || original.value.height % 2)
        throw std::runtime_error("Profile C requires even NPC source dimensions, at most 2048: " +
                                 source.string());
    ImageFormat(&original.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const int width = original.value.width / 2, height = original.value.height / 2;
    const auto working =
        static_cast<std::size_t>(original.value.width) * original.value.height * sizeof(Color) +
        static_cast<std::size_t>(width) * height * sizeof(Color) +
        static_cast<std::size_t>(original.value.width + 1) * (original.value.height + 1) * sizeof(double) * 4;
    requirePreparationBytes(0, working, PreparationLimits{}.loadingBytes);
    if (highWater)
        *highWater = std::max(*highWater, working);
    const auto* pixels = static_cast<const Color*>(original.value.data);
    for (int y = 0; y < original.value.height; ++y)
        if (y + .5f >= anchorY * original.value.height)
            for (int x = 0; x < original.value.width; ++x)
                if (pixels[y * original.value.width + x].a > cutoff)
                    throw std::runtime_error("Source NPC pixels extend below ground anchor");
    const AreaSampler area(pixels, original.value.width, original.value.height, original.value.width, cutoff);
    OwnedImage output(GenImageColor(width, height, BLANK));
    if (!output.value.data)
        throw std::runtime_error("Cannot allocate NPC derivative");
    auto* result = static_cast<Color*>(output.value.data);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            if (y + .5f < anchorY * height)
                result[y * width + x] = area.sample(x * 2, y * 2, (x + 1) * 2, (y + 1) * 2);
    fs::create_directories(destination.parent_path());
    if (!ExportImage(output.value, destination.string().c_str()))
        throw std::runtime_error("Cannot export NPC derivative");
    return {{"source", fingerprintFile(source)},
            {"output", fingerprintFile(destination)},
            {"source_dimensions", {original.value.width, original.value.height}},
            {"dimensions", {width, height}},
            {"anchor_y", anchorY},
            {"alpha_cutoff", cutoff}};
}
} // namespace
std::filesystem::path prepareNpcDerivatives(const fs::path& levelPath, const fs::path& destination,
                                            std::size_t* loadingHighWater) {
    auto level = read(levelPath);
    auto& definitions = level.at("definitions");
    if (!definitions.is_array() || definitions.size() > NpcWorld::maxTypes)
        throw std::runtime_error("Invalid derivative definition count");
    fs::create_directories(destination);
    for (std::size_t i = 0; i < definitions.size(); ++i) {
        const auto source = levelPath.parent_path() / relative(definitions[i].get<std::string>());
        const auto definition = read(source);
        const auto root = destination / std::to_string(i);
        fs::create_directories(root);
        const bool sheets = definition.contains("source_sheets");
        std::set<fs::path> sheetFrames;
        if (sheets) {
            const auto registration = relative(definition.at("source_sheets").get<std::string>());
            prepareNpcAnimation(
                source.parent_path() / registration,
                {ImageSampling::Area, .5, root / registration.parent_path(), loadingHighWater});
            const auto produced = read(root / registration.parent_path() / "derivation.json");
            for (auto it = produced.at("outputs").begin(); it != produced.at("outputs").end(); ++it)
                sheetFrames.insert((registration.parent_path() / "frames" / it.key()).lexically_normal());
        }
        std::map<std::string, float> seen;
        Json derivation{{"version", 1}, {"sampling", "area"}, {"resolution", .5}, {"frames", Json::object()}};
        const auto image = [&](const Json& spec) {
            const auto name =
                spec.is_string() ? spec.get<std::string>() : spec.at("image").get<std::string>();
            const auto path = relative(name);
            if (sheets) {
                if (!sheetFrames.count(path.lexically_normal()) || !fs::is_regular_file(root / path))
                    throw std::runtime_error("Registered sheets did not produce required frame: " + name);
            } else {
                const float anchorY =
                    spec.is_object() && spec.contains("anchor") ? spec.at("anchor")[1].get<float>() : 1.f;
                const auto prior = seen.find(name);
                if (prior != seen.end()) {
                    if (prior->second != anchorY)
                        throw std::runtime_error("Shared source image requires a consistent ground anchor");
                    return;
                }
                seen[name] = anchorY;
                derivation["frames"][name] =
                    resize(source.parent_path() / path, root / path, anchorY,
                           definition.value("alpha_cutoff", static_cast<unsigned char>(0)), loadingHighWater);
            }
        };
        if (definition.contains("views"))
            for (const auto& spec : definition.at("views"))
                image(spec);
        else
            for (const auto& clip : definition.at("animations"))
                for (const auto& view : clip.at("views"))
                    for (const auto& spec : view)
                        image(spec);
        if (!sheets) {
            std::ofstream out(root / "derivation.json");
            out << derivation.dump(2) << '\n';
            if (!out)
                throw std::runtime_error("Cannot write NPC derivative provenance");
        }
        const auto generated = root / "definition.json";
        fs::copy_file(source, generated, fs::copy_options::overwrite_existing);
        definitions[i] = fs::absolute(generated).string();
    }
    const auto generated = destination / "level.json";
    std::ofstream out(generated);
    out << level.dump(2) << '\n';
    if (!out)
        throw std::runtime_error("Cannot write derivative level");
    return generated;
}
} // namespace blobber

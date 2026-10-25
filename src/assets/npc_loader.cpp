#include "assets/npc_loader.hpp"
#include "assets/image.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace blobber {
namespace {
using Json = nlohmann::json;
Json readJson(const std::filesystem::path& path) {
    constexpr std::size_t maxBytes = 1024 * 1024;
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("Cannot open NPC JSON: " + path.string());
    std::string text;
    char block[4096];
    while (in.read(block, sizeof(block)) || in.gcount()) {
        if (text.size() + static_cast<std::size_t>(in.gcount()) > maxBytes)
            throw std::runtime_error("NPC JSON exceeds 1 MiB: " + path.string());
        text.append(block, static_cast<std::size_t>(in.gcount()));
    }
    if (in.bad())
        throw std::runtime_error("Cannot read NPC JSON: " + path.string());
    try {
        return Json::parse(text, nullptr, true, false);
    } catch (const Json::exception& e) {
        throw std::runtime_error("Invalid NPC JSON " + path.string() + ": " + e.what());
    }
}
int integer(const Json& value) {
    if (!value.is_number_integer() || value.get<double>() < std::numeric_limits<int>::min() ||
        value.get<double>() > std::numeric_limits<int>::max())
        throw std::runtime_error("NPC coordinates must be bounded integers");
    return value.get<int>();
}
Cell cell(const Json& value) {
    if (!value.is_array() || value.size() != 2)
        throw std::runtime_error("NPC cell/subcell must be [x,y]");
    return {integer(value[0]), integer(value[1])};
}
float number(const Json& value) {
    if (!value.is_number())
        throw std::runtime_error("NPC size/anchor must be numeric");
    const float n = value.get<float>();
    if (!std::isfinite(n))
        throw std::runtime_error("NPC size/anchor must be finite");
    return n;
}
Heading heading(const std::string& name) {
    const std::array<std::string, 4> names{"north", "east", "south", "west"};
    for (int i = 0; i < 4; ++i)
        if (names[i] == name)
            return static_cast<Heading>(i);
    throw std::runtime_error("Invalid NPC facing: " + name);
}
NpcAnimation animationName(const std::string& name) {
    const std::array<const char*, npcAnimationCount> names{"idle", "walk", "attack", "hurt", "death"};
    for (std::size_t i = 0; i < names.size(); ++i)
        if (name == names[i])
            return static_cast<NpcAnimation>(i);
    throw std::runtime_error("Unknown NPC animation: " + name);
}
void loadAnimations(const Json& definition, const std::filesystem::path& parent, NpcAppearance& appearance,
                    std::size_t& pixels);
NpcImage loadSprite(const std::filesystem::path& parent, const Json& spec, std::size_t& pixelCount) {
    const std::string name = spec.is_string() ? spec.get<std::string>() : spec.at("image").get<std::string>();
    const auto path = parent / name;
    OwnedImage image(LoadImage(path.string().c_str()));
    if (!image.value.data)
        throw std::runtime_error("Missing or invalid NPC image: " + path.string());
    NpcImage result;
    result.width = image.value.width;
    result.height = image.value.height;
    if (result.width <= 0 || result.height <= 0 || result.width > npcMaxImageDimension ||
        result.height > npcMaxImageDimension)
        throw std::runtime_error("NPC image exceeds 2048 pixel dimension: " + path.string());
    const std::size_t count = static_cast<std::size_t>(result.width) * result.height;
    if (count > npcPixelBudget - pixelCount)
        throw std::runtime_error("NPC source images exceed 128 MiB budget");
    pixelCount += count;
    std::unique_ptr<Color, decltype(&UnloadImageColors)> colors(LoadImageColors(image.value),
                                                                UnloadImageColors);
    if (!colors)
        throw std::runtime_error("Cannot decode NPC image: " + path.string());
    result.pixels.assign(colors.get(), colors.get() + count);
    if (spec.is_object() && spec.contains("anchor")) {
        const auto& anchor = spec.at("anchor");
        if (!anchor.is_array() || anchor.size() != 2)
            throw std::runtime_error("NPC anchor must be [x,y]");
        result.anchorX = number(anchor[0]);
        result.anchorY = number(anchor[1]);
    }
    return result;
}
void loadAnimations(const Json& definition, const std::filesystem::path& parent, NpcAppearance& appearance,
                    std::size_t& pixels) {
    const std::array<const char*, 4> sides{"front", "back", "left", "right"};
    if (definition.contains("views") == definition.contains("animations"))
        throw std::runtime_error("NPC requires either views or animations, not both");
    if (definition.contains("views")) {
        for (std::size_t i = 0; i < sides.size(); ++i)
            appearance.views[i] = loadSprite(parent, definition.at("views").at(sides[i]), pixels);
        return;
    }
    const auto& clips = definition.at("animations");
    if (!clips.is_object() || !clips.contains("idle"))
        throw std::runtime_error("NPC animations require idle clip");
    for (auto it = clips.begin(); it != clips.end(); ++it) {
        auto& clip = appearance.animations[static_cast<std::size_t>(animationName(it.key()))];
        if (it.value().contains("width")) {
            clip.width = number(it.value().at("width"));
            if (clip.width <= 0)
                throw std::runtime_error("NPC clip width must be positive");
        }
        const auto& durations = it.value().at("durations_ms");
        if (!durations.is_array() || durations.empty() || durations.size() > npcMaxFrames)
            throw std::runtime_error("NPC clips require 1..4 durations");
        clip.timing.count = static_cast<std::uint8_t>(durations.size());
        for (std::size_t f = 0; f < durations.size(); ++f) {
            const int duration = integer(durations[f]);
            if (duration <= 0 || duration > 60000)
                throw std::runtime_error("NPC frame duration must be 1..60000 ms");
            clip.timing.durationMs[f] = static_cast<std::uint32_t>(duration);
        }
        const auto& views = it.value().at("views");
        if (!views.is_object())
            throw std::runtime_error("NPC animation views must be an object");
        for (auto view = views.begin(); view != views.end(); ++view)
            if (std::find(sides.begin(), sides.end(), view.key()) == sides.end())
                throw std::runtime_error("Unknown NPC animation direction: " + view.key());
        for (std::size_t side = 0; side < sides.size(); ++side) {
            if (!views.contains(sides[side])) {
                if (it.key() == "attack" && side != 0)
                    continue;
                throw std::runtime_error("NPC clip missing required direction");
            }
            const auto& frames = views.at(sides[side]);
            if (!frames.is_array() || frames.size() != durations.size())
                throw std::runtime_error("NPC image/duration counts differ");
            for (const auto& frame : frames)
                clip.views[side].push_back(loadSprite(parent, frame, pixels));
        }
    }
}
} // namespace
LoadedNpcs loadNpcs(const std::filesystem::path& file, const Map& map, Cell spawn) {
    try {
        const Json level = readJson(file);
        const auto& files = level.at("definitions");
        const auto& placements = level.at("instances");
        if (!files.is_array() || files.size() > NpcWorld::maxTypes || !placements.is_array() ||
            placements.size() > NpcWorld::maxInstances)
            throw std::runtime_error("NPC definitions/instances must be arrays within capacity");
        std::vector<std::string> types;
        std::vector<NpcAppearance> appearances;
        std::size_t pixels = 0;
        for (const auto& entry : files) {
            const auto path = file.parent_path() / entry.get<std::string>();
            try {
                const auto definition = readJson(path);
                const auto id = definition.at("id").get<std::string>();
                if (id.empty() || id.size() > 128 || std::find(types.begin(), types.end(), id) != types.end())
                    throw std::runtime_error("Empty, oversized or duplicate NPC type ID: " + id);
                types.push_back(id);
                NpcAppearance appearance;
                appearance.width = number(definition.at("width"));
                appearance.height = number(definition.at("height"));
                if (definition.contains("alpha_cutoff")) {
                    const int cutoff = integer(definition.at("alpha_cutoff"));
                    if (cutoff < 0 || cutoff > 254)
                        throw std::runtime_error("NPC alpha_cutoff must be 0..254");
                    appearance.alphaCutoff = static_cast<unsigned char>(cutoff);
                }
                loadAnimations(definition, path.parent_path(), appearance, pixels);
                validateNpcAppearance(appearance);
                appearances.push_back(std::move(appearance));
            } catch (const std::exception& e) {
                throw std::runtime_error(path.string() + ": " + e.what());
            }
        }
        std::vector<NpcInstance> instances;
        for (const auto& placement : placements) {
            const auto id = placement.at("type").get<std::string>();
            const auto it = std::find(types.begin(), types.end(), id);
            if (it == types.end())
                throw std::runtime_error("Unknown NPC type: " + id);
            const auto animation = animationName(placement.value("animation", std::string("idle")));
            const int phase = placement.contains("phase_ms") ? integer(placement.at("phase_ms")) : 0;
            if (phase < 0)
                throw std::runtime_error("NPC phase_ms must be a nonnegative bounded integer");
            if (!npcTimings(appearances[it - types.begin()])[static_cast<std::size_t>(animation)].count)
                throw std::runtime_error("NPC placement selects an absent animation");
            instances.push_back({{static_cast<std::uint16_t>(it - types.begin())},
                                 cell(placement.at("cell")),
                                 cell(placement.at("subcell")),
                                 heading(placement.at("facing").get<std::string>()),
                                 animation,
                                 static_cast<std::uint32_t>(phase)});
        }
        return {NpcWorld(map, spawn, std::move(types), std::move(instances)), std::move(appearances)};
    } catch (const std::exception& e) {
        throw std::runtime_error("Loading NPC level " + file.string() + ": " + e.what());
    }
}
} // namespace blobber

#include "assets/npc_metadata.hpp"
#include "assets/fingerprint.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <memory>
#include <nlohmann/json.hpp>
#include <set>
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
        std::array<std::set<std::string>, 33> keys;
        return Json::parse(text, [&](int depth, Json::parse_event_t event, Json& value) {
            if (depth > 32)
                throw std::runtime_error("NPC JSON nesting exceeds 32");
            if (event == Json::parse_event_t::object_start)
                keys.at(depth).clear();
            if (event == Json::parse_event_t::key &&
                !keys.at(depth - 1).insert(value.get<std::string>()).second)
                throw std::runtime_error("Duplicate NPC JSON key");
            return true;
        });
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
    const std::array<const char*, npcAnimationCount> names{"idle",  "walk",          "attack", "hurt",
                                                           "death", "ranged_attack", "prone"};
    for (std::size_t i = 0; i < names.size(); ++i)
        if (name == names[i])
            return static_cast<NpcAnimation>(i);
    throw std::runtime_error("Unknown NPC animation: " + name);
}
void loadAnimations(const Json& definition, const std::filesystem::path& parent, NpcDefinition& appearance);
NpcImageReference loadSprite(const std::filesystem::path& parent, const Json& spec) {
    const std::string name = spec.is_string() ? spec.get<std::string>() : spec.at("image").get<std::string>();
    if (name.empty() || name.size() > 4096)
        throw std::runtime_error("NPC image path must contain 1..4096 bytes");
    NpcImageReference result;
    if (spec.is_object() && spec.contains("anchor")) {
        const auto& anchor = spec.at("anchor");
        if (!anchor.is_array() || anchor.size() != 2)
            throw std::runtime_error("NPC anchor must be [x,y]");
        result.anchorX = number(anchor[0]);
        result.anchorY = number(anchor[1]);
    }
    if (result.anchorX < 0 || result.anchorX > 1 || result.anchorY <= 0 || result.anchorY > 1)
        throw std::runtime_error("Invalid NPC image anchor");
    result.path = parent / name;
    return result;
}
void loadAnimations(const Json& definition, const std::filesystem::path& parent, NpcDefinition& appearance) {
    const std::array<const char*, 4> sides{"front", "back", "left", "right"};
    if (definition.contains("views") == definition.contains("animations"))
        throw std::runtime_error("NPC requires either views or animations, not both");
    if (definition.contains("views")) {
        for (std::size_t i = 0; i < sides.size(); ++i)
            appearance.views[i] = loadSprite(parent, definition.at("views").at(sides[i]));
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
        if (it.key() == "prone" && durations.size() != 1)
            throw std::runtime_error("NPC prone clip requires exactly one frame");
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
                clip.views[side].push_back(loadSprite(parent, frame));
        }
    }
}
} // namespace
ParsedNpcs parseNpcMetadata(const std::filesystem::path& file, const Map& map, Cell spawn,
                            bool includePlacements) {
    try {
        const Json level = readJson(file);
        const auto& files = level.at("definitions");
        const Json noPlacements = Json::array();
        const auto& placements = includePlacements ? level.at("instances") : noPlacements;
        if (!files.is_array() || files.size() > NpcWorld::maxTypes || !placements.is_array() ||
            placements.size() > NpcWorld::maxInstances)
            throw std::runtime_error("NPC definitions/instances must be arrays within capacity");
        std::vector<std::string> types;
        std::vector<NpcDefinition> appearances;
        std::uint64_t definitionHash = fingerprintBasis;
        for (const auto& entry : files) {
            const auto path = file.parent_path() / entry.get<std::string>();
            try {
                const auto definition = readJson(path);
                const auto canonical = definition.dump();
                fingerprintBytes(definitionHash, canonical.data(), canonical.size());
                const char separator = 0;
                fingerprintBytes(definitionHash, &separator, 1);
                const auto id = definition.at("id").get<std::string>();
                if (id.empty() || id.size() > 128 || std::find(types.begin(), types.end(), id) != types.end())
                    throw std::runtime_error("Empty, oversized or duplicate NPC type ID: " + id);
                types.push_back(id);
                NpcDefinition appearance;
                const auto sampling = definition.value("sampling", std::string("nearest"));
                if (sampling != "nearest" && sampling != "area")
                    throw std::runtime_error("NPC sampling must be nearest or area");
                appearance.areaSampling = sampling == "area";
                appearance.width = number(definition.at("width"));
                appearance.height = number(definition.at("height"));
                if (definition.contains("alpha_cutoff")) {
                    const int cutoff = integer(definition.at("alpha_cutoff"));
                    if (cutoff < 0 || cutoff > 254)
                        throw std::runtime_error("NPC alpha_cutoff must be 0..254");
                    appearance.alphaCutoff = static_cast<unsigned char>(cutoff);
                }
                if (definition.contains("attachments")) {
                    const auto& anchors = definition.at("attachments");
                    if (!anchors.is_object())
                        throw std::runtime_error("NPC attachments must be an object");
                    const std::array<const char*, 4> sides{"front", "back", "left", "right"};
                    for (const auto* kind : {"release", "torso"}) {
                        if (!anchors.contains(kind))
                            continue;
                        auto& values = std::string(kind) == "release" ? appearance.attachments.release
                                                                      : appearance.attachments.torso;
                        for (std::size_t i = 0; i < 4; ++i) {
                            const auto& point = anchors.at(kind).at(sides[i]);
                            if (!point.is_array() || point.size() != 2)
                                throw std::runtime_error("NPC attachment requires [x,y]");
                            values[i] = {number(point[0]), number(point[1])};
                            if (values[i].x < 0 || values[i].x > 1 || values[i].y < 0 || values[i].y > 1)
                                throw std::runtime_error("NPC attachment outside normalized canvas");
                        }
                    }
                }
                loadAnimations(definition, path.parent_path(), appearance);
                if (appearance.width <= 0 || appearance.width > .5f || appearance.height <= 0 ||
                    appearance.height > 1)
                    throw std::runtime_error("Invalid NPC dimensions");
                validateNpcTimings(npcTimings(appearance));
                for (std::size_t i = 0; i < npcAnimationCount; ++i) {
                    const auto& clip = appearance.animations[i];
                    const float maxWidth =
                        i == std::size_t(NpcAnimation::Death) || i == std::size_t(NpcAnimation::Prone) ? 1.f
                        : i == std::size_t(NpcAnimation::RangedAttack)                                 ? .6f
                                                                                                       : .5f;
                    if (clip.width > maxWidth)
                        throw std::runtime_error("Invalid NPC clip canvas width");
                }
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
            if (placement.contains("ai")) {
                const auto& ai = placement.at("ai");
                if (!ai.is_object())
                    throw std::runtime_error("NPC ai must be an object");
                auto& config = instances.back().ai;
                const auto mode = ai.at("routine").get<std::string>();
                if (mode == "guard")
                    config.routine = NpcRoutine::Guard;
                else if (mode == "patrol")
                    config.routine = NpcRoutine::Patrol;
                else if (mode == "wander")
                    config.routine = NpcRoutine::Wander;
                else
                    throw std::runtime_error("Unknown NPC AI routine");
                if (ai.contains("waypoints")) {
                    const auto& points = ai.at("waypoints");
                    if (!points.is_array() || points.size() > config.waypoints.size())
                        throw std::runtime_error("NPC patrol exceeds eight waypoints");
                    for (const auto& point : points)
                        config.waypoints[config.waypointCount++] = cell(point);
                }
            }
        }
        return {NpcWorld(map, spawn, std::move(types), std::move(instances)), std::move(appearances),
                definitionHash};
    } catch (const std::exception& e) {
        throw std::runtime_error("Loading NPC level " + file.string() + ": " + e.what());
    }
}
NpcTimings npcTimings(const NpcDefinition& a) {
    NpcTimings result{};
    if (!a.animations[0].timing.count)
        result[0] = {{{1, 0, 0, 0}}, 1};
    else
        for (std::size_t i = 0; i < npcAnimationCount; ++i)
            result[i] = a.animations[i].timing;
    return result;
}
NpcDefinitions loadNpcDefinitions(const std::filesystem::path& file) {
    const Map placeholder({"###", "#.#", "###"});
    auto parsed = parseNpcMetadata(file, placeholder, {1, 1}, false);
    NpcDefinitions out;
    out.definitionFingerprint = parsed.definitionFingerprint;
    for (std::size_t i = 0; i < parsed.world.typeCount(); ++i)
        out.types.push_back(parsed.world.typeId({std::uint16_t(i)}));
    for (const auto& appearance : parsed.definitions) {
        out.timings.push_back(npcTimings(appearance));
        out.attachments.push_back(appearance.attachments);
    }
    return out;
}
LoadedNpcMetadata loadNpcMetadata(const std::filesystem::path& file, const Map& map, Cell spawn) {
    auto parsed = parseNpcMetadata(file, map, spawn);
    std::vector<NpcTimings> timings;
    timings.reserve(parsed.definitions.size());
    std::vector<NpcAttachments> attachments;
    for (const auto& a : parsed.definitions) {
        timings.push_back(npcTimings(a));
        attachments.push_back(a.attachments);
    }
    return {std::move(parsed.world), std::move(timings), std::move(attachments),
            parsed.definitionFingerprint};
}
} // namespace blobber

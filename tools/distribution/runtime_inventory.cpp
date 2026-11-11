#include "runtime_inventory.hpp"
#include "assets/blueprint_loader.hpp"
#include "assets/dialogue.hpp"
#include "assets/fingerprint.hpp"
#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "assets/shops.hpp"
#include "assets/world_assets.hpp"
#include "audio/bank.hpp"
#include "audio/music.hpp"
#include "platform/file_io.hpp"
#include "ui/art_names.hpp"
#include "ui/equipment_art.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>

namespace blobber::distribution {
namespace {
void require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}
void regularFile(const fs::path& path) {
    checkedRoot(path);
    require(fs::is_regular_file(path), "Missing required runtime file: " + path.string());
}
std::vector<fs::path> yamlFiles(const fs::path& directory) {
    checkedRoot(directory);
    require(fs::is_directory(directory), "Missing runtime directory: " + directory.string());
    std::vector<fs::path> paths;
    for (const auto& entry : fs::directory_iterator(directory)) {
        const auto ext = entry.path().extension();
        if (ext == ".yaml" || ext == ".yml") {
            regularFile(entry.path());
            paths.push_back(entry.path());
        }
    }
    std::sort(paths.begin(), paths.end());
    require(!paths.empty(), "No runtime YAML in " + directory.string());
    return paths;
}
std::set<std::string> tags(const Json& manifest, const char* key, const std::set<std::string>& allowed) {
    std::set<std::string> result;
    const auto& values = manifest.at(key);
    require(values.is_array(), std::string("Expected manifest array: ") + key);
    for (const auto& value : values) {
        const auto tag = value.get<std::string>();
        require(allowed.count(tag) && result.insert(tag).second, "Unknown/duplicate manifest tag: " + tag);
    }
    return result;
}
Json manifestAt(const fs::path& path) {
    auto j = readJson(path);
    require(j.is_object() && j.at("version") == 1, "Unsupported runtime manifest version");
    for (auto it = j.begin(); it != j.end(); ++it)
        require(it.key() == "version" || it.key() == "files" || it.key() == "selectors" ||
                    it.key() == "validate",
                "Unknown runtime manifest field: " + it.key());
    require(j.at("files").is_array(), "Expected manifest files array");
    return j;
}
std::set<std::string> selectors(const Json& j) {
    return tags(j, "selectors", {"ui-icons", "npcs", "dialogue", "shops", "audio"});
}
std::set<std::string> validations(const Json& j) {
    return tags(j, "validate", {"rules", "blueprints", "world", "atlases"});
}
void atlasHeader(const fs::path& path, unsigned height) {
    regularFile(path);
    std::ifstream input(path, std::ios::binary);
    std::array<unsigned char, 14> header{};
    input.read(reinterpret_cast<char*>(header.data()), header.size());
    auto n = [&](unsigned i) {
        return (unsigned(header[i]) << 24) | (unsigned(header[i + 1]) << 16) |
               (unsigned(header[i + 2]) << 8) | header[i + 3];
    };
    require(input && n(0) == 0x716f6966 && n(4) == 2048 && n(8) == height && header[12] == 4 &&
                fs::file_size(path) <= std::uint64_t(2048) * height * 5 + 22,
            "Invalid prepared QOI atlas: " + path.string());
}
} // namespace

bool within(const fs::path& root, const fs::path& path) {
    auto r = root.begin(), p = path.begin();
    for (; r != root.end() && p != path.end() && *r == *p; ++r, ++p) {
    }
    return r == root.end();
}
fs::path checkedRoot(const fs::path& path) {
    require(!path.empty(), "Empty filesystem root");
    const auto absolute = fs::absolute(path).lexically_normal();
    fs::path current;
    for (const auto& part : absolute) {
        current /= part;
        const auto status = fs::symlink_status(current);
        require(!disk::reparsePoint(current), "Refusing symlink/reparse point: " + current.u8string());
        require(!fs::exists(status) || fs::is_directory(status) || fs::is_regular_file(status),
                "Refusing special file: " + current.string());
    }
    return absolute;
}
fs::path relativePath(const std::string& text) {
    require(!text.empty() && text.find_first_of("\\:") == std::string::npos &&
                std::none_of(text.begin(), text.end(), [](unsigned char c) { return c < 32; }),
            "Unsafe relative runtime path: " + text);
    const fs::path path(text);
    require(!path.is_absolute() && !path.has_root_path(), "Absolute runtime path: " + text);
    for (const auto& part : path)
        require(!part.empty() && part != "." && part != "..", "Unsafe runtime path: " + text);
    return path;
}
void checkTree(const fs::path& path) {
    checkedRoot(path);
    if (!fs::exists(path))
        return;
    if (fs::is_directory(path))
        for (const auto& entry : fs::recursive_directory_iterator(path))
            checkedRoot(entry.path());
}
Json readJson(const fs::path& path) {
    regularFile(path);
    require(fs::file_size(path) <= 8u * 1024 * 1024, "JSON exceeds packaging limit: " + path.string());
    std::ifstream in(path);
    return Json::parse(in);
}

Inventory resolve(const Settings& settings, bool includePrepared) {
    const auto manifest = manifestAt(settings.manifest);
    const auto selected = selectors(manifest);
    (void)validations(manifest);
    const auto assets = checkedRoot(settings.source);
    const auto prepared = checkedRoot(settings.prepared);
    const auto authored = checkedRoot(settings.manifest).parent_path();
    Inventory result;
    std::map<fs::path, File> files;
    auto add = [&](const std::string& root, const std::string& name, const std::string& destination,
                   const std::string& reason) {
        require(root == "assets" || root == "prepared" || root == "manifest", "Unknown input root: " + root);
        const auto relative = relativePath(name);
        const auto dest = relativePath(destination);
        require(dest.begin()->string() == "assets" || dest.begin()->string() == "notices",
                "Runtime destination must be under assets/ or notices/: " + destination);
        // Validate even skipped entries so raw and dist cannot disagree about path policy.
        if (root == "prepared" && !includePrepared)
            return;
        const auto source = (root == "assets" ? assets : root == "prepared" ? prepared : authored) / relative;
        regularFile(source);
        const File file{source, dest, root, name, reason};
        const auto old = files.find(dest);
        require(old == files.end() || old->second.source == source,
                "Conflicting runtime destination: " + destination);
        files.emplace(dest, file);
    };
    auto asset = [&](const fs::path& path, const std::string& reason) {
        const auto name = path.generic_string();
        add("assets", name, "assets/" + name, reason);
    };
    for (const auto& file : manifest.at("files")) {
#ifdef _WIN32
        if (file.at("path").get<std::string>().find("notices/gcc-runtime-") == 0)
            continue;
#endif
        for (auto it = file.begin(); it != file.end(); ++it)
            require(it.key() == "root" || it.key() == "path" || it.key() == "destination",
                    "Unknown manifest file field: " + it.key());
        const auto root = file.at("root").get<std::string>();
        const auto path = file.at("path").get<std::string>();
        const auto fallback = (root == "prepared" ? "assets/prepared/" : "assets/") + path;
        add(root, path, file.value("destination", fallback), "explicit");
    }
    if (selected.count("ui-icons")) {
        for (std::size_t i = 4; i < interfaceArtNames.size(); ++i)
            asset(std::string("ui/") + interfaceArtNames[i] + ".png", "UI region");
        for (const auto* id : equipmentIconIds)
            asset(std::string("ui/items/") + id + ".png", "equipment icon");
    }
    if (selected.count("npcs")) {
        asset("npcs/level.json", "NPC level");
        const auto level = readJson(assets / "npcs/level.json");
        for (const auto& entry : level.at("definitions"))
            asset(fs::path("npcs") / relativePath(entry.get<std::string>()), "NPC definition");
    }
    if (selected.count("dialogue")) {
        for (const auto& file : yamlFiles(assets / "dialogue"))
            asset(file.lexically_relative(assets), "dialogue YAML");
        // Check the tree before the loader canonicalizes references through symlinks.
        checkTree(assets / "ui");
        checkTree(assets / "audio");
        const auto dialogue = loadDialogue(assets / "dialogue", assets / "audio", assets / "ui");
        for (const auto& file : dialogue.audioFiles)
            asset(fs::path(file).lexically_relative(assets), "dialogue audio");
        for (const auto& file : dialogue.overlayFiles)
            asset(fs::path(file).lexically_relative(assets), "dialogue overlay");
    }
    if (selected.count("shops"))
        for (const auto& file : yamlFiles(assets / "shops"))
            asset(file.lexically_relative(assets), "shop YAML");
    if (selected.count("audio")) {
        asset("audio/bank.json", "audio bank");
        const auto bank = readJson(assets / "audio/bank.json");
        // Keep original variants as well as effective overrides: runtime has base fallbacks.
        for (const auto& cue : bank.at("cues"))
            for (const auto& v : cue.at("variants"))
                asset(fs::path("audio") / relativePath(v.at("path").get<std::string>()),
                      "base audio variant");
        if (fs::exists(assets / "audio/overrides.json")) {
            asset("audio/overrides.json", "audio overrides");
            // Reject missing override files even if the runtime would fall back to a base.
            const auto overrides = readJson(assets / "audio/overrides.json");
            for (const auto& cue : overrides.at("cues")) {
                if (cue.contains("variants"))
                    for (const auto& v : cue.at("variants"))
                        asset(fs::path("audio") / relativePath(v.at("path").get<std::string>()),
                              "override variant");
                if (cue.contains("variant_overrides"))
                    for (const auto& v : cue.at("variant_overrides"))
                        if (v.contains("path"))
                            asset(fs::path("audio") / relativePath(v.at("path").get<std::string>()),
                                  "variant override");
            }
        }
        const auto effective = audio::loadBank(assets / "audio");
        require(effective.warning.empty(), effective.warning);
        for (const auto& cue : effective.cues)
            for (const auto& v : cue.variants)
                asset(fs::path("audio") / relativePath(v.path), "effective audio variant");
        for (const auto* directory : {"audio/music", "audio/music/sanctuary", "audio/music/mainmenu"}) {
            const fs::path root(directory);
            asset(root / "playlist.json", "music playlist");
            const auto playlist = readJson(assets / root / "playlist.json");
            for (const auto& track : playlist.at("tracks"))
                asset(root / relativePath(track.at("path").get<std::string>()), "music track");
        }
    }
    fs::path previous;
    for (const auto& pair : files) {
        require(previous.empty() || !within(previous, pair.first), "File/directory destination conflict");
        result.files.push_back(pair.second);
        previous = pair.first;
    }
    result.identities = validateContent(settings, assets, prepared, includePrepared);
    return result;
}

Json validateContent(const Settings& settings, const fs::path& assets, const fs::path& prepared,
                     bool includePrepared) {
    const auto manifest = manifestAt(settings.manifest);
    const auto selected = selectors(manifest), checks = validations(manifest);
    Json identities = Json::object();
    std::optional<ShopCatalog> shops;
    if (checks.count("rules") || selected.count("shops")) {
        const auto rulesPath = assets / "rules/catalog.json";
        regularFile(rulesPath);
        const auto rules = loadRules(rulesPath);
        identities["rules"] = fingerprintFile(rulesPath);
        if (selected.count("shops"))
            shops = loadShops(assets / "shops", rules);
    }
    if (selected.count("npcs"))
        identities["npc_definitions"] = loadNpcDefinitions(assets / "npcs/level.json").definitionFingerprint;
    if (selected.count("dialogue")) {
        const auto dialogue = loadDialogue(assets / "dialogue", assets / "audio", assets / "ui");
        if (shops)
            validateShops(dialogue, *shops);
        for (const auto& f : dialogue.audioFiles)
            regularFile(f);
        for (const auto& f : dialogue.overlayFiles)
            regularFile(f);
    }
    if (selected.count("audio")) {
        const auto bank = audio::loadBank(assets / "audio");
        require(bank.warning.empty(), bank.warning);
        for (const auto& cue : bank.cues)
            for (const auto& v : cue.variants)
                regularFile(assets / "audio" / relativePath(v.path));
        for (const auto* name : {"audio/music", "audio/music/sanctuary", "audio/music/mainmenu"}) {
            regularFile(assets / name / "playlist.json");
            std::string warning;
            (void)audio::loadMusicTracks(assets / name, warning);
            require(warning.empty(), warning);
        }
    }
    if (checks.count("blueprints")) {
        (void)blueprints::load(assets / "blueprints/underkeep.json");
        identities["blueprints"] = fingerprintFile(assets / "blueprints/underkeep.json");
        if (includePrepared)
            blueprints::checkPrepared(assets, prepared / "world");
    }
    if (includePrepared && checks.count("world")) {
        const auto world = world_assets::load(prepared / "world");
        if (identities.contains("npc_definitions"))
            require(world.definitions == identities.at("npc_definitions").get<std::uint64_t>(),
                    "Prepared world NPC definitions differ from shipping JSON");
        identities["world"] = world.fingerprint;
        identities["world_payload"] = world.payloadChecksum;
    }
    if (includePrepared && checks.count("atlases")) {
        atlasHeader(prepared / "creator.qoi", 2048);
        atlasHeader(prepared / "portraits-small.qoi", 1024);
        atlasHeader(prepared / "portraits-large.qoi", 4096);
    }
    return identities;
}
} // namespace blobber::distribution

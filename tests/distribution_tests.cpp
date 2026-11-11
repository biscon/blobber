#include "assets/fingerprint.hpp"
#include "assets/npc_metadata.hpp"
#include "distribution/runtime_inventory.hpp"
#include "support/blueprint_fixtures.hpp"
#include "support/check.hpp"
#include "support/temp_directory.hpp"
#include "support/world_fixtures.hpp"
#include "ui/art_names.hpp"
#include "ui/equipment_art.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <set>
#ifndef _WIN32
#include <sys/stat.h>
#endif

namespace d = blobber::distribution;
namespace fs = std::filesystem;
using J = d::Json;
using namespace blobber;
namespace {
void write(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << text;
    out.close();
    check(bool(out), "Write owned fixture");
}
std::string read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}
struct Fixture {
    fs::path directory;
    d::Settings s;
    J manifest{{"version", 1}, {"files", J::array()}, {"selectors", J::array()}, {"validate", J::array()}};
    Fixture() {
        directory = testDirectory();
        s.manifest = directory / "packaging/runtime.json";
        s.source = directory / "source assets";
        s.build = directory / "managed build";
        s.prepared = s.build / "assets/prepared";
        s.configuration = "Release";
        s.executable = s.build / "blobber";
        write(s.build / "CMakeCache.txt", "fixture build\n");
        write(s.executable, "fixture executable\n");
#ifndef _WIN32
        fs::permissions(s.executable, fs::perms::owner_read | fs::perms::owner_exec);
#endif
        fs::create_directories(s.source);
        fs::create_directories(s.prepared);
        save();
    }
    ~Fixture() {
        std::error_code error;
        fs::remove_all(directory, error);
    }
    void save() { write(s.manifest, manifest.dump(2)); }
    void asset(const std::string& name, const std::string& bytes = "fixture") {
        write(s.source / name, bytes);
    }
    void fixed(const std::string& name, const std::string& bytes = "fixture") {
        asset(name, bytes);
        manifest["files"].push_back({{"root", "assets"}, {"path", name}});
    }
    fs::path stage() const { return s.build / "dist-stage/Release"; }
    fs::path report() const { return s.build / "dist-stage/Release.inventory.json"; }
};
std::set<std::string> paths(const d::Inventory& inventory) {
    std::set<std::string> names;
    for (const auto& file : inventory.files)
        names.insert(file.destination.generic_string());
    return names;
}
J npc() {
    return {{"id", "fixture"},
            {"width", .4},
            {"height", .7},
            {"source_sheets", {{"idle", "source/not-shipped.png"}}},
            {"views",
             {{"front", "frames/not-shipped.png"},
              {"back", "frames/not-shipped.png"},
              {"left", "frames/not-shipped.png"},
              {"right", "frames/not-shipped.png"}}}};
}
J conversation(const std::string& resident = "fixture.host") {
    const J beat{{"speaker", "host"}, {"text", "Welcome."}};
    return {{"version", 2},
            {"primary", "host"},
            {"participants", {{"host", {{"resident", resident}, {"name", "Host"}}}}},
            {"greeting", {{"beats", J::array({beat})}}},
            {"introduction", {{"beats", J::array({beat})}}},
            {"topics", J::array({{{"id", "farewell"}, {"label", "Farewell"}, {"kind", "farewell"}}})}};
}
void npcFiles(Fixture& f) {
    f.manifest["selectors"].push_back("npcs");
    f.asset("npcs/level.json", J{{"definitions", {"fixture.json"}}, {"instances", J::array()}}.dump());
    f.asset("npcs/fixture.json", npc().dump(2));
}
void selection() {
    Fixture f;
    npcFiles(f);
    f.manifest["selectors"].push_back("ui-icons");
    for (std::size_t i = 4; i < interfaceArtNames.size(); ++i)
        f.asset(std::string("ui/") + interfaceArtNames[i] + ".png");
    for (const auto* id : equipmentIconIds)
        f.asset(std::string("ui/items/") + id + ".png");
    f.fixed("blueprints/underkeep.json");
    f.fixed("ui/repeated.png");
    f.manifest["files"].push_back(f.manifest["files"].back());
    f.asset("ui/repeated.prompt.json", "authoring");
    f.asset("npcs/source/not-shipped.png", "source art");
    f.asset("npcs/frames/not-shipped.png", "bake input");
    write(f.s.prepared / "fixture.bin", "prepared output");
    f.manifest["files"].push_back({{"root", "prepared"}, {"path", "fixture.bin"}});
    f.save();
    const auto inventory = d::resolve(f.s, false);
    const auto names = paths(inventory);
    check(names.count("assets/blueprints/underkeep.json") && names.count("assets/npcs/fixture.json") &&
              names.count("assets/ui/goblin.png"),
          "Runtime files selected from fixed and reference entries");
    check(!names.count("assets/npcs/frames/not-shipped.png") &&
              !names.count("assets/ui/repeated.prompt.json") && !names.count("assets/ui/alden.png"),
          "Authoring artwork and prepared-only portraits excluded");
    check(names.size() == inventory.files.size(), "Identical entries deduplicate");
    check(!names.count("assets/prepared/fixture.bin") &&
              paths(d::resolve(f.s, true)).count("assets/prepared/fixture.bin"),
          "Prepared selection is explicit");
    d::synchronizeRaw(f.s);
    const auto staged = f.s.build / "assets/npcs/fixture.json";
    check(read(staged) == read(f.s.source / "npcs/fixture.json"), "NPC JSON copied byte for byte");
    check(loadNpcDefinitions(f.s.build / "assets/npcs/level.json").definitionFingerprint ==
              loadNpcDefinitions(f.s.source / "npcs/level.json").definitionFingerprint,
          "NPC identity preserved without referenced source artwork");
    check(!fs::exists(f.s.build / "assets/npcs/frames"), "Source frame directory not staged");
}
void dialogueFiles() {
    Fixture f;
    f.manifest["selectors"] = {"dialogue"};
    auto script = conversation();
    script["greeting"]["beats"][0]["audio"] = "dialogue/reaction.wav";
    script["greeting"]["beats"][0]["overlay_asset"] = "dialogue/scroll.png";
    f.asset("dialogue/host.yaml", script.dump());
    f.asset("dialogue/README.md");
    f.asset("audio/dialogue/reaction.wav");
    f.asset("ui/dialogue/scroll.png");
    f.save();
    auto names = paths(d::resolve(f.s, false));
    check(names.count("assets/audio/dialogue/reaction.wav") && names.count("assets/ui/dialogue/scroll.png") &&
              !names.count("assets/dialogue/README.md"),
          "Existing dialogue loader resolves audio and overlays");
    fs::remove(f.s.source / "audio/dialogue/reaction.wav");
    rejects([&] { d::resolve(f.s, false); }, "Missing dialogue audio fails staging");
    script["greeting"]["beats"][0].erase("audio");
    script["greeting"]["beats"][0].erase("overlay_asset");
    f.asset("dialogue/host.yaml", script.dump());
    f.asset("dialogue/new.yml", conversation("fixture.new").dump());
    names = paths(d::resolve(f.s, false));
    check(names.count("assets/dialogue/new.yml") && !names.count("assets/ui/dialogue/scroll.png"),
          "Added scripts and removed references are discovered without CMake inventory changes");
    fs::remove(f.s.source / "dialogue/new.yml");
    check(!paths(d::resolve(f.s, false)).count("assets/dialogue/new.yml"), "Deleted script disappears");
    f.asset("dialogue/host.yaml", "malformed: [");
    rejects([&] { d::resolve(f.s, false); }, "Malformed YAML rejects");
}
void audioFiles() {
    Fixture f;
    f.manifest["selectors"] = {"audio"};
    J bank{{"cues", {{"fixture", {{"variants", J::array({{{"id", "one"}, {"path", "base.wav"}}})}}}}}};
    f.asset("audio/bank.json", bank.dump());
    f.asset("audio/base.wav");
    f.asset("audio/replacement.wav");
    f.asset("audio/unused.wav");
    f.asset(
        "audio/overrides.json",
        J{{"cues", {{"fixture", {{"variant_overrides", {{"one", {{"path", "replacement.wav"}}}}}}}}}}.dump());
    for (const auto* root : {"audio/music", "audio/music/sanctuary", "audio/music/mainmenu"}) {
        f.asset(
            std::string(root) + "/playlist.json",
            J{{"tracks",
               J::array({{{"id", "one"}, {"path", "track.mp3"}, {"title", "Song"}, {"artist", "Fixture"}}})}}
                .dump());
        f.asset(std::string(root) + "/track.mp3");
    }
    f.save();
    const auto names = paths(d::resolve(f.s, false));
    check(names.count("assets/audio/base.wav") && names.count("assets/audio/replacement.wav") &&
              !names.count("assets/audio/unused.wav") && names.count("assets/audio/music/mainmenu/track.mp3"),
          "Base fallback, effective overrides and every playlist selected; unused audio excluded");
    fs::remove(f.s.source / "audio/replacement.wav");
    rejects([&] { d::resolve(f.s, false); }, "Missing override does not silently pass using base fallback");
    fs::remove(f.s.source / "audio/overrides.json");
    check(!paths(d::resolve(f.s, false)).count("assets/audio/replacement.wav"),
          "Absent optional override is supported");
    bank["cues"]["fixture"]["gain"] = 2;
    f.asset("audio/bank.json", bank.dump());
    rejects([&] { d::resolve(f.s, false); }, "Disabled cue warnings reject shipping inventory");
    bank["cues"]["fixture"].erase("gain");
    f.asset("audio/bank.json", bank.dump());
    fs::remove(f.s.source / "audio/music/sanctuary/track.mp3");
    rejects([&] { d::resolve(f.s, false); }, "Missing playlist track rejects");
}
void shopFiles() {
    Fixture f;
    f.manifest["selectors"] = {"shops"};
    const J rules{{"schema_version", 2},
                  {"attacks", J::array()},
                  {"monsters", J::array()},
                  {"races", J::array()},
                  {"classes", J::array()},
                  {"preset_party", nullptr},
                  {"items", J::array({{{"id", "token"},
                                       {"name", "Token"},
                                       {"description", "A token."},
                                       {"kind", "gear"},
                                       {"weight", 1}}})}};
    f.fixed("rules/catalog.json", rules.dump());
    auto shop = [](const char* id) {
        return J{{"version", 1},
                 {"id", id},
                 {"restock_days", 1},
                 {"buyback_hours", 1},
                 {"buyback_markup_percent", 50},
                 {"prices", {{"token", {{"buy", 2}, {"sell", 1}}}}},
                 {"stock", J::array({{{"item", "token"}, {"quantity", 1}}})}};
    };
    f.asset("shops/one.yaml", shop("one").dump());
    f.save();
    d::synchronizeRaw(f.s);
    f.asset("shops/two.yml", shop("two").dump());
    fs::remove(f.s.source / "shops/one.yaml");
    d::synchronizeRaw(f.s);
    check(fs::exists(f.s.build / "assets/shops/two.yml") && !fs::exists(f.s.build / "assets/shops/one.yaml"),
          "Added and removed shop YAML synchronize through the same inventory");
    f.asset("shops/two.yml", shop("two").dump());
    auto bad = shop("two");
    bad["stock"][0]["item"] = "missing";
    f.asset("shops/two.yml", bad.dump());
    rejects([&] { d::resolve(f.s, false); }, "Unknown shop item fails real loader validation");
}
void unsafeInputs() {
    Fixture f;
    f.fixed("ui/keep.png");
    f.save();
    for (const auto* path :
         {"../escape", "/absolute", "ui/../escape", "ui/./keep.png", "C:\\escape", "ui/"}) {
        auto bad = f.manifest;
        bad["files"][0]["destination"] = path;
        write(f.s.manifest, bad.dump());
        rejects([&] { d::resolve(f.s, false); }, "Unsafe destination rejected");
        bad = f.manifest;
        bad["files"][0]["path"] = path;
        write(f.s.manifest, bad.dump());
        rejects([&] { d::resolve(f.s, false); }, "Unsafe input path rejected");
    }
    f.save();
    fs::remove(f.s.source / "ui/keep.png");
    fs::create_symlink(f.s.executable, f.s.source / "ui/keep.png");
    rejects([&] { d::resolve(f.s, false); }, "Selected source symlink rejected");
    fs::remove(f.s.source / "ui/keep.png");
#ifndef _WIN32
    check(mkfifo((f.s.source / "ui/keep.png").c_str(), 0600) == 0, "Create owned FIFO fixture");
    rejects([&] { d::resolve(f.s, false); }, "Selected special file rejected before opening");
    fs::remove(f.s.source / "ui/keep.png");
#endif
    f.asset("ui/keep.png");
    f.fixed("ui/other.png");
    f.manifest["files"].back()["destination"] = "assets/ui/keep.png";
    f.save();
    rejects([&] { d::resolve(f.s, false); }, "Conflicting duplicate destination rejected");
}
void publicationChecks() {
    Fixture f;
    f.fixed("blueprints/underkeep.json", test::blueprintJson().dump());
    f.manifest["validate"] = {"blueprints"};
    write(f.s.prepared / "world/sources.json",
          J{{"blueprints/underkeep.json", fingerprintFile(f.s.source / "blueprints/underkeep.json")}}.dump());
    f.save();
    check(d::resolve(f.s, true).identities.contains("blueprints"),
          "Real blueprint loads with matching provenance");
    write(f.s.prepared / "world/sources.json", "{}");
    rejects([&] { d::resolve(f.s, true); }, "Mismatched blueprint provenance rejects");
    f.manifest["validate"] = {"world"};
    test::WorldFixture world;
    f.s.prepared = f.directory / "tiny publication";
    fs::create_directories(f.s.prepared / "world");
    fs::copy(world.directory, f.s.prepared / "world",
             fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    f.save();
    check(d::resolve(f.s, true).identities.contains("world"),
          "Tiny owned world publication validates headlessly");
    npcFiles(f);
    f.save();
    rejects([&] { d::resolve(f.s, true); }, "Publication and NPC definition mismatch rejects");
}
void rawStaging() {
    Fixture f;
    f.fixed("ui/keep.png", "one");
    f.save();
    write(f.s.build / "assets/ui/revisions/old.png", "stale broad copy");
    write(f.s.prepared / "keep.bin", "prepared");
    write(f.s.build / "assets/user-owned.txt", "unrelated");
    d::synchronizeRaw(f.s);
    const auto keep = f.s.build / "assets/ui/keep.png";
    const auto timestamp = fs::last_write_time(keep);
    d::synchronizeRaw(f.s);
    check(fs::last_write_time(keep) == timestamp && !fs::exists(f.s.build / "assets/ui/revisions"),
          "Raw sync removes stale broad copies and preserves unchanged timestamps");
    check(read(f.s.prepared / "keep.bin") == "prepared" &&
              read(f.s.build / "assets/user-owned.txt") == "unrelated",
          "Prepared and unrelated files remain untouched");
    f.asset("ui/keep.png", "two");
    fs::last_write_time(f.s.source / "ui/keep.png", timestamp + std::chrono::seconds(1));
    d::synchronizeRaw(f.s);
    check(read(keep) == "two", "Same-size source edit with changed timestamp replaces raw file");
    f.manifest["files"] = J::array();
    f.save();
    d::synchronizeRaw(f.s);
    check(!fs::exists(keep), "Removed inventory entry retires staged file");
    f.fixed("ui/type", "file");
    f.save();
    d::synchronizeRaw(f.s);
    fs::remove(f.s.source / "ui/type");
    f.manifest["files"] = J::array();
    f.fixed("ui/type/child", "child");
    f.save();
    d::synchronizeRaw(f.s);
    check(read(f.s.build / "assets/ui/type/child") == "child", "Stale file can become a directory");
    fs::remove_all(f.s.source / "ui/type");
    f.manifest["files"] = J::array();
    f.fixed("ui/type", "replacement");
    f.save();
    d::synchronizeRaw(f.s);
    check(read(f.s.build / "assets/ui/type") == "replacement", "Stale directory can become a file");
    f.s.multiConfig = true;
    f.s.configuration = "Debug";
    f.fixed("ui/tool.png");
    f.save();
    d::synchronizeRaw(f.s);
    check(fs::exists(f.s.build / "Debug/assets/ui/tool.png"),
          "Explicit multi-config raw tool staging remains available");
}
void distributionStaging() {
    Fixture f;
    npcFiles(f);
    f.fixed("ui/keep.png");
    f.asset("ui/old.prompt.json");
    f.save();
    d::stageDistribution(f.s);
    const auto report = d::verifyDistribution(f.s);
#ifdef _WIN32
    const char* gameName = "blobber.exe";
#else
    const char* gameName = "blobber";
#endif
    check(read(f.stage() / gameName) == read(f.s.executable), "Executable copied");
    check(report["identities"].contains("npc_definitions") && !report["excluded"].empty(),
          "Report records identities, excluded files and sizes");
    const auto previousReport = read(f.report());
    // Force a failure after copying: source rules validate, but are deliberately not inventoried.
    f.asset("rules/catalog.json",
            J{{"schema_version", 1}, {"attacks", J::array()}, {"monsters", J::array()}}.dump());
    f.manifest["validate"] = {"rules"};
    f.save();
    rejects([&] { d::stageDistribution(f.s); }, "Incomplete candidate fails staged loader validation");
    check(read(f.report()) == previousReport && fs::exists(f.stage() / "assets/ui/keep.png") &&
              !fs::exists(f.s.build / "dist-stage/.candidate"),
          "Failure preserves prior stage/report and cleans candidate");
    f.manifest["validate"] = J::array();
    f.manifest["files"] = J::array();
    f.save();
    d::stageDistribution(f.s);
    check(!fs::exists(f.stage() / "assets/ui/keep.png"),
          "Fresh distribution replacement removes stale files");
    check(d::verifyDistribution(f.s).at("stage_id") != report.at("stage_id"),
          "New stage has its own completion identity");
    write(f.stage() / "unexpected.txt", "not in manifest");
    rejects([&] { d::verifyDistribution(f.s); }, "Unexpected payload file fails verification");
    fs::remove(f.stage() / "unexpected.txt");
    fs::remove(f.stage() / "assets/npcs/fixture.json");
    rejects([&] { d::verifyDistribution(f.s); }, "Missing staged file fails verification");
}
void outputSafety() {
    Fixture f;
    f.fixed("ui/keep.png");
    f.save();
    const auto root = f.s.build / "dist-stage";
    for (const auto* config : {"Debug", "", "RelWithDebInfo"}) {
        f.s.configuration = config;
        rejects([&] { d::stageDistribution(f.s); }, "Unsupported configuration rejected before writes");
        check(!fs::exists(root), "Configuration failure creates no destination");
    }
    f.s.configuration = "Release";
    f.s.multiConfig = true;
    rejects([&] { d::stageDistribution(f.s); }, "Multi-config dist rejected");
    f.s.multiConfig = false;
    write(root / "foreign.txt", "foreign");
    rejects([&] { d::stageDistribution(f.s); }, "Unowned nonempty destination rejected");
    check(read(root / "foreign.txt") == "foreign", "Unowned destination preserved");
    fs::remove_all(root);
    fs::create_directory_symlink(f.s.source, root);
    rejects([&] { d::stageDistribution(f.s); }, "Symlink distribution output rejected");
    fs::remove(root);
    fs::create_directories(f.s.build / "assets");
    fs::create_directory_symlink(f.s.source / "ui", f.s.build / "assets/ui");
    rejects([&] { d::synchronizeRaw(f.s); }, "Raw output link cannot cause source deletion");
    check(read(f.s.source / "ui/keep.png") == "fixture", "Source preserved through rejected output link");
    fs::remove(f.s.build / "assets/ui");
    auto overlapping = f.s;
    overlapping.source = f.s.build / "assets";
    write(overlapping.source / "ui/keep.png", "input");
    rejects([&] { d::synchronizeRaw(overlapping); }, "Raw input/output overlap rejected");
    check(read(overlapping.source / "ui/keep.png") == "input", "Overlapping source preserved");
}
} // namespace
int main(int argc, char** argv) {
    try {
        check(argc == 2, "Select inventory or staging suite");
        if (std::string(argv[1]) == "inventory") {
            selection();
            dialogueFiles();
            audioFiles();
            shopFiles();
            unsafeInputs();
            publicationChecks();
        } else {
            check(std::string(argv[1]) == "staging", "Unknown test suite");
            rawStaging();
            distributionStaging();
            outputSafety();
        }
        std::cout << "Distribution " << argv[1] << " checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

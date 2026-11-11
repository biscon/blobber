#include "runtime_inventory.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

namespace blobber::distribution {
namespace {
// These are the previous CMake raw-copy roots plus the omitted blueprint root.
// They are build outputs, never source directories; prepared/ is separately owned.
const std::set<std::string> rawDirectories{"audio", "blueprints", "dialogue", "effects",  "fonts",
                                           "npcs",  "rules",      "shops",    "textures", "ui"};
void require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}
fs::path buildRoot(const Settings& s) {
    const auto build = checkedRoot(s.build);
    require(fs::is_regular_file(checkedRoot(build / "CMakeCache.txt")),
            "Staging requires an existing configured build root: " + build.string());
    require(build != build.root_path(), "Cannot stage in filesystem root");
    return build;
}
void separate(const fs::path& output, const fs::path& input) {
    require(!within(output, input) && !within(input, output),
            "Staging output overlaps input: " + output.string() + " / " + input.string());
}
void writeJson(const fs::path& path, const Json& j) {
    checkedRoot(path);
    std::ofstream out(path);
    out << j.dump(2) << '\n';
    out.close();
    require(bool(out), "Cannot write staging report: " + path.string());
}
void copy(const fs::path& source, const fs::path& destination, bool incremental) {
    checkedRoot(source);
    checkedRoot(destination);
    require(fs::is_regular_file(source), "Missing input: " + source.string());
    const auto size = fs::file_size(source);
    const auto time = fs::last_write_time(source);
    if (incremental && fs::is_regular_file(destination) && fs::file_size(destination) == size &&
        fs::last_write_time(destination) == time)
        return;
    fs::create_directories(destination.parent_path());
    fs::copy_file(source, destination, fs::copy_options::overwrite_existing);
    fs::permissions(destination, fs::status(source).permissions());
    fs::last_write_time(destination, time);
    require(fs::file_size(destination) == size && fs::file_size(source) == size &&
                fs::last_write_time(source) == time,
            "Input changed while staging: " + source.string());
}
fs::path ownedDistRoot(const Settings& s, bool create) {
    const auto build = buildRoot(s);
    const auto root = checkedRoot(build / "dist-stage");
    separate(root, checkedRoot(s.source));
    separate(root, checkedRoot(s.prepared));
    separate(root, checkedRoot(s.manifest).parent_path());
    const auto marker = root / ".blobber-dist-owner.json";
    const Json owner{{"format", 1}, {"build", build.generic_string()}};
    if (fs::exists(root)) {
        require(fs::is_directory(root), "Distribution root is not a directory");
        if (fs::exists(marker))
            require(readJson(marker) == owner, "Distribution ownership marker mismatch");
        else
            require(create && fs::is_empty(root),
                    "Refusing unowned nonempty distribution root: " + root.string());
    } else {
        require(create, "No successful distribution stage");
        fs::create_directory(root);
    }
    checkTree(root);
    if (create && !fs::exists(marker))
        writeJson(marker, owner);
    return root;
}
void supported(const Settings& s) {
#if (!defined(__linux__) || !defined(__x86_64__)) && (!defined(_WIN32) || !defined(_M_X64))
    throw std::runtime_error("dist-stage requires a native Linux or Windows x86-64 build");
#endif
    require(s.configuration == "Release" && !s.multiConfig,
            "dist-stage requires a single-configuration Release build (use the release Ninja preset)");
}
Json reportFor(const Settings& s, const Inventory& inventory) {
    Json report{{"format", 1},
                {"complete", true},
                {"configuration", s.configuration},
                {"stage_id", std::to_string(std::chrono::system_clock::now().time_since_epoch().count())},
                {"identities", inventory.identities},
                {"files", Json::array()},
                {"excluded", Json::array()}};
    std::uint64_t bytes = 0, excludedBytes = 0;
    std::set<fs::path> included;
    for (const auto& file : inventory.files) {
        const auto size = fs::file_size(file.source);
        report["files"].push_back({{"path", file.destination.generic_string()},
                                   {"input_root", file.root},
                                   {"source", file.relative},
                                   {"reason", file.reason},
                                   {"bytes", size}});
        included.insert(file.source);
        bytes += size;
    }
    for (const auto& input : {std::make_pair("assets", checkedRoot(s.source)),
                              std::make_pair("prepared", checkedRoot(s.prepared))}) {
        if (!fs::is_directory(input.second))
            continue;
        std::vector<fs::path> excluded;
        for (const auto& entry : fs::recursive_directory_iterator(input.second))
            if (!entry.is_symlink() && entry.is_regular_file() && !included.count(entry.path()))
                excluded.push_back(entry.path());
        std::sort(excluded.begin(), excluded.end());
        for (const auto& path : excluded) {
            const auto size = fs::file_size(path);
            report["excluded"].push_back({{"input_root", input.first},
                                          {"source", path.lexically_relative(input.second).generic_string()},
                                          {"reason", "not selected by runtime inventory"},
                                          {"bytes", size}});
            excludedBytes += size;
        }
    }
    report["total_bytes"] = bytes;
    report["excluded_bytes"] = excludedBytes;
    return report;
}
void verifyPayload(const Settings& s, const fs::path& payload, const Json& report) {
    require(report.at("format") == 1 && report.at("complete") == true &&
                report.at("configuration") == "Release",
            "Incomplete or incompatible stage report");
    checkTree(payload);
    std::set<fs::path> expected;
    std::uint64_t total = 0;
    for (const auto& entry : report.at("files")) {
        const auto path = relativePath(entry.at("path").get<std::string>());
        require(expected.insert(path).second, "Duplicate stage report path");
        const auto file = checkedRoot(payload / path);
        require(fs::is_regular_file(file) && fs::file_size(file) == entry.at("bytes").get<std::uint64_t>(),
                "Staged file missing or size differs: " + file.string());
        total += fs::file_size(file);
    }
    require(total == report.at("total_bytes").get<std::uint64_t>(), "Staged byte total differs");
    for (const auto& entry : fs::recursive_directory_iterator(payload))
        if (entry.is_regular_file())
            require(expected.count(entry.path().lexically_relative(payload)),
                    "Unexpected staged file: " + entry.path().string());
    const auto identities = validateContent(s, payload / "assets", payload / "assets/prepared", true);
    require(identities == report.at("identities"), "Staged content identities differ");
#ifdef _WIN32
    require(fs::is_regular_file(payload / "blobber.exe"), "Staged Windows executable missing");
#else
    const auto execute = fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec;
    require((fs::status(payload / "blobber").permissions() & execute) != fs::perms::none,
            "Staged executable has no execute permission");
#endif
}
} // namespace

void synchronizeRaw(const Settings& s) {
    const auto build = buildRoot(s);
    fs::path parent = build;
    if (s.multiConfig)
        parent /= relativePath(s.configuration);
    const auto root = checkedRoot(parent / "assets");
    const auto marker = checkedRoot(root / ".blobber-raw-inventory.json");
    const auto inventory = resolve(s, false);
    for (const auto& name : rawDirectories) {
        const auto owned = root / name;
        separate(owned, checkedRoot(s.source));
        separate(owned, checkedRoot(s.prepared));
        separate(owned, checkedRoot(s.manifest).parent_path());
        checkTree(owned);
        require(!fs::exists(owned) || fs::is_directory(owned),
                "Managed raw root is not a directory: " + owned.string());
    }
    // Refuse a prepared-root link even though this operation never changes its contents.
    checkedRoot(root / "prepared");
    std::set<fs::path> wanted;
    for (const auto& file : inventory.files) {
        auto component = file.destination.begin();
        if (*component != "assets")
            continue;
        ++component;
        require(component != file.destination.end() && rawDirectories.count(component->string()),
                "Raw file outside managed directories: " + file.destination.string());
        const auto path = file.destination.lexically_relative("assets");
        wanted.insert(path);
        checkedRoot(root / path);
    }
    fs::create_directories(root);
    // Success metadata is written last; a failed sync must not retain its old completion marker.
    fs::remove(marker);
    // Retire stale leaves first, including file/directory type transitions. Selected
    // inputs and every owned output tree have already passed preflight validation.
    for (const auto& name : rawDirectories) {
        const auto owned = root / name;
        if (!fs::exists(owned))
            continue;
        require(fs::is_directory(owned), "Managed raw root is not a directory: " + owned.string());
        std::vector<fs::path> directories;
        for (const auto& entry : fs::recursive_directory_iterator(owned)) {
            if (entry.is_directory())
                directories.push_back(entry.path());
            else if (!wanted.count(entry.path().lexically_relative(root)))
                fs::remove(entry.path());
        }
        std::sort(directories.rbegin(), directories.rend());
        for (const auto& directory : directories)
            if (fs::is_empty(directory))
                fs::remove(directory);
        if (fs::is_empty(owned))
            fs::remove(owned);
    }
    for (const auto& file : inventory.files)
        if (file.destination.begin()->string() == "assets")
            copy(file.source, root / file.destination.lexically_relative("assets"), true);
    const auto identities = validateContent(s, root, checkedRoot(s.prepared), false);
    require(identities == inventory.identities, "Raw staging content identities differ");
    writeJson(marker, {{"format", 1},
                       {"configuration", s.configuration},
                       {"identities", identities},
                       {"files", wanted.size()}});
    std::cout << "Runtime raw assets synchronized: " << wanted.size() << " files in " << root << '\n';
}

void stageDistribution(const Settings& s) {
    supported(s);
    auto inventory = resolve(s, true);
    const auto executable = checkedRoot(s.executable);
    require(fs::is_regular_file(executable), "Missing game executable: " + executable.string());
#ifdef _WIN32
    const char* gameName = "blobber.exe";
#else
    const char* gameName = "blobber";
#endif
    inventory.files.push_back({executable, gameName, "executable", gameName, "game executable"});
    std::sort(inventory.files.begin(), inventory.files.end(),
              [](const File& a, const File& b) { return a.destination < b.destination; });
    const auto root = ownedDistRoot(s, true);
    separate(root, executable);
    const auto candidate = root / ".candidate", candidateReport = root / ".candidate.inventory.json";
    const auto final = root / "Release", report = root / "Release.inventory.json";
    const auto previous = root / ".previous", previousReport = root / ".previous.inventory.json";
    require(!fs::exists(previous) && !fs::exists(previousReport),
            "Interrupted promotion: inspect .previous outputs in " + root.string());
    // Candidate leftovers belong to this marked output root, never to an input tree.
    fs::remove_all(candidate);
    fs::remove(candidateReport);
    bool movedStage = false, movedReport = false, installedStage = false, installedReport = false;
    try {
        fs::create_directory(candidate);
        for (const auto& file : inventory.files)
            copy(file.source, candidate / file.destination, false);
        const auto nextReport = reportFor(s, inventory);
        verifyPayload(s, candidate, nextReport);
        writeJson(candidateReport, nextReport);
        if (fs::exists(final)) {
            fs::rename(final, previous);
            movedStage = true;
        }
        if (fs::exists(report)) {
            fs::rename(report, previousReport);
            movedReport = true;
        }
        fs::rename(candidate, final);
        installedStage = true;
        fs::rename(candidateReport, report);
        installedReport = true;
    } catch (...) {
        if (installedReport)
            fs::remove(report);
        if (installedStage)
            fs::remove_all(final);
        if (movedStage)
            fs::rename(previous, final);
        if (movedReport)
            fs::rename(previousReport, report);
        fs::remove_all(candidate);
        fs::remove(candidateReport);
        throw;
    }
    fs::remove_all(previous);
    fs::remove(previousReport);
    const auto result = readJson(report);
    std::cout << "Content stage complete (no portable ABI claim): " << final << '\n'
              << result.at("files").size() << " files; " << result.at("total_bytes") << " bytes\n"
              << "Inventory: " << report << '\n';
}

Json verifyDistribution(const Settings& s) {
    supported(s);
    const auto root = ownedDistRoot(s, false);
    require(!fs::exists(root / ".previous") && !fs::exists(root / ".previous.inventory.json"),
            "Interrupted distribution promotion; stage is not verified");
    const auto report = readJson(root / "Release.inventory.json");
    verifyPayload(s, root / "Release", report);
    return report;
}
} // namespace blobber::distribution

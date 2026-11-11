#pragma once
#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace blobber::distribution {
namespace fs = std::filesystem;
using Json = nlohmann::json;
struct Settings {
    fs::path manifest, source, prepared, build, executable;
    std::string configuration;
    bool multiConfig = false;
};
struct File {
    fs::path source, destination;
    std::string root, relative, reason;
};
struct Inventory {
    std::vector<File> files;
    Json identities = Json::object();
};
// Paths are checked before loaders open files and before output is modified.
fs::path checkedRoot(const fs::path&);
fs::path relativePath(const std::string&);
bool within(const fs::path& root, const fs::path& path);
void checkTree(const fs::path&);
Json readJson(const fs::path&);
Inventory resolve(const Settings&, bool includePrepared);
Json validateContent(const Settings&, const fs::path& assets, const fs::path& prepared, bool includePrepared);
void synchronizeRaw(const Settings&);
void stageDistribution(const Settings&);
Json verifyDistribution(const Settings&);
} // namespace blobber::distribution

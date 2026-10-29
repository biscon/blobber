#pragma once
#include "game/arena.hpp"
#include <filesystem>
namespace blobber {
constexpr std::size_t arenaMaxBytes = 1024 * 1024;
std::string encodeArena(const rules::Catalog&, const ArenaScenario&);
ArenaScenario decodeArena(const rules::Catalog&, const std::string&);
struct ScenarioEntry {
    std::array<char, 256> file{};
    std::array<char, 65> name{};
    std::array<char, 192> error{};
    std::uint64_t revision = 0;
};
struct ScenarioListing {
    std::array<ScenarioEntry, 128> entries{};
    std::size_t count = 0;
};
struct ScenarioWrite {
    ScenarioEntry entry;
    bool durable = true;
};
enum class ScenarioStage { Write, Flush, Rename, DirectoryFlush };
struct ScenarioIoHooks {
    bool (*fail)(ScenarioStage) = nullptr;
};
class ArenaScenarioStore {
  public:
    ArenaScenarioStore(const rules::Catalog&, std::filesystem::path directory, ScenarioIoHooks = {});
    const std::filesystem::path& directory() const noexcept { return directory_; }
    ScenarioListing list() const;
    ArenaScenario load(const ScenarioEntry&) const;
    // Null replacement creates a uniquely named file; replacement checks the observed revision.
    ScenarioWrite save(const ArenaScenario&, const ScenarioEntry* replacement = nullptr) const;

  private:
    const rules::Catalog& catalog_;
    std::filesystem::path directory_;
    ScenarioIoHooks hooks_;
};
} // namespace blobber

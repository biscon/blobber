#pragma once
#include "rules/creation.hpp"
#include <filesystem>

namespace blobber {
struct SavedCharacterId {
    std::array<char, 37> text{};
    friend bool operator==(const SavedCharacterId& a, const SavedCharacterId& b) noexcept {
        return a.text == b.text;
    }
};
bool validSavedCharacterId(const SavedCharacterId&) noexcept;
struct SavedCharacter {
    SavedCharacterId id;
    rules::CharacterDraft draft;
};
struct LibrarySnapshot {
    static constexpr std::size_t capacity = 128;
    std::array<SavedCharacter, capacity> records{};
    std::size_t count = 0;
    std::uint64_t revision = 0;
    const SavedCharacter* find(SavedCharacterId) const noexcept;
};
enum class LibraryError {
    None,
    Missing,
    Invalid,
    Unsupported,
    Full,
    Stale,
    NotFound,
    Busy,
    Io,
    Entropy,
    Exhausted,
    RecoveryRequired,
    DurabilityUncertain
};
struct LibraryResult {
    LibraryError error = LibraryError::None;
    bool committed = false;
    SavedCharacterId id;
    std::array<char, 192> message{};
    explicit operator bool() const noexcept { return error == LibraryError::None; }
};
// Fault seams are confined to explicit disk operations, not ordinary queries.
enum class LibraryStage { Entropy, Write, Flush, Backup, Replace, DirectoryFlush };
struct LibraryIoHooks {
    bool (*fail)(LibraryStage) = nullptr;
    bool (*entropy)(std::array<std::uint8_t, 16>&) = nullptr;
};
std::filesystem::path characterLibraryPath(const std::filesystem::path& overridePath = {});
class CharacterLibrary {
  public:
    CharacterLibrary(const rules::Catalog&, std::filesystem::path, LibraryIoHooks = {});
    CharacterLibrary(rules::Catalog&&, std::filesystem::path, LibraryIoHooks = {}) = delete;
    CharacterLibrary(const rules::Catalog&&, std::filesystem::path, LibraryIoHooks = {}) = delete;
    const LibrarySnapshot& snapshot() const noexcept { return state_; }
    const std::filesystem::path& path() const noexcept { return path_; }
    LibraryResult load();       // On failure, preserve the last successful snapshot and disallow mutation.
    LibraryResult initialize(); // Seed only a genuinely absent store.
    LibraryResult create(const rules::CharacterDraft&, std::uint64_t expectedRevision);
    LibraryResult replace(SavedCharacterId, const rules::CharacterDraft&, std::uint64_t expectedRevision);
    LibraryResult erase(SavedCharacterId, std::uint64_t expectedRevision);
    rules::PresetParty startingParty(const std::array<SavedCharacterId, 4>&,
                                     const std::array<rules::CharacterId, 4>& formation) const;

  private:
    const rules::Catalog& catalog_;
    std::filesystem::path path_;
    LibraryIoHooks hooks_;
    LibrarySnapshot state_;
    bool loaded_ = false;
    LibraryResult commit(LibrarySnapshot&, std::uint64_t expected, bool initial = false);
    SavedCharacterId newId(const LibrarySnapshot&);
};
} // namespace blobber

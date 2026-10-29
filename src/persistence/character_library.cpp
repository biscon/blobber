#include "persistence/library_codec.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <sys/file.h>
#include <unistd.h>

namespace blobber {
namespace {
struct Descriptor {
    int fd = -1;
    explicit Descriptor(int value) : fd(value) {}
    ~Descriptor() {
        if (fd >= 0)
            ::close(fd);
    }
    Descriptor(const Descriptor&) = delete;
    Descriptor& operator=(const Descriptor&) = delete;
};
LibraryResult result(LibraryError code, const char* text, bool committed = false) {
    LibraryResult r;
    r.error = code;
    r.committed = committed;
    std::snprintf(r.message.data(), r.message.size(), "%s", text);
    return r;
}
[[noreturn]] void io(const char* operation) {
    throw LibraryException(LibraryError::Io, std::string(operation) + ": " + std::strerror(errno));
}
void fault(const LibraryIoHooks& hooks, LibraryStage stage) {
    if (hooks.fail && hooks.fail(stage))
        throw LibraryException(stage == LibraryStage::Entropy ? LibraryError::Entropy : LibraryError::Io,
                               "Injected library I/O failure");
}
std::string readFile(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f)
        throw LibraryException(LibraryError::Io, "Cannot open character library");
    std::string out;
    char block[4096];
    while (f.read(block, sizeof(block)) || f.gcount()) {
        if (out.size() + std::size_t(f.gcount()) > libraryMaxBytes)
            throw LibraryException(LibraryError::Invalid, "Library exceeds 1 MiB");
        out.append(block, std::size_t(f.gcount()));
    }
    if (f.bad())
        throw LibraryException(LibraryError::Io, "Cannot read character library");
    return out;
}
void writeFile(const std::filesystem::path& path, const std::string& text, const LibraryIoHooks& hooks) {
    Descriptor file(::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600));
    if (file.fd < 0)
        io("Create temporary library");
    try {
        fault(hooks, LibraryStage::Write);
        std::size_t offset = 0;
        while (offset < text.size()) {
            const auto n = ::write(file.fd, text.data() + offset, text.size() - offset);
            if (n < 0 && errno == EINTR)
                continue;
            if (n <= 0)
                io("Write library");
            offset += std::size_t(n);
        }
        fault(hooks, LibraryStage::Flush);
        if (::fsync(file.fd) < 0)
            io("Flush library");
        const int fd = file.fd;
        file.fd = -1;
        if (::close(fd) < 0)
            io("Close library");
    } catch (...) {
        ::unlink(path.c_str());
        throw;
    }
}
bool systemEntropy(std::array<std::uint8_t, 16>& bytes) {
    Descriptor f(::open("/dev/urandom", O_RDONLY | O_CLOEXEC));
    if (f.fd < 0)
        return false;
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const auto n = ::read(f.fd, bytes.data() + offset, bytes.size() - offset);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return false;
        offset += std::size_t(n);
    }
    return true;
}
bool artifacts(const std::filesystem::path& p) {
    return std::filesystem::exists(p.string() + ".bak") || std::filesystem::exists(p.string() + ".tmp") ||
           std::filesystem::exists(p.string() + ".bak.tmp") || std::filesystem::exists(p.string() + ".lock");
}
} // namespace
bool validSavedCharacterId(const SavedCharacterId& id) noexcept {
    if (id.text[36] || id.text[14] != '4' ||
        (id.text[19] != '8' && id.text[19] != '9' && id.text[19] != 'a' && id.text[19] != 'b'))
        return false;
    for (unsigned i = 0; i < 36; ++i) {
        const auto c = id.text[i];
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (c != '-')
                return false;
        } else if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            return false;
    }
    return true;
}
const SavedCharacter* LibrarySnapshot::find(SavedCharacterId id) const noexcept {
    for (std::size_t i = 0; i < count; ++i)
        if (records[i].id == id)
            return &records[i];
    return nullptr;
}
std::filesystem::path characterLibraryPath(const std::filesystem::path& overridePath) {
    if (!overridePath.empty())
        return std::filesystem::absolute(overridePath);
    if (const auto* xdg = std::getenv("XDG_DATA_HOME");
        xdg && *xdg && std::filesystem::path(xdg).is_absolute())
        return std::filesystem::path(xdg) / "blobber/characters.json";
    if (const auto* home = std::getenv("HOME"); home && *home && std::filesystem::path(home).is_absolute())
        return std::filesystem::path(home) / ".local/share/blobber/characters.json";
    throw std::runtime_error("No absolute user-data directory; supply a character library path");
}
CharacterLibrary::CharacterLibrary(const rules::Catalog& c, std::filesystem::path path, LibraryIoHooks hooks)
    : catalog_(c), path_(characterLibraryPath(path)), hooks_(hooks) {}
LibraryResult CharacterLibrary::load() {
    loaded_ = false;
    try {
        if (!std::filesystem::exists(path_))
            return result(artifacts(path_) ? LibraryError::RecoveryRequired : LibraryError::Missing,
                          "Library missing; existing artifacts require manual recovery");
        auto candidate = decodeLibrary(catalog_, readFile(path_));
        state_ = candidate;
        loaded_ = true;
        return {};
    } catch (const LibraryException& e) {
        return result(e.code, e.what());
    } catch (const std::exception& e) {
        return result(LibraryError::Invalid, e.what());
    }
}
SavedCharacterId CharacterLibrary::newId(const LibrarySnapshot& state) {
    for (unsigned attempt = 0; attempt < 8; ++attempt) {
        fault(hooks_, LibraryStage::Entropy);
        std::array<std::uint8_t, 16> bytes{};
        if (!(hooks_.entropy ? hooks_.entropy(bytes) : systemEntropy(bytes)))
            throw LibraryException(LibraryError::Entropy, "Cannot obtain UUID entropy");
        bytes[6] = (bytes[6] & 15) | 64;
        bytes[8] = (bytes[8] & 63) | 128;
        SavedCharacterId id;
        constexpr char hex[] = "0123456789abcdef";
        unsigned n = 0;
        for (unsigned i = 0; i < 16; ++i) {
            if (i == 4 || i == 6 || i == 8 || i == 10)
                id.text[n++] = '-';
            id.text[n++] = hex[bytes[i] >> 4];
            id.text[n++] = hex[bytes[i] & 15];
        }
        if (!state.find(id))
            return id;
    }
    throw LibraryException(LibraryError::Entropy, "UUID collisions exhausted eight attempts");
}
LibraryResult CharacterLibrary::commit(LibrarySnapshot& candidate, std::uint64_t expected, bool initial) {
    bool committed = false;
    bool ownTemporary = false, ownBackupTemporary = false;
    const auto temporary = path_.string() + ".tmp", backup = path_.string() + ".bak",
               backupTemporary = backup + ".tmp";
    try {
        if (expected == UINT64_MAX)
            return result(LibraryError::Exhausted, "Library revision exhausted");
        if (!initial && (!loaded_ || state_.revision != expected))
            return result(LibraryError::Stale, "Reload the library before changing it");
        std::filesystem::create_directories(path_.parent_path());
        Descriptor lock(::open((path_.string() + ".lock").c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600));
        if (lock.fd < 0)
            io("Open library lock");
        if (::flock(lock.fd, LOCK_EX | LOCK_NB) < 0)
            return result(LibraryError::Busy, "Library is being changed by another writer");
        std::string old;
        if (std::filesystem::exists(path_)) {
            old = readFile(path_);
            const auto disk = decodeLibrary(catalog_, old);
            if (initial || disk.revision != expected)
                return result(LibraryError::Stale, "Library changed on disk; reload it");
        } else if (!initial)
            return result(LibraryError::RecoveryRequired, "Library disappeared; original artifacts retained");
        else if (std::filesystem::exists(backup) || std::filesystem::exists(temporary) ||
                 std::filesystem::exists(backupTemporary))
            return result(LibraryError::RecoveryRequired,
                          "Existing library artifacts require manual recovery");
        candidate.revision = expected + 1;
        const auto text = encodeLibrary(catalog_, candidate);
        writeFile(temporary, text, hooks_);
        ownTemporary = true;
        fault(hooks_, LibraryStage::Backup);
        // Initial stores also retain a valid recovery copy. Empty libraries are documents, never deletion.
        writeFile(backupTemporary, initial ? text : old, {});
        ownBackupTemporary = true;
        if (::rename(backupTemporary.c_str(), backup.c_str()) < 0)
            io("Replace library backup");
        ownBackupTemporary = false;
        Descriptor directory(::open(path_.parent_path().c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC));
        if (directory.fd < 0)
            io("Open library directory");
        if (::fsync(directory.fd) < 0)
            io("Flush library backup directory");
        fault(hooks_, LibraryStage::Replace);
        if (::rename(temporary.c_str(), path_.c_str()) < 0)
            io("Replace library");
        ownTemporary = false;
        committed = true;
        state_ = candidate;
        loaded_ = true;
        fault(hooks_, LibraryStage::DirectoryFlush);
        if (::fsync(directory.fd) < 0)
            io("Flush library directory");
        return result(LibraryError::None, "Library saved", true);
    } catch (const std::exception& e) {
        if (ownTemporary)
            ::unlink(temporary.c_str());
        if (ownBackupTemporary)
            ::unlink(backupTemporary.c_str());
        if (committed)
            return result(LibraryError::DurabilityUncertain,
                          "Saved, but directory durability could not be confirmed; reload before retrying",
                          true);
        const auto* specific = dynamic_cast<const LibraryException*>(&e);
        return result(specific ? specific->code : LibraryError::Io, e.what());
    }
}
LibraryResult CharacterLibrary::initialize() {
    const auto loaded = load();
    if (loaded)
        return loaded;
    if (loaded.error != LibraryError::Missing)
        return loaded;
    try {
        LibrarySnapshot candidate;
        if (!catalog_.preset())
            return result(LibraryError::Invalid, "Cannot seed library without presets");
        for (const auto& b : catalog_.preset()->characters) {
            SavedCharacter r;
            if (!rules::draftFromBuild(catalog_, b, r.draft))
                return result(LibraryError::Invalid, "Preset is not a legal starting build");
            r.id = newId(candidate);
            candidate.records[candidate.count++] = r;
        }
        return commit(candidate, 0, true);
    } catch (const LibraryException& e) {
        return result(e.code, e.what());
    } catch (const std::exception& e) {
        return result(LibraryError::Io, e.what());
    }
}
LibraryResult CharacterLibrary::create(const rules::CharacterDraft& draft, std::uint64_t expected) {
    if (!loaded_ || state_.revision != expected)
        return result(LibraryError::Stale, "Reload library before saving");
    if (!rules::previewBuild(catalog_, draft))
        return result(LibraryError::Invalid, "Character draft is incomplete or invalid");
    if (state_.count == state_.capacity)
        return result(LibraryError::Full, "Character library is full (128 characters)");
    try {
        auto candidate = state_;
        SavedCharacter r;
        r.draft = draft;
        r.id = newId(candidate);
        candidate.records[candidate.count++] = r;
        auto saved = commit(candidate, expected);
        if (saved.committed)
            saved.id = r.id;
        return saved;
    } catch (const LibraryException& e) {
        return result(e.code, e.what());
    } catch (const std::exception& e) {
        return result(LibraryError::Io, e.what());
    }
}
LibraryResult CharacterLibrary::replace(SavedCharacterId id, const rules::CharacterDraft& draft,
                                        std::uint64_t expected) {
    if (!loaded_ || state_.revision != expected)
        return result(LibraryError::Stale, "Reload library before saving");
    if (!rules::previewBuild(catalog_, draft))
        return result(LibraryError::Invalid, "Character draft is incomplete or invalid");
    auto candidate = state_;
    for (std::size_t i = 0; i < candidate.count; ++i)
        if (candidate.records[i].id == id) {
            candidate.records[i].draft = draft;
            auto r = commit(candidate, expected);
            r.id = id;
            return r;
        }
    return result(LibraryError::NotFound, "Character no longer exists");
}
LibraryResult CharacterLibrary::erase(SavedCharacterId id, std::uint64_t expected) {
    if (!loaded_ || state_.revision != expected)
        return result(LibraryError::Stale, "Reload library before deleting");
    auto candidate = state_;
    for (std::size_t i = 0; i < candidate.count; ++i)
        if (candidate.records[i].id == id) {
            std::move(candidate.records.begin() + i + 1, candidate.records.begin() + candidate.count,
                      candidate.records.begin() + i);
            candidate.records[--candidate.count] = {};
            return commit(candidate, expected);
        }
    return result(LibraryError::NotFound, "Character no longer exists");
}
rules::PresetParty CharacterLibrary::startingParty(const std::array<SavedCharacterId, 4>& ids,
                                                   const std::array<rules::CharacterId, 4>& formation) const {
    if (!loaded_ || !rules::validFormation(formation))
        throw std::invalid_argument("Invalid library or formation");
    rules::PresetParty out;
    out.formation = formation;
    for (std::size_t i = 0; i < 4; ++i) {
        for (std::size_t j = 0; j < i; ++j)
            if (ids[i] == ids[j])
                throw std::invalid_argument("Select four distinct characters");
        const auto* r = state_.find(ids[i]);
        if (!r)
            throw std::invalid_argument("Selected character no longer exists");
        out.characters[i] = rules::finishBuild(catalog_, r->draft, r->id.text.data());
    }
    return out;
}
} // namespace blobber

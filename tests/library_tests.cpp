#include "persistence/library_codec.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sys/file.h>
#include <unistd.h>
using namespace blobber;
namespace {
int failing = -1;
bool failStage(LibraryStage s) {
    return int(s) == failing;
}
bool noEntropy(std::array<std::uint8_t, 16>&) {
    return false;
}
bool repeatedEntropy(std::array<std::uint8_t, 16>& b) {
    b.fill(7);
    return true;
}
std::string read(const std::filesystem::path& p) {
    std::ifstream f(p);
    return {std::istreambuf_iterator<char>(f), {}};
}
void write(const std::filesystem::path& p, const std::string& s) {
    std::ofstream(p) << s;
}
struct Temp {
    std::filesystem::path path;
    Temp() {
        char name[] = "/tmp/blobber-library-XXXXXX";
        auto* p = ::mkdtemp(name);
        if (!p)
            throw std::runtime_error("mkdtemp");
        path = p;
    }
    ~Temp() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};
} // namespace
int main() {
    try {
        const auto cat = creationCatalog();
        Temp temp;
        const auto path = temp.path / "characters.json";
        CharacterLibrary library(cat, path, {failStage, nullptr});
        check(library.load().error == LibraryError::Missing, "missing first-run store");
        auto init = library.initialize();
        check(bool(init) && init.committed && library.snapshot().count == 4, "seed four exactly once");
        const auto seeded = read(path);
        check(bool(library.initialize()) && read(path) == seeded, "no reseeding existing store");
        auto draft = library.snapshot().records[0].draft;
        const auto oldRevision = library.snapshot().revision;
        auto added = library.create(draft, oldRevision);
        check(added && added.committed && validSavedCharacterId(added.id),
              "save duplicate name independent id");
        check(library.create(draft, oldRevision).error == LibraryError::Stale &&
                  library.snapshot().count == 5,
              "duplicate activation stale");
        std::array<SavedCharacterId, 4> ids;
        for (unsigned i = 0; i < 4; ++i)
            ids[i] = library.snapshot().records[i].id;
        auto starting = library.startingParty(ids, {{{3}, {2}, {1}, {0}}});
        Party party(cat, starting);
        check(bool(library.erase(ids[0], library.snapshot().revision)), "delete saved blueprint");
        check(party.character({0})->hitPoints > 0, "session survives deletion");
        rejects([&] { library.startingParty(ids, {{{0}, {1}, {2}, {3}}}); }, "deleted selection rejects");
        CharacterLibrary other(cat, path);
        check(bool(other.load()), "round trip");
        check(bool(library.create(draft, library.snapshot().revision)), "concurrent first writer");
        check(other.create(draft, other.snapshot().revision).error == LibraryError::Stale,
              "stale disk revision rejected");
        for (auto stage : {LibraryStage::Entropy, LibraryStage::Write, LibraryStage::Flush,
                           LibraryStage::Backup, LibraryStage::Replace}) {
            const auto prior = read(path);
            const auto n = library.snapshot().count;
            failing = int(stage);
            auto failed = library.create(draft, library.snapshot().revision);
            check(!failed && !failed.committed, "injected save rejected");
            check(read(path) == prior && library.snapshot().count == n,
                  "failed write preserves memory and primary");
        }
        failing = int(LibraryStage::DirectoryFlush);
        auto uncertain = library.create(draft, library.snapshot().revision);
        check(uncertain.error == LibraryError::DurabilityUncertain && uncertain.committed,
              "post-replacement result is committed");
        failing = -1;
        check(bool(other.load()) && other.snapshot().count == library.snapshot().count,
              "committed uncertainty reload");
        auto prior = read(path);
        failing = int(LibraryStage::Replace);
        check(!library.erase(added.id, library.snapshot().revision) && read(path) == prior,
              "failed delete preserves primary");
        failing = -1;
        auto revised = draft;
        std::snprintf(revised.name.data(), revised.name.size(), "Replacement");
        check(bool(library.replace(added.id, revised, library.snapshot().revision)), "checked replace");
        const int lock = ::open((path.string() + ".lock").c_str(), O_RDWR);
        check(lock >= 0 && ::flock(lock, LOCK_EX | LOCK_NB) == 0, "test lock");
        check(library.create(draft, library.snapshot().revision).error == LibraryError::Busy,
              "writer lock rejects busy");
        ::close(lock);
        prior = read(path);
        auto json = nlohmann::json::parse(prior);
        auto invalid = [&](const std::string& bytes, LibraryError error = LibraryError::Invalid) {
            write(path, bytes);
            const auto n = library.snapshot().count;
            auto r = library.load();
            check(r.error == error, "invalid library status");
            check(library.snapshot().count == n && read(path) == bytes, "invalid library preserved");
            check(library.create(draft, library.snapshot().revision).error == LibraryError::Stale,
                  "invalid load disables mutation");
            write(path, prior);
            check(bool(library.load()), "reload valid snapshot");
        };
        invalid("{");
        invalid(std::string(libraryMaxBytes + 1, ' '));
        invalid("{\"schema_version\":1,\"schema_version\":1}");
        auto modified = json;
        modified["schema_version"] = 99;
        invalid(modified.dump(), LibraryError::Unsupported);
        modified = json;
        modified["characters"][0]["portrait"] = "missing";
        invalid(modified.dump());
        modified = json;
        modified["characters"][0]["spells"]["racial"] = {"missing"};
        invalid(modified.dump());
        modified = json;
        modified["characters"][1]["id"] = modified["characters"][0]["id"];
        invalid(modified.dump());
        modified = json;
        modified["characters"][0]["gear"]["items"][0]["quantity"] = 999;
        invalid(modified.dump());
        modified = json;
        modified["revision"] = std::numeric_limits<std::uint64_t>::max();
        write(path, modified.dump());
        check(bool(library.load()), "max revision reads");
        check(library.create(draft, library.snapshot().revision).error == LibraryError::Exhausted,
              "revision overflow rejects");
        write(path, prior);
        check(bool(library.load()), "restore revision");
        CharacterLibrary entropy(cat, path, {nullptr, noEntropy});
        check(bool(entropy.load()), "entropy fixture reads");
        check(entropy.create(draft, entropy.snapshot().revision).error == LibraryError::Entropy,
              "entropy failure no record");
        CharacterLibrary collisions(cat, temp.path / "collision.json", {nullptr, repeatedEntropy});
        check(collisions.initialize().error == LibraryError::Entropy, "bounded collision failure");
        // Boundary-capacity fixture is encoded directly to keep routine disk tests fast.
        auto full = library.snapshot();
        while (full.count < full.capacity) {
            auto r = full.records[0];
            std::snprintf(r.id.text.data(), r.id.text.size(), "00000000-0000-4000-8000-%012zu", full.count);
            full.records[full.count++] = r;
        }
        write(path, encodeLibrary(cat, full));
        check(bool(library.load()), "capacity file reads");
        check(library.create(draft, library.snapshot().revision).error == LibraryError::Full,
              "full library rejects addition");
        auto over = nlohmann::json::parse(read(path));
        over["characters"].push_back(over["characters"][0]);
        write(path, over.dump());
        check(!library.load(), "oversized record count rejects");
        LibrarySnapshot empty;
        empty.revision = 7;
        write(path, encodeLibrary(cat, empty));
        check(bool(library.initialize()) && library.snapshot().count == 0, "intentional empty does not seed");
        std::filesystem::remove(path);
        check(library.initialize().error == LibraryError::RecoveryRequired,
              "missing primary plus artifacts does not seed");
        const auto interrupted = temp.path / "interrupted.json";
        write(interrupted.string() + ".tmp", "partial");
        CharacterLibrary interruptedLibrary(cat, interrupted);
        check(interruptedLibrary.initialize().error == LibraryError::RecoveryRequired,
              "interrupted first write retains artifact");
        std::cout << "Library round trips, bounds, ownership and failure transactions passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

#include "persistence/arena_store.hpp"
#include "assets/rules_json.hpp"
#include "persistence/starting_codec.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <set>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
namespace blobber {
namespace {
using namespace rules_json;
struct Fd {
    int fd;
    ~Fd() {
        if (fd >= 0)
            ::close(fd);
    }
};
std::uint64_t digest(const std::string& s) {
    std::uint64_t h = 14695981039346656037ull;
    for (unsigned char ch : s) {
        h ^= ch;
        h *= 1099511628211ull;
    }
    return h;
}
std::string read(const std::filesystem::path& path) {
    Fd f{::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK)};
    struct stat st{};
    if (f.fd < 0 || ::fstat(f.fd, &st) || !S_ISREG(st.st_mode) || st.st_size < 0 ||
        std::uint64_t(st.st_size) > arenaMaxBytes)
        throw std::runtime_error("Scenario must be a readable regular file within 1 MiB.");
    std::string text(std::size_t(st.st_size), '\0');
    std::size_t offset = 0;
    while (offset < text.size()) {
        const auto n = ::read(f.fd, text.data() + offset, text.size() - offset);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            throw std::runtime_error("Scenario changed or could not be read.");
        offset += std::size_t(n);
    }
    char extra;
    if (::read(f.fd, &extra, 1) != 0)
        throw std::runtime_error("Scenario changed while reading.");
    return text;
}
std::filesystem::path child(const std::filesystem::path& directory, const char* file) {
    std::filesystem::path p(file);
    if (p.empty() || p.has_parent_path() || p.extension() != ".json")
        throw std::runtime_error("Invalid scenario filename.");
    return directory / p;
}
} // namespace
std::string encodeArena(const rules::Catalog& c, const ArenaScenario& s) {
    if (const auto* error = arenaError(c, s))
        throw std::runtime_error(error);
    Json party = Json::array(), roster = Json::array();
    for (const auto& member : s.party)
        party.push_back(encodeStartingDraft(c, member.draft, member.id.data()));
    for (std::size_t i = 0; i < s.count; ++i)
        roster.push_back(
            {{"monster", c.monster(s.roster[i].monster)->id}, {"quantity", s.roster[i].quantity}});
    auto text = Json{{"schema_version", 1}, {"layout_version", 1}, {"name", s.name.data()},
                     {"seed", s.seed},      {"party", party},      {"monsters", roster}}
                    .dump(2) +
                "\n";
    if (text.size() > arenaMaxBytes)
        throw std::runtime_error("Scenario exceeds 1 MiB.");
    return text;
}
ArenaScenario decodeArena(const rules::Catalog& c, const std::string& text) {
    if (text.size() > arenaMaxBytes)
        throw std::runtime_error("Scenario exceeds 1 MiB.");
    std::array<std::set<std::string>, 17> keys;
    auto root = Json::parse(text, [&](int depth, Json::parse_event_t event, Json& v) {
        if (depth > 16)
            fail("json", "excessive nesting");
        if (event == Json::parse_event_t::object_start)
            keys[std::size_t(depth)].clear();
        if (event == Json::parse_event_t::key &&
            !keys[std::size_t(depth - 1)].insert(v.get<std::string>()).second)
            fail("json", "duplicate key");
        return true;
    });
    fields(root, "scenario", {"schema_version", "layout_version", "name", "seed", "party", "monsters"});
    if (integer(root.at("schema_version"), "version") != 1 ||
        integer(root.at("layout_version"), "layout") != 1)
        fail("version", "unsupported scenario version");
    ArenaScenario s;
    const auto name = string(root.at("name"), "name");
    if (!rules::validDisplayText(name, 64))
        fail("name", "invalid scenario name");
    std::copy(name.begin(), name.end(), s.name.begin());
    const auto& seed = root.at("seed");
    if ((!seed.is_number_unsigned() && !seed.is_number_integer()) ||
        (seed.is_number_integer() && seed.get<std::int64_t>() < 0) || seed.get<std::uint64_t>() > UINT32_MAX)
        fail("seed", "expected uint32");
    s.seed = seed.get<std::uint32_t>();
    array(root.at("party"), "party", 4);
    if (root.at("party").size() != 4)
        fail("party", "expected four characters");
    for (unsigned i = 0; i < 4; ++i) {
        const auto& j = root.at("party")[i];
        s.party[i].draft = decodeStartingDraft(c, j);
        const auto id = j.at("id").get<std::string>();
        std::copy(id.begin(), id.end(), s.party[i].id.begin());
    }
    array(root.at("monsters"), "monsters", NpcWorld::maxTypes);
    for (const auto& j : root.at("monsters")) {
        fields(j, "monster", {"monster", "quantity"});
        const auto quantity = integer(j.at("quantity"), "quantity");
        if (quantity <= 0 || quantity > int(NpcWorld::maxInstances))
            fail("quantity", "expected 1..256");
        s.roster[s.count++] = {c.findMonster(identifier(j.at("monster"), "monster")), unsigned(quantity)};
    }
    if (const auto* error = arenaError(c, s))
        throw std::runtime_error(error);
    return s;
}
ArenaScenarioStore::ArenaScenarioStore(const rules::Catalog& c, std::filesystem::path path,
                                       ScenarioIoHooks hooks)
    : catalog_(c), directory_(std::move(path)), hooks_(hooks) {}
ScenarioListing ArenaScenarioStore::list() const {
    ScenarioListing result;
    if (!std::filesystem::exists(directory_))
        return result;
    for (const auto& f : std::filesystem::directory_iterator(directory_)) {
        if (f.path().extension() != ".json")
            continue;
        if (result.count == result.entries.size())
            throw std::runtime_error("Scenario folder exceeds 128 files. Move some files and refresh.");
        auto& entry = result.entries[result.count++];
        const auto filename = f.path().filename().string();
        if (filename.size() >= entry.file.size())
            throw std::runtime_error("Scenario filename too long.");
        std::snprintf(entry.file.data(), entry.file.size(), "%s", filename.c_str());
        std::snprintf(entry.name.data(), entry.name.size(), "%s", filename.c_str());
        try {
            const auto text = read(f.path());
            entry.revision = digest(text);
            const auto scenario = decodeArena(catalog_, text);
            entry.name = scenario.name;
        } catch (const std::exception& e) {
            std::snprintf(entry.error.data(), entry.error.size(), "%s", e.what());
        }
    }
    std::sort(result.entries.begin(), result.entries.begin() + result.count,
              [](const auto& a, const auto& b) {
                  const auto cmp = std::strcmp(a.name.data(), b.name.data());
                  return cmp ? cmp < 0 : std::strcmp(a.file.data(), b.file.data()) < 0;
              });
    return result;
}
ArenaScenario ArenaScenarioStore::load(const ScenarioEntry& e) const {
    const auto text = read(child(directory_, e.file.data()));
    if (digest(text) != e.revision)
        throw std::runtime_error("Scenario changed. Refresh and select again.");
    return decodeArena(catalog_, text);
}
ScenarioWrite ArenaScenarioStore::save(const ArenaScenario& s, const ScenarioEntry* replace) const {
    const auto text = encodeArena(catalog_, s);
    std::filesystem::create_directories(directory_);
    Fd lock{::open((directory_ / ".lock").c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600)};
    if (lock.fd < 0 || ::flock(lock.fd, LOCK_EX))
        throw std::runtime_error("Cannot lock scenario folder.");
    auto listing = list();
    if (!replace && listing.count == listing.entries.size())
        throw std::runtime_error("Scenario folder is full (128).");
    auto target = replace ? child(directory_, replace->file.data()) : std::filesystem::path{};
    if (replace) {
        if (replace->error[0] || digest(read(target)) != replace->revision)
            throw std::runtime_error("Scenario changed or invalid. Reload or Save As.");
    } else {
        for (unsigned i = 1; i <= 10000; ++i) {
            target = directory_ / ("scenario-" + std::to_string(i) + ".json");
            if (!std::filesystem::exists(std::filesystem::symlink_status(target)))
                break;
            if (i == 10000)
                throw std::runtime_error("No free scenario filename.");
        }
    }
    ScenarioWrite result;
    std::snprintf(result.entry.file.data(), result.entry.file.size(), "%s", target.filename().c_str());
    result.entry.name = s.name;
    result.entry.revision = digest(text);
    auto pattern = (directory_ / ".scenario-XXXXXX").string();
    Fd temp{::mkstemp(pattern.data())};
    if (temp.fd < 0)
        throw std::runtime_error("Cannot create temporary scenario file.");
    bool committed = false;
    auto fault = [&](ScenarioStage stage) {
        if (hooks_.fail && hooks_.fail(stage))
            throw std::runtime_error("Scenario write interrupted.");
    };
    try {
        fault(ScenarioStage::Write);
        std::size_t offset = 0;
        while (offset < text.size()) {
            const auto n = ::write(temp.fd, text.data() + offset, text.size() - offset);
            if (n < 0 && errno == EINTR)
                continue;
            if (n <= 0)
                throw std::runtime_error("Cannot write scenario.");
            offset += std::size_t(n);
        }
        fault(ScenarioStage::Flush);
        if (::fsync(temp.fd))
            throw std::runtime_error("Cannot flush scenario.");
        const int descriptor = temp.fd;
        temp.fd = -1;
        if (::close(descriptor))
            throw std::runtime_error("Cannot close scenario before publication.");
        fault(ScenarioStage::Rename);
        // Lock serializes our writers; recheck external edits immediately before publication.
        if (replace && digest(read(target)) != replace->revision)
            throw std::runtime_error("Scenario changed during save.");
        if (replace) {
            if (::rename(pattern.c_str(), target.c_str()))
                throw std::runtime_error("Cannot replace scenario.");
        } else {
            if (::link(pattern.c_str(), target.c_str()))
                throw std::runtime_error("Cannot publish scenario; filename changed.");
            ::unlink(pattern.c_str());
        }
        committed = true;
        Fd dir{::open(directory_.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC)};
        result.durable = !(hooks_.fail && hooks_.fail(ScenarioStage::DirectoryFlush)) && dir.fd >= 0 &&
                         ::fsync(dir.fd) == 0;
        return result;
    } catch (...) {
        if (!committed)
            ::unlink(pattern.c_str());
        throw;
    }
}
} // namespace blobber

#include "persistence/campaign_store.hpp"
#include "assets/fingerprint.hpp"
#include "persistence/character_library.hpp"
#include <cerrno>
#include <fcntl.h>
#include <nlohmann/json.hpp>
#include <set>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
namespace blobber {
namespace {
using Json = nlohmann::json;
struct Fd {
    int value = -1;
    ~Fd() {
        if (value >= 0)
            ::close(value);
    }
};
CampaignIoReply diskRead(const std::filesystem::path& path) {
    Fd file{::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK)};
    if (file.value < 0) {
        if (errno == ENOENT)
            return {};
        throw std::runtime_error("Cannot read campaign slot");
    }
    struct stat info{};
    if (::fstat(file.value, &info) || !S_ISREG(info.st_mode) || info.st_size < 0 ||
        std::uint64_t(info.st_size) > campaignMaxBytes)
        throw std::runtime_error("Campaign slot must be a regular file within 16 MiB");
    CampaignIoReply out;
    out.exists = true;
    out.data.resize(std::size_t(info.st_size));
    std::size_t offset = 0;
    while (offset < out.data.size()) {
        const auto n = ::read(file.value, out.data.data() + offset, out.data.size() - offset);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            throw std::runtime_error("Campaign slot changed during read");
        offset += std::size_t(n);
    }
    char extra;
    if (::read(file.value, &extra, 1) != 0)
        throw std::runtime_error("Campaign slot changed during read");
    return out;
}
bool terminalRecord(const std::string& text, std::string& id) {
    // Terminal markers have a deliberately tiny, exact shape.
    if (text.size() > 512)
        return false;
    bool duplicate = false;
    std::set<std::string> keys;
    auto j = Json::parse(
        text,
        [&](int, Json::parse_event_t event, Json& value) {
            if (event == Json::parse_event_t::key && !keys.insert(value.get<std::string>()).second)
                duplicate = true;
            return true;
        },
        false);
    if (duplicate)
        return false;
    if (!j.is_object() || j.size() != 3 || !j.contains("terminal_schema") ||
        !j["terminal_schema"].is_number_integer() || j["terminal_schema"] != 1 || !j.contains("run_id") ||
        !j["run_id"].is_string() || !j.contains("status") ||
        (j["status"] != "victory" && j["status"] != "defeat"))
        return false;
    id = j["run_id"].get<std::string>();
    return id.size() == 32 && id.find_first_not_of("0123456789abcdef") == std::string::npos;
}
} // namespace
std::string campaignError(const char* message) {
    constexpr const char* digits = "0123456789abcdef";
    std::string out;
    for (std::size_t i = 0; message[i] && out.size() < 508; ++i) {
        const auto c = static_cast<unsigned char>(message[i]);
        if (c >= 32 && c < 127)
            out += char(c);
        else {
            out += "\\x";
            out += digits[c >> 4];
            out += digits[c & 15];
        }
    }
    return out;
}
std::string campaignRevision(const std::string& text) {
    auto h = fingerprintBasis;
    fingerprintBytes(h, text.data(), text.size());
    return std::to_string(h);
}
std::filesystem::path campaignSavePath(const std::filesystem::path& overridePath) {
    if (!overridePath.empty())
        return std::filesystem::absolute(overridePath);
    return characterLibraryPath().parent_path() / "campaign.json";
}
CampaignDisk::CampaignDisk(std::filesystem::path path, CampaignIoHooks hooks)
    : path_(std::move(path)), hooks_(hooks) {}
CampaignIoReply CampaignDisk::exchange(const CampaignIoRequest& request) {
    CampaignIoReply result;
    const auto fail = [&](CampaignIoStage stage) {
        if (hooks_.fail && hooks_.fail(stage))
            throw std::runtime_error("Campaign I/O interrupted");
    };
    try {
        if (request.operation == "identity") {
            result.data = request.data;
            return result;
        }
        if (request.operation == "read") {
            fail(CampaignIoStage::Read);
            return diskRead(path_);
        }
        if (request.operation != "replace" || request.data.size() > campaignMaxBytes)
            throw std::runtime_error("Invalid campaign file operation");
        std::filesystem::create_directories(path_.parent_path());
        Fd lock{::open((path_.string() + ".lock").c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600)};
        if (lock.value < 0 || ::flock(lock.value, LOCK_EX))
            throw std::runtime_error("Cannot lock campaign slot");
        auto check = [&] {
            auto current = diskRead(path_);
            if ((current.exists ? campaignRevision(current.data) : "missing") != request.expected)
                throw std::runtime_error("Campaign slot changed; review it before retrying");
        };
        check();
        auto temp = (path_.parent_path() / ".campaign-XXXXXX").string();
        Fd file{::mkstemp(temp.data())};
        if (file.value < 0)
            throw std::runtime_error("Cannot create campaign temporary file");
        try {
            fail(CampaignIoStage::Write);
            std::size_t offset = 0;
            while (offset < request.data.size()) {
                const auto n =
                    ::write(file.value, request.data.data() + offset, request.data.size() - offset);
                if (n < 0 && errno == EINTR)
                    continue;
                if (n <= 0)
                    throw std::runtime_error("Cannot write campaign slot");
                offset += std::size_t(n);
            }
            fail(CampaignIoStage::Flush);
            if (::fsync(file.value))
                throw std::runtime_error("Cannot flush campaign slot");
            fail(CampaignIoStage::Close);
            const auto descriptor = file.value;
            file.value = -1;
            if (::close(descriptor))
                throw std::runtime_error("Cannot close campaign slot");
            fail(CampaignIoStage::Rename);
            check();
            if (::rename(temp.c_str(), path_.c_str()))
                throw std::runtime_error("Cannot replace campaign slot");
            result.published = result.exists = true;
            fail(CampaignIoStage::DirectoryFlush);
            Fd directory{::open(path_.parent_path().c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC)};
            if (directory.value < 0 || ::fsync(directory.value))
                throw std::runtime_error("Campaign published; directory durability unconfirmed");
            result.durable = true;
        } catch (...) {
            if (!result.published)
                ::unlink(temp.c_str());
            throw;
        }
    } catch (const std::exception& e) {
        result.error = campaignError(e.what());
    }
    return result;
}
CampaignStore::CampaignStore(const rules::Catalog& c, const NpcDefinitions& d, CampaignContent content,
                             CampaignFiles& files)
    : catalog_(c), definitions_(d), content_(std::move(content)), files_(files) {}
CampaignIoReply CampaignStore::read() {
    auto reply = files_.exchange({"read", {}, {}});
    if (!reply.error.empty())
        throw std::runtime_error(reply.error);
    return reply;
}
std::string CampaignStore::synchronizeIdentity(const std::string& proposed) {
    auto reply = files_.exchange({"identity", proposed, {}});
    if (!reply.error.empty())
        throw std::runtime_error(reply.error);
    if (reply.data.size() != 32 || reply.data.find_first_not_of("0123456789abcdef") != std::string::npos)
        throw std::runtime_error("Invalid campaign run identity");
    return reply.data;
}
CampaignSlot CampaignStore::inspect() {
    CampaignSlot out;
    try {
        auto r = read();
        out.exists = r.exists;
        if (!r.exists) {
            out.error = "No campaign save";
            return out;
        }
        out.revision = campaignRevision(r.data);
        if (uncertainTerminal_) {
            out.terminal = true;
            out.error = "Terminal save status unresolved; retry the marker or explicitly save a new run";
            return out;
        }
        if (terminalRecord(r.data, out.runId)) {
            out.terminal = true;
            out.error = "Saved expedition has ended";
            return out;
        }
        auto s = decodeCampaign(catalog_, definitions_, content_, r.data);
        out.runId = s->runId;
        if (deniedRun_ == out.runId) {
            out.terminal = true;
            out.error = "This expedition ended in this process";
            return out;
        }
        auto run = std::make_unique<CampaignRun>(catalog_, std::move(*s), definitions_.timings);
        out.active = true;
        out.depth = run->activeLocation();
        out.location = run->name();
        for (const auto& member : run->party().snapshot().characters) {
            if (!out.partySummary.empty())
                out.partySummary += ", ";
            out.partySummary += member.build.name;
        }
    } catch (const std::exception& e) {
        out.error = campaignError(e.what());
    }
    return out;
}
std::unique_ptr<CampaignRun> CampaignStore::load() {
    if (uncertainTerminal_)
        throw std::invalid_argument("Terminal save status unresolved; retry the marker");
    auto r = read();
    std::string id;
    if (!r.exists)
        throw std::invalid_argument("No campaign save");
    if (terminalRecord(r.data, id))
        throw std::invalid_argument("Saved expedition has ended");
    auto s = decodeCampaign(catalog_, definitions_, content_, r.data);
    if (deniedRun_ == s->runId)
        throw std::invalid_argument("This expedition ended in this process");
    return std::make_unique<CampaignRun>(catalog_, std::move(*s), definitions_.timings);
}
CampaignWrite CampaignStore::save(CampaignRun& run, const std::string& expected, bool replaceDifferent) {
    auto s = run.checkpoint();
    if (deniedRun_ == run.runId())
        throw std::invalid_argument("Campaign has ended");
    auto current = read();
    if ((current.exists ? campaignRevision(current.data) : "missing") != expected)
        throw std::invalid_argument("Campaign slot changed; review it before retrying");
    if (current.exists && !replaceDifferent) {
        auto info = inspect();
        if (!info.active || info.runId != run.runId())
            throw std::invalid_argument("Confirm replacing the previous campaign save");
    }
    auto text = encodeCampaign(catalog_, definitions_, content_, *s);
    // A save must be restorable before replacing the only good slot. This staged
    // owner borrows no live Party, consumes no live RNG and never publishes IDs.
    auto admission = std::make_unique<CampaignRun>(catalog_, std::move(*s), definitions_.timings);
    auto result = files_.exchange({"replace", text, expected});
    if (result.published) {
        deniedRun_.clear();
        uncertainTerminal_ = false;
    }
    return {result.published, result.durable, result.published ? campaignRevision(text) : expected,
            result.error};
}
CampaignWrite CampaignStore::terminal(const CampaignRun& run) {
    if (run.status() == CampaignStatus::Active)
        throw std::invalid_argument("Campaign is still active");
    uncertainTerminal_ = true; // Fail closed even when reading/allocating the marker fails.
    try {
        if (deniedRun_.empty())
            deniedRun_ = run.runId();
        auto current = read();
        uncertainTerminal_ = false;
        if (!current.exists)
            return {false, true, "missing", {}};
        std::string id;
        if (terminalRecord(current.data, id))
            return {false, true, campaignRevision(current.data), {}};
        auto s = decodeCampaign(catalog_, definitions_, content_, current.data);
        if (s->runId != run.runId())
            return {false, true, campaignRevision(current.data), {}};
        deniedRun_ = run.runId();
        const auto text = Json{{"terminal_schema", 1},
                               {"run_id", run.runId()},
                               {"status", run.status() == CampaignStatus::Victory ? "victory" : "defeat"}}
                              .dump() +
                          "\n";
        auto result = files_.exchange({"replace", text, campaignRevision(current.data)});
        return {result.published, result.durable,
                result.published ? campaignRevision(text) : campaignRevision(current.data), result.error};
    } catch (const std::exception& e) {
        uncertainTerminal_ = true;
        return {false, false, {}, campaignError(e.what())};
    }
}
} // namespace blobber

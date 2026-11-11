#include "persistence/campaign_store.hpp"
#include "assets/fingerprint.hpp"
#include "persistence/character_library.hpp"
#include "platform/file_io.hpp"
#include <nlohmann/json.hpp>
#include <set>
namespace blobber {
namespace {
using Json = nlohmann::json;
CampaignIoReply diskRead(const std::filesystem::path& path) {
    auto read = disk::read(path, campaignMaxBytes, true);
    CampaignIoReply out;
    out.exists = read.exists;
    out.data = std::move(read.bytes);
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
        !j["terminal_schema"].is_number_integer() || j["terminal_schema"] != 2 || !j.contains("run_id") ||
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
        bool busy = false;
        auto lock = disk::File::lock(disk::suffix(path_, ".lock"), true, busy);
        auto check = [&] {
            auto current = diskRead(path_);
            if ((current.exists ? campaignRevision(current.data) : "missing") != request.expected)
                throw std::runtime_error("Campaign slot changed; review it before retrying");
        };
        check();
        disk::Temporary temp(path_.parent_path() / ".campaign-", true);
        fail(CampaignIoStage::Write);
        temp.file.write(request.data);
        fail(CampaignIoStage::Flush);
        temp.file.flush();
        fail(CampaignIoStage::Close);
        temp.file.close();
        fail(CampaignIoStage::Rename);
        check();
        temp.publish(path_);
        result.published = result.exists = true;
        fail(CampaignIoStage::DirectoryFlush);
        disk::confirm(path_);
        result.durable = true;
    } catch (const std::exception& e) {
        result.error = campaignError(e.what());
    }
    return result;
}
CampaignStore::CampaignStore(const rules::Catalog& c, const NpcDefinitions& d, CampaignContent content,
                             CampaignFiles& files)
    : catalog_(c), definitions_(d), content_(std::move(content)), files_(files) {
    if (!content_.blueprints)
        throw std::invalid_argument("Missing blueprint catalog");
}
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
        const auto summary = inspectCampaign(content_, r.data);
        out.runId = summary.runId;
        if (deniedRun_ == out.runId) {
            out.terminal = true;
            out.error = "This expedition ended in this process";
            return out;
        }
        out.active = true;
        out.depth = dungeonLocation(summary.location) ? summary.location : 0;
        constexpr const char* names[]{"Sanctuary",       "Dungeon floor 1", "Dungeon floor 2",
                                      "Dungeon floor 3", "Sanctuary inn",   "Sanctuary smithy",
                                      "Arcane Emporium"};
        out.location = names[summary.location];
        out.partySummary = summary.party;
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
    return std::make_unique<CampaignRun>(*content_.blueprints, catalog_, std::move(*s), definitions_.timings);
}
CampaignWrite CampaignStore::save(CampaignRun& run, const std::string& expected, bool replaceDifferent) {
    auto s = run.captureSave();
    if (deniedRun_ == run.runId())
        throw std::invalid_argument("Campaign has ended");
    auto current = read();
    if ((current.exists ? campaignRevision(current.data) : "missing") != expected)
        throw std::invalid_argument("Campaign slot changed; review it before retrying");
    if (current.exists && !replaceDifferent) {
        bool sameRun = false;
        try {
            sameRun = inspectCampaign(content_, current.data).runId == run.runId();
        } catch (const std::exception&) {
            // Invalid or incompatible slots require explicit replacement.
        }
        if (!sameRun)
            throw std::invalid_argument("Confirm replacing the previous campaign save");
    }
    auto text = encodeCampaign(catalog_, definitions_, content_, *s);
    auto result = files_.exchange({"replace", text, expected});
    if (result.published) {
        deniedRun_.clear();
        uncertainTerminal_ = false;
    }
    return {result.published, result.durable, result.published ? campaignRevision(text) : expected,
            result.error};
}
CampaignWrite CampaignStore::terminal(const CampaignRun& run) {
    return terminal(CampaignTerminalRequest{run.runId(), run.status()});
}
CampaignWrite CampaignStore::terminal(const CampaignTerminalRequest& request) {
    if (request.status == CampaignStatus::Active)
        throw std::invalid_argument("Campaign is still active");
    uncertainTerminal_ = true; // Fail closed even when reading/allocating the marker fails.
    try {
        if (deniedRun_.empty())
            deniedRun_ = request.runId;
        auto current = read();
        uncertainTerminal_ = false;
        if (!current.exists)
            return {false, true, "missing", {}};
        std::string id;
        if (terminalRecord(current.data, id))
            return {false, true, campaignRevision(current.data), {}};
        // Association must not depend on whether an older save is still restorable.
        // An unsaved expedition has no authority to replace another run's slot.
        if (savedCampaignRunId(current.data) != request.runId)
            return {false, true, campaignRevision(current.data), {}};
        deniedRun_ = request.runId;
        const auto text = Json{{"terminal_schema", 2},
                               {"run_id", request.runId},
                               {"status", request.status == CampaignStatus::Victory ? "victory" : "defeat"}}
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

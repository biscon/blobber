#pragma once
#include "persistence/campaign_codec.hpp"
#include <filesystem>
namespace blobber {
struct CampaignIoRequest {
    std::string operation, data, expected;
};
struct CampaignIoReply {
    std::string data, error;
    bool exists = false, published = false, durable = false;
};
// Only preparation/I/O boundaries call this interface. Replay substitutes recorded I/O.
class CampaignFiles {
  public:
    virtual ~CampaignFiles() = default;
    virtual CampaignIoReply exchange(const CampaignIoRequest&) = 0;
};
enum class CampaignIoStage { Read, Write, Flush, Close, Rename, DirectoryFlush };
struct CampaignIoHooks {
    bool (*fail)(CampaignIoStage) = nullptr;
};
class CampaignDisk final : public CampaignFiles {
  public:
    CampaignDisk(std::filesystem::path, CampaignIoHooks = {});
    CampaignIoReply exchange(const CampaignIoRequest&) override;

  private:
    std::filesystem::path path_;
    CampaignIoHooks hooks_;
};
struct CampaignSlot {
    std::string revision = "missing", runId, location, partySummary, error;
    bool exists = false, active = false, terminal = false;
    unsigned depth = 0;
};
struct CampaignWrite {
    bool published = false, durable = false;
    std::string revision, error;
};
std::string campaignError(const char*); // Bounded ASCII-safe errors, including corrupt UTF-8.
std::string campaignRevision(const std::string&);
std::filesystem::path campaignSavePath(const std::filesystem::path& overridePath = {});
class CampaignStore {
  public:
    CampaignStore(const rules::Catalog&, const NpcDefinitions&, CampaignContent, CampaignFiles&);
    CampaignSlot inspect();
    std::unique_ptr<CampaignRun> load();
    CampaignWrite save(CampaignRun&, const std::string& expected, bool replaceDifferent = false);
    CampaignWrite terminal(const CampaignRun&);
    std::string synchronizeIdentity(const std::string& proposed);

  private:
    CampaignIoReply read();
    const rules::Catalog& catalog_;
    const NpcDefinitions& definitions_;
    CampaignContent content_;
    CampaignFiles& files_;
    // One slot needs one denied association; no unbounded history of ended runs.
    std::string deniedRun_;
    bool uncertainTerminal_ = false;
};
} // namespace blobber

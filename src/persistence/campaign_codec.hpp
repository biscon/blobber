#pragma once
#include "assets/npc_metadata.hpp"
#include "game/campaign.hpp"
namespace blobber {
constexpr std::size_t campaignMaxBytes = 16u * 1024u * 1024u;
struct CampaignContent {
    std::string catalog;
    std::uint64_t npcs = 0;
};
// Read association without requiring current save/content versions. Never restores state.
std::string savedCampaignRunId(const std::string&);
struct CampaignSummary {
    std::string runId, party;
    unsigned location = 0;
};
CampaignSummary inspectCampaign(const CampaignContent&, const std::string&);
std::string encodeCampaign(const rules::Catalog&, const NpcDefinitions&, const CampaignContent&,
                           CampaignSaveData&);
std::string encodeCampaign(const rules::Catalog&, const NpcDefinitions&, const CampaignContent&,
                           CampaignCheckpoint&);
std::unique_ptr<CampaignCheckpoint> decodeCampaign(const rules::Catalog&, const NpcDefinitions&,
                                                   const CampaignContent&, const std::string&);
} // namespace blobber

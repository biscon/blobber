#pragma once
#include "assets/npc_metadata.hpp"
#include "game/campaign.hpp"
namespace blobber {
constexpr std::size_t campaignMaxBytes = 16u * 1024u * 1024u;
struct CampaignContent {
    std::string catalog;
    std::uint64_t npcs = 0;
};
std::string encodeCampaign(const rules::Catalog&, const NpcDefinitions&, const CampaignContent&,
                           CampaignCheckpoint&);
std::unique_ptr<CampaignCheckpoint> decodeCampaign(const rules::Catalog&, const NpcDefinitions&,
                                                   const CampaignContent&, const std::string&);
} // namespace blobber

#pragma once
#include "assets/npc_metadata.hpp"
#include "automation/trace.hpp"
#include <vector>
namespace blobber::automation {
// Verify first; caller supplies a fresh Protocol for the capture pass.
void captureReplay(Protocol&, const std::filesystem::path& trace, const Json& header,
                   const std::filesystem::path& assets, const NpcDefinitions&,
                   const std::filesystem::path& output, const std::vector<std::size_t>& operations,
                   TraceCampaignFiles* files = nullptr);
} // namespace blobber::automation

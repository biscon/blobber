#pragma once
#include "game/sanctuary_hub.hpp"
#include "world/npc.hpp"
namespace blobber {
inline constexpr const char* innInteriorIdentity = "sanctuary-inn-v1";
// Preparation-only authored content. Entry faces into the room; exit faces its door.
SanctuaryHub innInterior();
NpcInstance innkeeperPlacement(const std::vector<std::string>& types);
void validateInnInterior(const Map&, const CampaignLocationState&);
} // namespace blobber

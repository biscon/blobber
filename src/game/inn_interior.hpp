#pragma once
#include "game/sanctuary_hub.hpp"
namespace blobber {
inline constexpr const char* innInteriorIdentity = "sanctuary-inn-v1";
// Preparation-only authored content. Entry faces into the room; exit faces its door.
SanctuaryHub innInterior();
void validateInnInterior(const Map&, const CampaignLocationState&);
} // namespace blobber

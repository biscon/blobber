#pragma once
#include "game/sanctuary_hub.hpp"
namespace blobber {
inline constexpr const char* emporiumInteriorIdentity = "sanctuary-emporium-v1";
inline constexpr Player emporiumCustomer{{6, 5}, Heading::North};
inline constexpr Cell emporiumResidentCell{6, 3}, emporiumResidentSubcell{1, 1};
SanctuaryHub emporiumInterior();
NpcInstance shadowtartPlacement(const std::vector<std::string>& types);
void validateEmporiumInterior(const Map&, const CampaignLocationState&);
} // namespace blobber

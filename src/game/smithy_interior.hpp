#pragma once
#include "game/sanctuary_hub.hpp"
namespace blobber {
inline constexpr const char* smithyInteriorIdentity = "sanctuary-smithy-v1";
inline constexpr Player smithyCustomer{{6, 5}, Heading::North};
inline constexpr Cell smithyResidentCell{6, 3}, smithyResidentSubcell{1, 1};
SanctuaryHub smithyInterior();
NpcInstance blacksmithPlacement(const std::vector<std::string>& types);
void validateSmithyInterior(const Map&, const CampaignLocationState&);
} // namespace blobber

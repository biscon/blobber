#pragma once
#include "game/movement.hpp"
#include "game/recovery.hpp"
#include "world/fixtures.hpp"
namespace blobber {
inline constexpr const char* sanctuaryHubIdentity = "sanctuary-city-v2";
struct CampaignLocationState;
struct SanctuaryHub {
    Map map;
    Player spawn, entry, exit;
    FloorFixtures fixtures;
    RecoveryLevel recovery;
};
// Preparation-only owned definition. No assets are loaded by these CPU functions.
SanctuaryHub sanctuaryHub();
void validateSanctuaryHub(const Map&, const CampaignLocationState&);
} // namespace blobber

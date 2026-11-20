#pragma once
#include "game/inventory.hpp"
#include "world/fixtures.hpp"
namespace blobber {
enum class CampaignLocationId : std::uint8_t {
    City,
    FloorOne,
    FloorTwo,
    FloorThree,
    Inn,
    Smithy,
    Emporium,
    Seep,
    Galleries,
    Vault,
    Count
};
constexpr unsigned campaignLocationCount = unsigned(CampaignLocationId::Count);
struct DungeonDestination {
    CampaignLocationId location;
    const char* name;
    const char* stairLabel;
    unsigned entryLevel, entryPartyXp;
};
inline constexpr DungeonDestination dungeonFloors[]{
    {CampaignLocationId::FloorOne, "Dungeon floor 1", "Enter floor 1.", 1, 0},
    {CampaignLocationId::FloorTwo, "Dungeon floor 2", "Enter floor 2.", 1, 750},
    {CampaignLocationId::FloorThree, "Dungeon floor 3", "Enter floor 3.", 2, 1650},
    {CampaignLocationId::Seep, "The Seep", "Enter The Seep.", 2, 2700},
    {CampaignLocationId::Galleries, "The Drowned Galleries", "Enter The Drowned Galleries.", 2, 3400},
    {CampaignLocationId::Vault, "The Brood Vault", "Enter The Brood Vault.", 3, 4650}};
constexpr unsigned dungeonFloorCount = std::size(dungeonFloors);
constexpr unsigned floorLocation(unsigned depth) noexcept {
    return depth && depth <= dungeonFloorCount ? unsigned(dungeonFloors[depth - 1].location)
                                               : campaignLocationCount;
}
constexpr unsigned locationDepth(unsigned location) noexcept {
    for (unsigned depth = 1; depth <= dungeonFloorCount; ++depth)
        if (floorLocation(depth) == location)
            return depth;
    return 0;
}
constexpr bool dungeonLocation(unsigned location) noexcept {
    return locationDepth(location) != 0;
}
inline const char* campaignLocationName(unsigned location) noexcept {
    const unsigned depth = locationDepth(location);
    if (depth)
        return dungeonFloors[depth - 1].name;
    switch (CampaignLocationId(location)) {
    case CampaignLocationId::City:
        return "Sanctuary";
    case CampaignLocationId::Inn:
        return "Sanctuary inn";
    case CampaignLocationId::Smithy:
        return "Sanctuary smithy";
    case CampaignLocationId::Emporium:
        return "Arcane Emporium";
    default:
        return "Unknown location";
    }
}
constexpr bool interiorLocation(CampaignLocationId id) noexcept {
    return id == CampaignLocationId::Inn || id == CampaignLocationId::Smithy ||
           id == CampaignLocationId::Emporium;
}
struct DoorwayId {
    std::uint64_t world = 0;
    std::uint16_t slot = UINT16_MAX;
};
enum class TravelKind : std::uint8_t { Stairs, Recall, Waypoint, Doorway };
inline const char* travelKindName(TravelKind kind) noexcept {
    switch (kind) {
    case TravelKind::Doorway:
        return "doorway";
    case TravelKind::Stairs:
        return "stairs";
    case TravelKind::Recall:
        return "recall";
    case TravelKind::Waypoint:
        return "waypoint";
    }
    return "invalid";
}
struct TravelRequest {
    std::uint64_t revision = 0, world = 0;
    TravelKind kind = TravelKind::Stairs;
    FixtureRequest fixture;
    DoorwayId doorway;
    unsigned depth = 0;
    ItemHandle supply;
    std::uint64_t inventoryRevision = 0;
};
} // namespace blobber

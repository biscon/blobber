#pragma once
#include "game/inventory.hpp"
#include "world/fixtures.hpp"
namespace blobber {
enum class TravelKind : std::uint8_t { Stairs, Recall, Waypoint };
inline const char* travelKindName(TravelKind kind) noexcept {
    switch (kind) {
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
    unsigned depth = 0;
    ItemHandle supply;
    std::uint64_t inventoryRevision = 0;
};
} // namespace blobber

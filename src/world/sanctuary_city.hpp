#pragma once
#include "game/movement.hpp"
#include "world/map.hpp"
#include <array>
#include <string_view>
namespace blobber {
constexpr unsigned cityDoorwayCapacity = 16, cityReservationCapacity = 16;
struct CityDoorway {
    const char* name;
    unsigned placement;
    Player approach;
};
struct CityReservation {
    const char* name;
    Cell origin;
    unsigned width, depth;
    bool contains(Cell c) const noexcept {
        return c.x >= origin.x && c.y >= origin.y && c.x < origin.x + int(width) &&
               c.y < origin.y + int(depth);
    }
};
struct CityLoop {
    const char* name;
    std::array<Cell, 5> corners; // Closed cardinal rectangle; no frame-time routing.
};
struct SanctuaryCityReview {
    Map map;
    Player spawn;
    std::array<CityDoorway, cityDoorwayCapacity> doors{};
    unsigned doorCount = 0;
    std::array<CityReservation, cityReservationCapacity> reservations{};
    unsigned reservationCount = 0;
    Player rest{}, dungeonLanding{}, gate{};
    Cell dungeonFixture{};
    CityReservation gateApproach{};
    std::array<CityLoop, 2> loops{};
    CityReservation alley{};
    const CityDoorway& doorway(std::string_view name) const;
};
// Preparation-only validation; may allocate. Metadata never enters the save schema.
void validateSanctuaryCity(const SanctuaryCityReview& city);
// Development-only authored scene; never substitutes the campaign hub.
SanctuaryCityReview sanctuaryCityReview();
} // namespace blobber

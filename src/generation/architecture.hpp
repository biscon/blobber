#pragma once
#include "generation/floor.hpp"
namespace blobber::generation {
// Preparation-only helpers. Layout must have passed validate(); dressing is fresh.
using RoomCeilings = std::array<std::int16_t, maxRooms>;
RoomCeilings roomCeilings(const FloorLayout&) noexcept;
void prepareArchitecture(const FloorLayout&, WorldDefinition&);
} // namespace blobber::generation

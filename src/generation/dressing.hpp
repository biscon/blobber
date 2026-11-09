#pragma once
#include "generation/floor.hpp"
namespace blobber::generation {
// Preparation-only admission: room membership, clear supporting area and local bypass.
// Reservations include existing furniture, even walkable chest cells.
bool blockingPropFitsRoom(const FloorLayout&, const Decoration&, const CellMask& reserved) noexcept;
// Preparation-only. Adds independent cosmetic choices and validated furniture.
bool dressUnderkeep(FloorLayout&, GenerationRequest, WorldDefinition&,
                    const BlueprintReservations* = nullptr);
} // namespace blobber::generation

#pragma once
#include "generation/floor.hpp"
namespace blobber::generation {
struct BlueprintReservations {
    CellMask blocked{}, furnished{}, lanes{}, encounters{}, rewards{};
    std::array<blueprints::Reward, maxRooms> reward{};
    std::array<bool, maxRooms> authored{};
};
const BlueprintInstance* blueprintAt(const FloorLayout&, unsigned room) noexcept;
// Catalog validation and the exact four-purpose roster. Throws for invalid content.
void validateBlueprintRoster(const blueprints::BlueprintCatalog&);
// Derive masks and optionally emit scenery, after geometry and sockets are final.
Failure prepareBlueprintRooms(const FloorLayout&, const blueprints::BlueprintCatalog&, BlueprintReservations&,
                              WorldDefinition* = nullptr);
Failure validateBlueprintWorld(const FloorLayout&, const blueprints::BlueprintCatalog&, const Map&);
unsigned roomDistance(const FloorLayout&, unsigned a, unsigned b) noexcept;
} // namespace blobber::generation

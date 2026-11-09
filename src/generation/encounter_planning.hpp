#pragma once
#include "generation/floor.hpp"
namespace blobber::generation {
struct DepthBand {
    std::array<EncounterRecipe, 5> ordinary{};
    unsigned count = 0, optionalCount = 0;
    EncounterRecipe optional{}, guardian{};
};
const DepthBand& encounterBand(unsigned depth) noexcept;
// Preparation-only planners. Failure leaves the candidate unpublished.
bool planEncounterSites(FloorLayout&, const DepthBand&, std::array<unsigned, 7>& rooms,
                        const BlueprintReservations* = nullptr) noexcept;
bool placeEncounterFormation(FloorLayout&, unsigned room, EncounterRecipe, EncounterRole, Heading,
                             const BlueprintReservations* = nullptr, bool dryRun = false) noexcept;
} // namespace blobber::generation

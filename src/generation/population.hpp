#pragma once
#include "game/floor_objective.hpp"
#include "generation/floor.hpp"
namespace blobber::generation {
struct PopulationDefinition {
    std::array<NpcInstance, 32> actors{};
    unsigned count = 0;
    RecoveryLevel recovery;
    FloorObjectiveDefinition objective;
};
PopulationDefinition resolvePopulation(const FloorLayout&, GenerationRequest, const rules::Catalog&,
                                       const std::vector<std::string>& types);
struct PreparedPopulation {
    NpcWorld placements;
    RecoveryLevel recovery;
    FloorObjectiveDefinition objective;
};
PreparedPopulation preparePopulation(const GeneratedFloor&, const rules::Catalog&,
                                     const std::vector<std::string>& types);
} // namespace blobber::generation

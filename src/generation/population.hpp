#pragma once
#include "game/floor_objective.hpp"
#include "generation/floor.hpp"
namespace blobber::generation {
struct PreparedPopulation {
    NpcWorld placements;
    RecoveryLevel recovery;
    FloorObjectiveDefinition objective;
};
PreparedPopulation preparePopulation(const GeneratedFloor&, const rules::Catalog&,
                                     const std::vector<std::string>& types);
} // namespace blobber::generation

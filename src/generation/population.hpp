#pragma once
#include "game/floor_objective.hpp"
#include "generation/floor.hpp"
namespace blobber::generation {
struct RewardProfile {
    rules::WeaponTraining weapons = rules::allWeaponTraining;
    bool spellAttack = true, manaCaster = true, unarmored = true;
};
RewardProfile rewardProfile(const rules::Catalog&, const rules::PresetParty&) noexcept;
struct PopulationDefinition {
    std::array<NpcInstance, 32> actors{};
    unsigned count = 0;
    RecoveryLevel recovery;
    FloorObjectiveDefinition objective;
};
PopulationDefinition resolvePopulation(const FloorLayout&, GenerationRequest, const rules::Catalog&,
                                       const std::vector<std::string>& types, RewardProfile profile = {});
struct PreparedPopulation {
    NpcWorld placements;
    RecoveryLevel recovery;
    FloorObjectiveDefinition objective;
};
PreparedPopulation preparePopulation(const GeneratedFloor&, const rules::Catalog&,
                                     const std::vector<std::string>& types, RewardProfile profile = {});
} // namespace blobber::generation

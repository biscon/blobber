#include "generation/population.hpp"
#include <algorithm>
#include <stdexcept>
namespace blobber::generation {
PreparedPopulation preparePopulation(const GeneratedFloor& f, const rules::Catalog& catalog,
                                     const std::vector<std::string>& types) {
    if (const auto error = validate(f.layout))
        throw std::invalid_argument(errorName(error.code));
    constexpr const char* names[]{"goblin", "goblin.archer", "goblin.guardian.placeholder"};
    std::vector<NpcInstance> instances;
    instances.reserve(f.layout.populationCount);
    FloorObjectiveDefinition objective;
    objective.approachDoor = f.layout.approachDoor;
    objective.gate = f.layout.guardianGate;
    objective.lootSeed = std::uint32_t(seedFor(f.request, Domain::Reward));
    for (unsigned i = 0; i < f.layout.populationCount; ++i) {
        const auto& p = f.layout.population[i];
        const auto name = names[unsigned(p.recipe)];
        const auto monster = catalog.findMonster(name);
        const auto appearance = std::find(types.begin(), types.end(), monsterAppearance(name));
        if (!catalog.monster(monster) || appearance == types.end())
            throw std::invalid_argument("Generated monster or appearance unavailable");
        NpcInstance npc{{std::uint16_t(appearance - types.begin())}, p.cell, p.subcell, p.facing};
        npc.monster = monster;
        instances.push_back(npc);
        if (p.guardian)
            objective.members[objective.count++] = std::uint16_t(i);
    }
    const auto item = [&](const char* id) {
        const auto value = catalog.findItem(id);
        if (!catalog.item(value))
            throw std::invalid_argument("Generated supply unavailable");
        return value;
    };
    RecoveryLevel recovery;
    recovery.ground[recovery.groundCount++] = {f.layout.optionalSupply, item("healing_potion"), 1};
    recovery.ground[recovery.groundCount++] = {f.layout.optionalSupply, item("mana_potion"), 1};
    objective.rewards[objective.rewardCount++] = {f.layout.guardianReward, item("healing_potion"), 1};
    objective.rewards[objective.rewardCount++] = {f.layout.guardianReward, item("camp_kit"), 1};
    return {NpcWorld(f.map, f.layout.spawn.cell, types, std::move(instances)), recovery, objective};
}
} // namespace blobber::generation

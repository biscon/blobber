#include "generation/population.hpp"
#include <algorithm>
#include <stdexcept>
namespace blobber::generation {
PopulationDefinition resolvePopulation(const FloorLayout& layout, GenerationRequest request,
                                       const rules::Catalog& catalog, const std::vector<std::string>& types) {
    if (const auto error = validate(layout))
        throw std::invalid_argument(errorName(error.code));
    PopulationDefinition out;
    auto& objective = out.objective;
    objective.approachDoor = layout.approachDoor;
    objective.gate = layout.guardianGate;
    objective.lootSeed = std::uint32_t(seedFor(request, Domain::Reward));
    for (unsigned i = 0; i < layout.populationCount; ++i) {
        const auto& p = layout.population[i];
        const auto name = speciesName(p.recipe);
        const auto monster = catalog.findMonster(name);
        const auto appearance = std::find(types.begin(), types.end(), name);
        if (!catalog.monster(monster) || appearance == types.end())
            throw std::invalid_argument("Generated monster or appearance unavailable");
        NpcInstance npc{{std::uint16_t(appearance - types.begin())}, p.cell, p.subcell, p.facing};
        npc.monster = monster;
        out.actors[out.count++] = npc;
        if (p.guardian)
            objective.members[objective.count++] = std::uint16_t(i);
    }
    const auto item = [&](const char* id) {
        const auto value = catalog.findItem(id);
        if (!catalog.item(value))
            throw std::invalid_argument("Generated supply unavailable");
        return value;
    };
    auto& recovery = out.recovery;
    recovery.ground[recovery.groundCount++] = {layout.supply, item("healing_potion"), 1};
    recovery.ground[recovery.groundCount++] = {layout.supply, item("mana_potion"), 2};
    recovery.ground[recovery.groundCount++] = {layout.supply, item("arrow"), 20};
    recovery.ground[recovery.groundCount++] = {layout.optionalSupply, item("healing_potion"), 1};
    recovery.ground[recovery.groundCount++] = {layout.optionalSupply, item("mana_potion"), 1};
    recovery.ground[recovery.groundCount++] = {layout.optionalSupply, item("arrow"), 10};
    if (layout.depth < 3) {
        objective.rewards[objective.rewardCount++] = {layout.guardianReward, item("camp_kit"), 1};
    }
    return out;
}
PreparedPopulation preparePopulation(const GeneratedFloor& f, const rules::Catalog& catalog,
                                     const std::vector<std::string>& types) {
    const auto definition = resolvePopulation(f.layout, f.request, catalog, types);
    std::vector<NpcInstance> actors(definition.actors.begin(), definition.actors.begin() + definition.count);
    return {NpcWorld(f.map, f.layout.spawn.cell, types, std::move(actors)), definition.recovery,
            definition.objective};
}
} // namespace blobber::generation

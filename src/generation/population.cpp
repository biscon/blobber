#include "generation/population.hpp"
#include <algorithm>
#include <stdexcept>
namespace blobber::generation {
RewardProfile rewardProfile(const rules::Catalog& catalog, const rules::PresetParty& party) noexcept {
    RewardProfile out{0, false, false, false};
    for (const auto& c : party.characters) {
        const auto* cls = catalog.characterClass(c.characterClass);
        const auto* race = catalog.race(c.race);
        if (!cls || !race)
            continue;
        out.weapons |= cls->weapons | race->weapons;
        out.manaCaster |= cls->casting != rules::Casting::None;
        out.spellAttack |= cls->casting != rules::Casting::None;
        for (unsigned i = 0; i < catalog.spellCount(); ++i)
            if ((c.spells.racial & (1u << i)) &&
                rules::spellRules(catalog.spell({std::uint8_t(i)})->kind)->attack)
                out.spellAttack = true;
        out.unarmored |= c.equipment.body.index == UINT16_MAX && c.equipment.offHand.index == UINT16_MAX;
    }
    return out;
}
namespace {
rules::ItemId wearableReward(const rules::Catalog& catalog, GenerationRequest request,
                             RewardProfile profile) {
    std::array<const char*, 3> pool{};
    unsigned count = 0;
    if (request.depth == 1)
        pool = {"boots_of_balance", "belt_of_vigor", "ring_of_resolve"}, count = 3;
    else if (request.depth == 2) {
        if (profile.spellAttack)
            pool[count++] = "circlet_of_focus";
        pool[count++] = "gloves_of_striking";
        if (profile.unarmored)
            pool[count++] = "apprentice_bracers";
    } else {
        pool[count++] = "cloak_of_protection";
        if (profile.manaCaster)
            pool[count++] = "amulet_of_reserves";
    }
    const auto id = catalog.findItem(pool[seedFor(request, Domain::Reward, 100) % count]);
    if (!catalog.item(id))
        throw std::invalid_argument("Missing wearable reward content");
    return id;
}
rules::ItemId weaponReward(const rules::Catalog& catalog, GenerationRequest request, RewardProfile profile) {
    std::array<rules::ItemId, 6> pool{};
    unsigned count = 0;
    const auto add = [&](const char* name) {
        const auto id = catalog.findItem(name);
        const auto* item = catalog.item(id);
        if (!item)
            throw std::invalid_argument("Missing magic weapon reward content");
        if (profile.weapons & rules::weaponBit(item->weapon))
            pool[count++] = id;
    };
    for (const auto* name : {"longsword_plus1", "mace_plus1", "rapier_plus1", "shortbow_plus1"})
        add(name);
    if (count < 2) {
        add("dagger_plus1");
        add("quarterstaff_plus1");
    }
    if (count < 2)
        throw std::invalid_argument("Starting party has no usable magic weapon rewards");
    const auto depth = request.depth;
    request.depth = 2; // Both floors share one campaign-level draw without replacement.
    const auto first = unsigned(seedFor(request, Domain::Reward, 101) % count);
    std::swap(pool[0], pool[first]);
    const auto second = 1 + unsigned(seedFor(request, Domain::Reward, 102) % (count - 1));
    std::swap(pool[1], pool[second]);
    return pool[depth == 3 ? 1 : 0];
}
} // namespace
PopulationDefinition resolvePopulation(const FloorLayout& layout, GenerationRequest request,
                                       const rules::Catalog& catalog, const std::vector<std::string>& types,
                                       RewardProfile profile) {
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
        const auto appearance = std::find(types.begin(), types.end(), monsterAppearance(name));
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
    if (!floorSpec(layout.depth)->finalFloor) {
        objective.rewards[objective.rewardCount++] = {layout.guardianReward, item("camp_kit"), 1};
    }
    // Rules-only test/tool catalogs may deliberately omit the optional magic expansion.
    if (catalog.item(catalog.findItem("cloak_of_protection"))) {
        objective.rewards[objective.rewardCount++] = {layout.guardianReward,
                                                      wearableReward(catalog, request, profile), 1};
        if (layout.depth >= 2)
            recovery.ground[recovery.groundCount++] = {layout.optionalSupply,
                                                       weaponReward(catalog, request, profile), 1};
    }
    if (layout.cacheCount) {
        for (unsigned i = 0; i < recovery.groundCount; ++i) {
            auto& p = recovery.ground[i];
            const unsigned owner = p.cell == layout.optionalSupply ? 1 : 0;
            p.cell = layout.caches[owner].cell;
            p.container = std::uint8_t(owner);
        }
        for (unsigned i = 0; i < objective.rewardCount; ++i) {
            objective.rewards[i].cell = layout.caches[2].cell;
            objective.rewards[i].container = 2;
        }
    }
    return out;
}
PreparedPopulation preparePopulation(const GeneratedFloor& f, const rules::Catalog& catalog,
                                     const std::vector<std::string>& types, RewardProfile profile) {
    const auto definition = resolvePopulation(f.layout, f.request, catalog, types, profile);
    std::vector<NpcInstance> actors(definition.actors.begin(), definition.actors.begin() + definition.count);
    return {NpcWorld(f.map, f.layout.spawn.cell, types, std::move(actors)), definition.recovery,
            definition.objective};
}
} // namespace blobber::generation

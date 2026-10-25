#pragma once
#include "game/actors.hpp"

namespace blobber {
inline rules::Catalog testMonsterCatalog(const std::vector<std::string>& names) {
    std::vector<rules::MonsterDefinition> monsters;
    for (const auto& name : names) {
        rules::MonsterDefinition monster;
        monster.id = name;
        monster.abilities.fill(10);
        monster.armorClass = 12;
        monster.hitPoints = 7;
        monster.attackCount = 1;
        monster.attacks[0] = {0};
        monsters.push_back(monster);
    }
    return {{{"fixture.attack", rules::AttackKind::Melee, 2, {1, 4, 0}, rules::DamageType::Piercing}},
            std::move(monsters)};
}
inline const rules::Catalog& testActorCatalog() {
    static const auto catalog = testMonsterCatalog({"test", "near", "far", "friendly-or-hostile"});
    return catalog;
}
// Returned owner borrows the caller's map and the fixture's immutable catalog.
inline ActorWorld testActors(const Map& map, Cell spawn, std::vector<std::string> types,
                             std::vector<NpcInstance> instances) {
    return ActorWorld(map, testActorCatalog(), NpcWorld(map, spawn, std::move(types), std::move(instances)));
}
} // namespace blobber

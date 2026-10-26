#pragma once
#include "party_fixtures.hpp"
#include "world/map.hpp"
namespace blobber {
inline Map hybridRoom() {
    return Map({"#############", "#...........#", "#...........#", "#...........#", "#...........#",
                "#...........#", "#...........#", "#...........#", "#...........#", "#...........#",
                "#...........#", "#...........#", "#############"});
}
inline rules::Catalog hybridCatalog(bool escape = true) {
    auto f = partyFixture();
    f.preset.characters[2].equipment.ranged = {2};
    rules::MonsterDefinition m;
    m.id = "goblin";
    m.abilities = {8, 14, 10, 10, 8, 8};
    m.armorClass = 15;
    m.hitPoints = 1000;
    m.attacks[0] = {0};
    m.attacks[1] = {1};
    m.attackCount = 2;
    m.traits = escape ? static_cast<rules::Traits>(rules::Trait::NimbleEscape) : 0;
    auto archer = m;
    archer.id = "archer";
    archer.attacks[0] = {1};
    archer.attackCount = 1;
    auto swordsman = m;
    swordsman.id = "swordsman";
    swordsman.attackCount = 1;
    return {{{"blade", rules::AttackKind::Melee, -100, {1, 4, -100}, rules::DamageType::Slashing},
             {"bow", rules::AttackKind::Ranged, -100, {1, 4, -100}, rules::DamageType::Piercing}},
            {m, archer, swordsman},
            f.races,
            f.classes,
            f.items,
            f.preset};
}

} // namespace blobber

#pragma once
#include "combat_fixtures.hpp"

namespace blobber::test {
using namespace rules;
inline Catalog newtCatalog() {
    auto f = partyFixture();
    MonsterDefinition newt;
    newt.id = "test.newt";
    newt.hitPoints = 16;
    newt.armorClass = 12;
    newt.speedFeet = 15;
    newt.abilities = {14, 9, 14, 2, 10, 4};
    newt.attacks[0] = {0};
    newt.attackCount = 1;
    newt.traits = static_cast<Traits>(Trait::BlindCaveNewt);
    return {{{"test.bite", AttackKind::Melee, 4, {3, 6, 3}, DamageType::Piercing}},
            {newt},
            f.races,
            f.classes,
            f.items,
            f.preset};
}
} // namespace blobber::test

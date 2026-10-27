#pragma once
#include "game/session.hpp"
#include "inventory_fixtures.hpp"
namespace blobber {
inline rules::Catalog recoveryCatalog(bool kits = true, bool fullLoot = false) {
    auto f = inventoryFixture();
    f.classes[0].features |= static_cast<rules::ClassFeatures>(rules::ClassFeature::SecondWind);
    f.classes[3].features |= static_cast<rules::ClassFeatures>(rules::ClassFeature::ArcaneRecovery);
    for (int i = 0; i < 2; ++i) {
        rules::ItemDefinition d;
        d.id = i ? "kit" : "gold";
        d.name = d.description = d.id;
        d.kind = i ? rules::ItemKind::Supply : rules::ItemKind::Currency;
        d.weight = i ? 800 : 2;
        f.items.push_back(d);
    }
    if (kits)
        f.preset.characters[0].items[f.preset.characters[0].itemCount++] = {{13}, 2};
    rules::MonsterDefinition m;
    m.id = "goblin";
    m.abilities.fill(10);
    m.hitPoints = 7;
    m.armorClass = 12;
    m.attacks[0] = {0};
    m.attackCount = 1;
    m.loot[0] = {{0}, 1};
    m.loot[1] = {{12}, 1, 6};
    m.loot[2] = {{10}, 1, 0, 1, 15};
    m.loot[3] = {{11}, 1, 0, 16, 20};
    m.lootCount = 4;
    if (fullLoot) {
        m.lootCount = 8;
        for (auto& e : m.loot)
            e = {{12}, 1};
    }
    return {{{"hit", rules::AttackKind::Melee, 0, {1, 4, 0}, rules::DamageType::Slashing}},
            {m},
            f.races,
            f.classes,
            f.items,
            f.preset};
}
inline RestRequest restRequest(const GameSession& s, RestKind kind = RestKind::Long) {
    return {kind, {}, s.actors().snapshot().world, s.restToken()};
}
inline unsigned inventoryQuantity(const Party& p, std::uint16_t c, std::uint16_t item) {
    unsigned n = 0;
    for (std::uint16_t i = 0; i < 52; ++i) {
        const auto* stack = p.inventory().stack(p.inventory().at({{c}, i}));
        if (stack && stack->item.index == item)
            n += stack->quantity;
    }
    return n;
}
} // namespace blobber

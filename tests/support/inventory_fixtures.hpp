#pragma once
#include "hybrid_fixtures.hpp"
#include "party_fixtures.hpp"
namespace blobber {
inline PartyFixture inventoryFixture() {
    auto f = partyFixture();
    f.classes[1].casting = rules::Casting::Wisdom;
    f.classes[1].features = static_cast<rules::ClassFeatures>(rules::ClassFeature::LifeDomain);
    f.preset.characters[1].lifeDomain = true;
    f.classes[3].casting = rules::Casting::Intelligence;
    f.preset.characters[2].equipment.ranged = {2};
    for (auto kind : {rules::Consumable::Healing, rules::Consumable::Mana}) {
        rules::ItemDefinition d;
        d.id = kind == rules::Consumable::Healing ? "fixture.healing" : "fixture.mana";
        d.name = d.id;
        d.description = "Deterministic test potion";
        d.kind = rules::ItemKind::Consumable;
        d.consumable = kind;
        d.weight = 50;
        f.items.push_back(d);
    }
    for (auto& c : f.preset.characters) {
        c.items[c.itemCount++] = {{10}, 2};
        c.items[c.itemCount++] = {{11}, 2};
    }
    return f;
}
inline rules::Catalog inventoryCatalog(PartyFixture f = inventoryFixture()) {
    rules::MonsterDefinition m;
    m.id = "goblin";
    m.abilities.fill(10);
    m.hitPoints = 10000;
    m.armorClass = 30;
    m.attacks[0] = {0};
    m.attackCount = 1;
    return {{{"fixture.attack", rules::AttackKind::Melee, -100, {1, 4, 0}, rules::DamageType::Slashing}},
            {m},
            f.races,
            f.classes,
            f.items,
            f.preset};
}
inline ItemHandle findInventoryItem(const Party& p, unsigned character, unsigned definition) {
    for (std::uint16_t cell = 0; cell < 52; ++cell) {
        const auto h = p.inventory().at({{static_cast<std::uint16_t>(character)}, cell});
        const auto* s = p.inventory().stack(h);
        if (s && s->item.index == definition)
            return h;
    }
    return {};
}
inline InventoryRequest inventoryRequest(const Party& p, ItemHandle h, unsigned owner, unsigned cell,
                                         InventoryOp op = InventoryOp::Move, unsigned q = 0) {
    return {op,
            h,
            {{static_cast<std::uint16_t>(owner)}, static_cast<std::uint16_t>(cell)},
            static_cast<std::uint16_t>(q),
            p.inventory().revision()};
}
} // namespace blobber

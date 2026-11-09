#pragma once
#include "rules/content.hpp"
#include "rules/creation.hpp"
namespace blobber::test {
// Dedicated mechanical fixtures, independent of editable game data and artwork.
inline std::vector<rules::ItemDefinition> magicItems() {
    using namespace rules;
    std::vector<ItemDefinition> items;
    const auto add = [&](const char* id, WearSlot slot, EquipmentBonuses bonus) {
        ItemDefinition item;
        item.id = id;
        item.name = id;
        item.description = "Owned magic fixture";
        item.wearSlot = slot;
        item.bonuses = bonus;
        items.push_back(item);
    };
    EquipmentBonuses b;
    b.spellAttack = 1;
    add("circlet_of_focus", WearSlot::Head, b);
    b = {};
    b.meleeDamage = 1;
    add("gloves_of_striking", WearSlot::Hands, b);
    b = {};
    b.initiative = b.dexteritySave = 1;
    add("boots_of_balance", WearSlot::Feet, b);
    b = {};
    b.maxHitPoints = 3;
    add("belt_of_vigor", WearSlot::Belt, b);
    b = {};
    b.armorClass = b.allSaves = 1;
    add("cloak_of_protection", WearSlot::Cloak, b);
    b = {};
    b.allSaves = 1;
    add("ring_of_resolve", WearSlot::Ring, b);
    b = {};
    b.maxMana = 1;
    add("amulet_of_reserves", WearSlot::Amulet, b);
    b = {};
    b.unarmoredArmorClass = 1;
    add("apprentice_bracers", WearSlot::Wrists, b);
    constexpr const char* names[]{"longsword_plus1", "mace_plus1",   "rapier_plus1",
                                  "shortbow_plus1",  "dagger_plus1", "quarterstaff_plus1"};
    constexpr WeaponType types[]{WeaponType::Longsword, WeaponType::Mace,   WeaponType::Rapier,
                                 WeaponType::Shortbow,  WeaponType::Dagger, WeaponType::Quarterstaff};
    for (unsigned i = 0; i < 6; ++i) {
        ItemDefinition item;
        item.id = names[i];
        item.name = names[i];
        item.description = "Owned enchanted weapon";
        item.kind = ItemKind::Weapon;
        item.weapon = types[i];
        item.damage = {1, 6, 0};
        item.enchantment = 1;
        item.finesse = i == 2 || i == 4;
        item.ranged = i == 3;
        item.twoHanded = i == 3;
        item.ammunition = i == 3 ? Ammunition::Arrow : Ammunition::None;
        items.push_back(item);
    }
    return items;
}
inline rules::Catalog withMagicItems(const rules::Catalog& base) {
    using namespace rules;
    std::vector<AttackDefinition> attacks;
    std::vector<MonsterDefinition> monsters;
    std::vector<RaceDefinition> races;
    std::vector<ClassDefinition> classes;
    std::vector<ItemDefinition> items;
    std::vector<SpellDefinition> spells;
    std::vector<CreationClass> creation;
    for (std::uint16_t i = 0; i < base.attackCount(); ++i)
        attacks.push_back(*base.attack({i}));
    for (std::uint16_t i = 0; i < base.monsterCount(); ++i)
        monsters.push_back(*base.monster({i}));
    for (std::uint16_t i = 0; i < base.raceCount(); ++i)
        races.push_back(*base.race({i}));
    for (std::uint16_t i = 0; i < base.classCount(); ++i) {
        classes.push_back(*base.characterClass({i}));
        if (const auto* c = base.creation({i}))
            creation.push_back(*c);
    }
    for (std::uint16_t i = 0; i < base.itemCount(); ++i)
        items.push_back(*base.item({i}));
    for (std::uint16_t i = 0; i < base.spellCount(); ++i)
        spells.push_back(*base.spell({i}));
    const auto extra = magicItems();
    items.insert(items.end(), extra.begin(), extra.end());
    return {attacks, monsters, races,
            classes, items,    base.preset() ? std::optional<PresetParty>(*base.preset()) : std::nullopt,
            spells,  creation};
}
} // namespace blobber::test

#pragma once
#include "rules/content.hpp"

namespace blobber {
// Test-owned mechanical examples. Never read the editable game catalog.
struct PartyFixture {
    std::vector<rules::RaceDefinition> races;
    std::vector<rules::ClassDefinition> classes;
    std::vector<rules::ItemDefinition> items;
    rules::PresetParty preset;
    rules::Catalog catalog(bool withParty = true) const {
        return {{},      {},    races,
                classes, items, withParty ? std::optional<rules::PresetParty>(preset) : std::nullopt};
    }
};
inline PartyFixture partyFixture() {
    using namespace rules;
    PartyFixture f;
    f.races = {{"fixture.human", "Human fixture", {1, 1, 1, 1, 1, 1}, 0, 0},
               {"fixture.dwarf", "Dwarf fixture", {0, 0, 2, 0, 1, 0}, 1, weaponBit(WeaponType::Warhammer)},
               {"fixture.halfling", "Halfling fixture", {0, 2, 0, 0, 0, 1}, 0, 0},
               {"fixture.elf", "Elf fixture", {0, 2, 0, 1, 0, 0}, 0, weaponBit(WeaponType::Shortbow)}};
    f.classes = {{"fixture.fighter",
                  "Fighter fixture",
                  10,
                  {true, false, true, false, false, false},
                  allWeaponTraining,
                  15,
                  true},
                 {"fixture.cleric",
                  "Cleric fixture",
                  8,
                  {false, false, false, false, true, true},
                  simpleWeaponTraining,
                  11,
                  false},
                 {"fixture.rogue",
                  "Rogue fixture",
                  8,
                  {false, true, false, true, false, false},
                  simpleWeaponTraining | weaponBit(WeaponType::Shortsword),
                  1,
                  false},
                 {"fixture.wizard",
                  "Wizard fixture",
                  6,
                  {false, false, false, true, true, false},
                  weaponBit(WeaponType::Dagger),
                  0,
                  false}};
    auto weapon = [&](const char* id, WeaponType type, int sides, DamageType damage, int weight,
                      bool finesse = false, bool ranged = false, bool twoHanded = false, int versatile = 0) {
        ItemDefinition i;
        i.id = id;
        i.name = id;
        i.description = "Test-owned weapon";
        i.kind = ItemKind::Weapon;
        i.weight = weight;
        i.weapon = type;
        i.damage = {1, sides, 0};
        i.damageType = damage;
        i.finesse = finesse;
        i.ranged = ranged;
        i.twoHanded = twoHanded;
        i.versatileSides = versatile;
        f.items.push_back(i);
    };
    weapon("fixture.sword", WeaponType::Longsword, 8, DamageType::Slashing, 300, false, false, false,
           10);                                                                                   // 0
    weapon("fixture.mace", WeaponType::Mace, 6, DamageType::Bludgeoning, 400);                    // 1
    weapon("fixture.short", WeaponType::Shortsword, 6, DamageType::Piercing, 200, true);          // 2
    weapon("fixture.bow", WeaponType::Shortbow, 6, DamageType::Piercing, 200, false, true, true); // 3
    weapon("fixture.dagger", WeaponType::Dagger, 4, DamageType::Piercing, 100, true);             // 4
    for (const auto armor : {ArmorType::Light, ArmorType::Medium, ArmorType::Heavy}) {
        ItemDefinition i;
        i.id = "fixture.armor" + std::to_string(static_cast<int>(armor));
        i.name = i.id;
        i.description = "Test-owned armor";
        i.kind = ItemKind::Armor;
        i.armor = armor;
        i.baseArmorClass = armor == ArmorType::Light ? 11 : armor == ArmorType::Medium ? 14 : 16;
        i.weight = armor == ArmorType::Light ? 1000 : armor == ArmorType::Medium ? 4500 : 5500;
        f.items.push_back(i); // 5 light, 6 medium, 7 heavy
    }
    ItemDefinition shield;
    shield.id = "fixture.shield";
    shield.name = "Shield fixture";
    shield.description = "Test shield";
    shield.kind = ItemKind::Shield;
    shield.weight = 600;
    f.items.push_back(shield); // 8
    ItemDefinition arrow;
    arrow.id = "fixture.arrow";
    arrow.name = "Arrow fixture";
    arrow.description = "Test arrow";
    arrow.kind = ItemKind::Ammunition;
    arrow.weight = 5;
    f.items.push_back(arrow); // 9
    constexpr Abilities bases[]{
        {15, 13, 14, 8, 12, 10}, {13, 10, 14, 8, 15, 12}, {8, 15, 14, 12, 13, 10}, {8, 14, 13, 15, 12, 10}};
    for (std::uint16_t n = 0; n < partySize; ++n) {
        auto& b = f.preset.characters[n];
        b.id = "fixture.character" + std::to_string(n);
        b.name = "Character " + std::to_string(n);
        b.race = {n};
        b.characterClass = {n};
        b.baseAbilities = bases[n];
    }
    auto& fighter = f.preset.characters[0];
    fighter.defense = true;
    fighter.items[0] = {{0}, 1};
    fighter.items[1] = {{7}, 1};
    fighter.items[2] = {{8}, 1};
    fighter.itemCount = 3;
    fighter.equipment = {{1}, {0}, {2}};
    auto& cleric = f.preset.characters[1];
    cleric.items[0] = {{1}, 1};
    cleric.items[1] = {{6}, 1};
    cleric.items[2] = {{8}, 1};
    cleric.itemCount = 3;
    cleric.equipment = {{1}, {0}, {2}};
    auto& rogue = f.preset.characters[2];
    rogue.items[0] = {{2}, 1};
    rogue.items[1] = {{5}, 1};
    rogue.items[2] = {{3}, 1};
    rogue.items[3] = {{9}, 20};
    rogue.itemCount = 4;
    rogue.equipment = {{1}, {0}, {}};
    auto& wizard = f.preset.characters[3];
    wizard.items[0] = {{4}, 1};
    wizard.itemCount = 1;
    wizard.equipment.mainHand = {0};
    return f;
}
} // namespace blobber

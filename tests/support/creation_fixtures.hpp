#pragma once
#include "rules/creation.hpp"
#include "spell_fixtures.hpp"
#include <cstdio>
namespace blobber {
inline rules::Catalog creationCatalog(bool progression = false) {
    using namespace rules;
    const auto base = spellCatalog();
    std::vector<RaceDefinition> races;
    std::vector<ClassDefinition> classes;
    std::vector<ItemDefinition> items;
    std::vector<SpellDefinition> spells;
    for (std::uint16_t i = 0; i < 4; ++i) {
        races.push_back(*base.race({i}));
        classes.push_back(*base.characterClass({i}));
    }
    constexpr const char* raceIds[]{"human", "hill_dwarf", "lightfoot_halfling", "high_elf"};
    for (unsigned i = 0; i < 4; ++i)
        races[i].id = raceIds[i];
    races[3].wizardCantrip = true;
    for (std::uint16_t i = 0; i < base.itemCount(); ++i)
        items.push_back(*base.item({i}));
    for (std::uint16_t i = 0; i < base.spellCount(); ++i)
        spells.push_back(*base.spell({i}));
    if (progression) {
        for (unsigned i = 0; i < 4; ++i)
            classes[i].progression = ProgressionClass(i + 1);
        for (unsigned i = unsigned(SpellKind::Aid); i < unsigned(SpellKind::Count); ++i) {
            const auto kind = SpellKind(i);
            const auto& r = *spellRules(kind);
            const SpellId id{std::uint16_t(spells.size())};
            spells.push_back({r.id, r.name, r.description, kind});
            for (auto& cls : classes)
                if (spellClass(kind, cls.casting))
                    cls.spells[cls.spellCount++] = id;
        }
    }
    ItemDefinition staff = items[1];
    staff.id = "quarterstaff";
    staff.weapon = WeaponType::Quarterstaff;
    staff.weight = 400;
    staff.versatileSides = 8;
    items.push_back(staff); // 10
    auto add = [&](const char* id, ItemKind kind, int weight, Consumable consumable = Consumable::None) {
        ItemDefinition d;
        d.id = id;
        d.name = id;
        d.description = "Dedicated creation fixture";
        d.kind = kind;
        d.weight = weight;
        d.consumable = consumable;
        items.push_back(d);
    };
    add("spellbook", ItemKind::Gear, 300);
    add("arcane_focus", ItemKind::Gear, 100);
    add("healing_potion", ItemKind::Consumable, 50, Consumable::Healing);
    add("mana_potion", ItemKind::Consumable, 50, Consumable::Mana);
    add("camp_kit", ItemKind::Supply, 800);
    add("holy_symbol", ItemKind::Gear, 100);
    classes[3].weapons |= weaponBit(WeaponType::Quarterstaff);
    std::vector<CreationClass> creation;
    auto grant = [](unsigned item, unsigned slot = 255, unsigned count = 1) {
        return StartingGrant{{std::uint16_t(item)}, std::uint16_t(count), std::uint8_t(slot)};
    };
    auto option = [&](unsigned item, unsigned slot) {
        GearOption o;
        o.id = "option" + std::to_string(item);
        o.name = o.id;
        o.items[0] = grant(item, slot);
        o.count = 1;
        return o;
    };
    GearOption bow = option(3, 3);
    bow.items[1] = grant(9, 255, 20);
    bow.count = 2;
    for (std::uint16_t i = 0; i < 4; ++i) {
        CreationClass c;
        c.characterClass = {i};
        c.recommended = base.preset()->characters[i].baseAbilities;
        c.groups[0].count = 2;
        c.groups[0].options[0] = option(i == 0 ? 0 : i == 1 ? 1 : i == 2 ? 2 : 4, 1);
        c.groups[0].options[1] = option(i == 0 ? 2 : i == 1 ? 10 : i == 2 ? 4 : 10, 1);
        c.groupCount = 1;
        if (i < 3) {
            c.groupCount = 3;
            c.groups[1].count = i < 2 ? 2 : 1;
            c.groups[1].options[0] = option(i == 0 ? 7 : i == 1 ? 6 : 5, 0);
            c.groups[1].options[1] = option(i == 0 ? 5 : 7, 0);
            c.groups[2].count = i < 2 ? 2 : 1;
            c.groups[2].options[0] = i < 2 ? option(8, 2) : bow;
            c.groups[2].options[1] = bow;
        }
        if (i == 1)
            c.supplies[c.supplyCount++] = grant(16);
        if (i == 3) {
            c.supplies[c.supplyCount++] = grant(11);
            c.supplies[c.supplyCount++] = grant(12);
        }
        c.supplies[c.supplyCount++] = grant(13, 255, 2);
        if (i == 1 || i == 3)
            c.supplies[c.supplyCount++] = grant(14, 255, 2);
        creation.push_back(c);
    }
    Catalog staging({}, {}, races, classes, items, std::nullopt, spells, creation);
    PresetParty preset;
    for (std::uint16_t i = 0; i < 4; ++i) {
        auto d = recommendedDraft(staging, {i}, {i});
        std::snprintf(d.name.data(), d.name.size(), "Fixture %u", i);
        preset.characters[i] = finishBuild(staging, d, "fixture." + std::to_string(i));
    }
    auto monster = *base.monster({0});
    if (progression) {
        monster.experience = 50;
        monster.humanoid = true;
    }
    return {{*base.attack({0})}, {monster}, races, classes, items, preset, spells, creation};
}
} // namespace blobber

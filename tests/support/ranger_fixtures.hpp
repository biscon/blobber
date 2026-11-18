#pragma once
#include "creation_fixtures.hpp"
#include "game/session.hpp"

namespace blobber::test {
inline rules::Catalog rangerCatalog(const rules::Catalog& world = creationCatalog(true)) {
    using namespace rules;
    const auto base = creationCatalog(true);
    std::vector<AttackDefinition> attacks;
    std::vector<MonsterDefinition> monsters;
    std::vector<RaceDefinition> races;
    std::vector<ClassDefinition> classes;
    std::vector<ItemDefinition> items;
    std::vector<SpellDefinition> spells;
    std::vector<CreationClass> creation;
    for (std::uint16_t i = 0; i < world.attackCount(); ++i)
        attacks.push_back(*world.attack({i}));
    for (std::uint16_t i = 0; i < world.monsterCount(); ++i)
        monsters.push_back(*world.monster({i}));
    for (std::uint16_t i = 0; i < world.itemCount(); ++i)
        items.push_back(*world.item({i}));
    for (std::uint16_t i = 0; i < base.raceCount(); ++i) {
        races.push_back(*base.race({i}));
        classes.push_back(*base.characterClass({i}));
        creation.push_back(*base.creation({i}));
    }
    races[2].size = CreatureSize::Small;
    for (std::uint16_t i = 0; i < base.spellCount(); ++i)
        spells.push_back(*base.spell({i}));
    const ClassId ranger{std::uint16_t(classes.size())};
    auto cls = classes[0];
    cls.id = "ranger";
    cls.name = "Ranger";
    cls.allowsDefense = false;
    cls.features = 0;
    cls.armor = armorBit(ArmorType::Light) | armorBit(ArmorType::Medium) | armorBit(ArmorType::Shield);
    cls.saves = {true, true, false, false, false, false};
    cls.progression = ProgressionClass::Ranger;
    cls.casting = Casting::Wisdom;
    cls.spellList = SpellList::Ranger;
    cls.spellProgression = SpellProgression::Half;
    cls.spells[0] = base.findSpell("hunters_mark");
    cls.spells[1] = base.findSpell("cure_wounds");
    cls.spells[2] = base.findSpell("goodberry");
    cls.spellCount = 3;
    classes.push_back(cls);
    auto cfg = creation[2];
    cfg.characterClass = ranger;
    cfg.recommended = {8, 15, 14, 8, 15, 10};
    auto longbow = items[cfg.groups[2].options[0].items[0].item.index];
    longbow.id = "fixture.longbow";
    longbow.name = "Longbow";
    longbow.weapon = WeaponType::Longbow;
    longbow.damage.sides = 8;
    longbow.heavy = true;
    auto& alternate = cfg.groups[2].options[1] = cfg.groups[2].options[0];
    alternate.id = "longbow";
    alternate.name = "Longbow";
    alternate.items[0].item = {std::uint16_t(items.size())};
    cfg.groups[2].count = 2;
    items.push_back(longbow);
    creation.push_back(cfg);
    const Catalog staging(attacks, monsters, races, classes, items, std::nullopt, spells, creation);
    auto preset = *base.preset();
    auto draft = recommendedDraft(staging, {0}, ranger);
    std::snprintf(draft.name.data(), draft.name.size(), "Ranger");
    preset.characters[0] = finishBuild(staging, draft, "ranger.fixture");
    return {attacks, monsters, races, classes, items, preset, spells, creation};
}

inline void advanceRanger(Party& party, int target, rules::SpellKind healing = rules::SpellKind::Goodberry) {
    while (party.character({0})->build.level < target) {
        party.grantNextLevelExperience();
        auto request = party.advancementRequest({0});
        if (request.level == 1)
            request.choices.learned = rules::spellBit(party.catalog().findSpell(rules::spellRules(healing)->id));
        if (request.level == 3)
            request.choices.abilityImprovement.increases[4] = 2;
        if (!party.advance(request))
            throw std::runtime_error("Ranger fixture advancement failed");
    }
}

inline PartyCommand rangerSpell(const GameSession& game, rules::SpellKind kind) {
    auto command = spellCommand(game, kind);
    command.character = {0};
    return command;
}
} // namespace blobber::test

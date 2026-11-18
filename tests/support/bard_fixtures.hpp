#pragma once
#include "creation_fixtures.hpp"
#include "game/session.hpp"
namespace blobber::test {
inline rules::Catalog bardCatalog(const rules::Catalog& world = creationCatalog(true)) {
    using namespace rules;
    const auto base = creationCatalog(true);
    std::vector<RaceDefinition> races;
    std::vector<ClassDefinition> classes;
    std::vector<CreationClass> creation;
    std::vector<ItemDefinition> items;
    std::vector<SpellDefinition> spells;
    std::vector<MonsterDefinition> monsters;
    std::vector<AttackDefinition> attacks;
    for (std::uint16_t i = 0; i < base.raceCount(); ++i)
        races.push_back(*base.race({i}));
    for (std::uint16_t i = 0; i < base.classCount(); ++i) {
        classes.push_back(*base.characterClass({i}));
        creation.push_back(*base.creation({i}));
    }
    for (std::uint16_t i = 0; i < world.itemCount(); ++i)
        items.push_back(*world.item({i}));
    for (std::uint16_t i = 0; i < world.attackCount(); ++i)
        attacks.push_back(*world.attack({i}));
    for (std::uint16_t i = 0; i < world.monsterCount(); ++i)
        monsters.push_back(*world.monster({i}));
    for (std::uint16_t i = 0; i < base.spellCount(); ++i)
        spells.push_back(*base.spell({i}));
    monsters.front().armorClass = 15;
    attacks.front().bonus = 5;
    attacks.front().damage = {1, 4, 0};
    auto cls = classes[2];
    cls.id = "bard";
    cls.name = "Bard";
    cls.features = 0;
    cls.casting = Casting::Charisma;
    cls.spellList = SpellList::Bard;
    cls.progression = ProgressionClass::Bard;
    cls.spellCount = 0;
    for (std::uint16_t i = 0; i < spells.size(); ++i)
        if (spellClass(spells[i].kind, SpellList::Bard))
            cls.spells[cls.spellCount++] = {i};
    const ClassId bard{std::uint16_t(classes.size())};
    classes.push_back(cls);
    auto cfg = creation[2];
    cfg.characterClass = bard;
    cfg.recommended = {8, 14, 14, 10, 10, 15};
    creation.push_back(cfg);
    Catalog staging(attacks, monsters, races, classes, items, {}, spells, creation);
    auto preset = *base.preset();
    auto draft = recommendedDraft(staging, {0}, bard);
    std::snprintf(draft.name.data(), draft.name.size(), "Lyra");
    preset.characters[0] = finishBuild(staging, draft, "bard.fixture");
    return {attacks, monsters, races, classes, items, preset, spells, creation};
}
inline PartyCommand inspire(GameSession& game, unsigned target) {
    auto command = featureCommand(game, 0, rules::Feature::BardicInspiration);
    command.targetCount = 1;
    command.allies[0] = {std::uint16_t(target)};
    return command;
}
inline void levelBard(Party& party, int level) {
    using namespace rules;
    while (party.character({0})->build.level < level) {
        party.grantNextLevelExperience();
        auto request = party.advancementRequest({0});
        const auto& b = party.character({0})->build;
        if (request.level == 3)
            request.choices.abilityImprovement.increases[5] = 2;
        for (auto k : {SpellChoice::Learned, SpellChoice::Cantrips}) {
            const auto ctx =
                advancementChoices(party.catalog(), b.view(), b.portrait, b.spells, request.choices, k);
            const auto choices = recommendedChoices(party.catalog(), ctx, k);
            (k == SpellChoice::Learned ? request.choices.learned : request.choices.cantrips) = choices;
        }
        if (!party.advance(request))
            throw std::runtime_error("Bard advancement failed");
    }
}
inline PartyCommand inspiredShot(const GameSession& game) {
    auto c = turnCommand(game, PartyAction::Ranged);
    c.character = {2};
    c.target = game.actors().id(0);
    return c;
}
} // namespace blobber::test

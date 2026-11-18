#pragma once
#include "creation_fixtures.hpp"
#include "game/session.hpp"
namespace blobber::test {
inline rules::Catalog paladinCatalog(rules::CreatureType type = rules::CreatureType::Undead,
                                     int armorClass = 1, int hp = 1000,
                                     rules::DamageProfile profile = {}, bool fortitude = false) {
    using namespace rules;
    const auto base = creationCatalog(true);
    std::vector<RaceDefinition> races;
    std::vector<ClassDefinition> classes;
    std::vector<ItemDefinition> items;
    std::vector<SpellDefinition> spells;
    std::vector<CreationClass> creation;
    for (std::uint16_t i = 0; i < base.raceCount(); ++i) races.push_back(*base.race({i}));
    for (std::uint16_t i = 0; i < base.classCount(); ++i) {
        classes.push_back(*base.characterClass({i}));
        creation.push_back(*base.creation({i}));
    }
    for (std::uint16_t i = 0; i < base.itemCount(); ++i) items.push_back(*base.item({i}));
    for (std::uint16_t i = 0; i < base.spellCount(); ++i) spells.push_back(*base.spell({i}));
    auto cls = classes[0];
    cls.id = "paladin"; cls.name = "Paladin"; cls.allowsDefense = false;
    cls.progression = ProgressionClass::Paladin; cls.features = 0;
    cls.casting = Casting::Charisma; cls.spellList = SpellList::Paladin;
    cls.spellProgression = SpellProgression::Half; cls.saves = {false, false, false, false, true, true};
    cls.spellCount = 0;
    for (auto kind : {SpellKind::Bless, SpellKind::CureWounds, SpellKind::ShieldOfFaith})
        cls.spells[cls.spellCount++] = base.findSpell(spellRules(kind)->id);
    classes.push_back(cls);
    auto cfg = creation[0]; cfg.characterClass = {4};
    cfg.recommended = {15, 8, 14, 8, 10, 15};
    creation.push_back(cfg);
    auto monster = *base.monster({0});
    monster.creatureType = type; monster.humanoid = type == CreatureType::Humanoid;
    monster.armorClass = armorClass; monster.hitPoints = hp; monster.damageProfile = profile;
    monster.traits = fortitude ? Traits(Trait::UndeadFortitude) : 0;
    Catalog staging({*base.attack({0})}, {monster}, races, classes, items, std::nullopt, spells, creation);
    auto preset = *base.preset();
    auto d = recommendedDraft(staging, {0}, {4});
    std::snprintf(d.name.data(), d.name.size(), "Paladin");
    preset.characters[0] = finishBuild(staging, d, "paladin.fixture");
    return {{*base.attack({0})}, {monster}, races, classes, items, preset, spells, creation};
}
inline rules::AdvancementChoices paladinAdvanceChoices(const rules::Catalog& cat,
                                                       const rules::CharacterIdentityView& identity,
                                                       const rules::SpellGrants& grants) {
    using namespace rules;
    AdvancementChoices choices;
    if (identity.level == 3) choices.abilityImprovement.increases[5] = 2;
    for (auto kind : {SpellChoice::Cantrips, SpellChoice::Learned, SpellChoice::Prepared}) {
        const auto context = advancementChoices(cat, identity, {}, grants, choices, kind);
        const auto mask = recommendedChoices(cat, context, kind);
        if (kind == SpellChoice::Cantrips) choices.cantrips = mask;
        else if (kind == SpellChoice::Learned) choices.learned = mask;
        else choices.prepared = mask;
    }
    return choices;
}
inline void advancePaladin(Party& party, int target) {
    while (party.character({0})->build.level < target) {
        party.grantNextLevelExperience();
        auto request = party.advancementRequest({0});
        const auto& ch = *party.character({0});
        request.choices = paladinAdvanceChoices(party.catalog(), ch.build.view(), ch.build.spells);
        if (!party.advance(request)) throw std::runtime_error("Paladin fixture advancement failed");
    }
}
} // namespace blobber::test

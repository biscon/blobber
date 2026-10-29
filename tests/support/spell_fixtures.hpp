#pragma once
#include "combat_fixtures.hpp"
#include "hybrid_fixtures.hpp"
namespace blobber {
inline rules::Catalog spellCatalog(int hp = 1000, int ac = 1, int saveAbility = 10, bool stress = false,
                                   bool sleepImmune = false, bool metal = false) {
    auto f = partyFixture();
    f.races[3].sleepImmune = sleepImmune;
    f.items[7].metalArmor = metal;
    f.classes[0].features = static_cast<rules::ClassFeatures>(rules::ClassFeature::SecondWind);
    f.classes[1].features = static_cast<rules::ClassFeatures>(rules::ClassFeature::LifeDomain);
    f.classes[2].features = static_cast<rules::ClassFeatures>(rules::ClassFeature::SneakAttack);
    f.classes[3].features = static_cast<rules::ClassFeatures>(rules::ClassFeature::ArcaneRecovery);
    f.classes[1].casting = rules::Casting::Wisdom;
    f.classes[3].casting = rules::Casting::Intelligence;
    f.preset.characters[1].lifeDomain = true;
    f.preset.characters[2].equipment.ranged = {2};
    std::vector<rules::SpellDefinition> spells;
    for (unsigned i = 0; i < static_cast<unsigned>(rules::SpellKind::Count); ++i) {
        const auto kind = static_cast<rules::SpellKind>(i);
        const auto& r = *rules::spellRules(kind);
        spells.push_back({r.id, r.name, r.description, kind});
        auto& cls = f.classes[r.casting == rules::Casting::Wisdom ? 1 : 3];
        cls.spells[cls.spellCount++] = {static_cast<std::uint16_t>(i)};
    }
    if (stress) {
        for (auto& cls : f.classes) {
            cls.casting = rules::Casting::Intelligence;
            cls.spells = f.classes[3].spells;
            cls.spellCount = f.classes[3].spellCount;
            cls.features = 0;
        }
        f.preset.characters[1].lifeDomain = false;
    }
    // Explicit simulation fixture grants. Creation budgets are tested independently;
    // this fixture covers every concrete spell without a production access bypass.
    for (unsigned n = 0; n < 4; ++n) {
        auto& b = f.preset.characters[n];
        const auto& cls = f.classes[n];
        for (std::size_t i = 0; i < cls.spellCount; ++i) {
            const auto id = cls.spells[i];
            const auto bit = std::uint32_t(1) << id.index;
            const auto& r = *rules::spellRules(spells[id.index].kind);
            if (!r.level)
                b.spells.cantrips |= bit;
            else if (b.lifeDomain && (spells[id.index].kind == rules::SpellKind::Bless ||
                                      spells[id.index].kind == rules::SpellKind::CureWounds))
                b.spells.domain |= bit;
            else {
                b.spells.prepared |= bit;
                if (cls.casting == rules::Casting::Intelligence)
                    b.spells.learned |= bit;
            }
        }
    }
    rules::MonsterDefinition m;
    m.id = "goblin";
    m.sleepImmune = sleepImmune;
    m.metalArmor = metal;
    m.abilities.fill(saveAbility);
    m.armorClass = ac;
    m.hitPoints = hp;
    m.attackCount = 1;
    m.attacks[0] = {0};
    return {{{"blade",
              stress ? rules::AttackKind::Ranged : rules::AttackKind::Melee,
              -100,
              {1, 4, -100},
              rules::DamageType::Slashing}},
            {m},
            f.races,
            f.classes,
            f.items,
            f.preset,
            spells};
}
inline constexpr Player spellSpawn{{6, 8}, Heading::North};
inline NpcWorld spellPlacements(const Map& map, int count = 3, bool close = false) {
    std::vector<NpcInstance> instances;
    for (int i = 0; i < count; ++i)
        instances.push_back(
            {{0}, {6 + (close ? i / 4 - 1 : 0), close ? 7 : 4}, {i % 2, (i / 2) % 2}, Heading::South});
    return {map, spellSpawn.cell, {"goblin"}, instances};
}
inline PartyCommand spellCommand(const GameSession& s, rules::SpellKind kind) {
    PartyCommand c;
    c.action = PartyAction::Spell;
    c.character = {
        static_cast<std::uint16_t>(rules::spellRules(kind)->casting == rules::Casting::Wisdom ? 1 : 3)};
    c.spell = s.party().catalog().findSpell(rules::spellRules(kind)->id);
    c.world = s.combat().snapshot().world;
    c.turn = s.combat().snapshot().turn;
    return c;
}
inline PartyCommand featureCommand(const GameSession& s, unsigned character, rules::Feature feature) {
    PartyCommand c;
    c.character = {static_cast<std::uint16_t>(character)};
    c.action = PartyAction::Feature;
    c.feature = feature;
    c.world = s.combat().snapshot().world;
    c.turn = s.combat().snapshot().turn;
    return c;
}
inline void spellReady(GameSession& s, unsigned character) {
    for (int i = 0; i < 200; ++i) {
        const auto& state = s.combat().snapshot();
        if (state.awaitingAction && state.activeCharacter && state.activeCharacter->index == character)
            return;
        if (state.awaitingAction)
            s.submit(turnCommand(s));
        s.update(.5, false);
    }
    throw std::runtime_error("Spell fixture failed to reach requested turn");
}
} // namespace blobber

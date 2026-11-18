#pragma once
#include "bard_fixtures.hpp"
#include "progression_fixtures.hpp"
#include "support/check.hpp"

namespace blobber::test {
inline void barbarianChecks() {
    using namespace rules;
    const auto cat = barbarianCatalog();
    const auto cls = cat.findClass("barbarian");
    const auto map = hybridRoom();
    const auto empty = spellPlacements(map, 0);
    auto preset = *cat.preset();
    for (unsigned shield : {0u, 1u}) {
        auto draft = recommendedDraft(cat, cat.findRace("high_elf"), cls);
        draft.gear[0] = shield;
        std::snprintf(draft.name.data(), draft.name.size(), "Ulf");
        preset.characters[0] = finishBuild(cat, draft, "barbarian.elf");
        GameSession g(map, cat, preset, empty, 1, spellSpawn, false);
        auto& p = g.party();
        const auto& hero = *p.character({0});
        check(p.rageUses({0}) == 2 && hero.build.level == 1, "Level one starts with two Rages");
        advanceBarbarian(p, 4);
        check(hero.stats.armorClass ==
                      10 + hero.stats.modifiers[1] + hero.stats.modifiers[2] + int(shield) * 2 &&
                  !hero.maxMana && p.rageUses({0}) == 3,
              "Barbarian AC, ASI and daily charges");
        EffectRequest e;
        e.world = g.actors().snapshot().world;
        e.caster = CharacterId{0};
        e.targets[0] = CharacterId{0};
        e.count = 1;
        e.difficulty = 13;
        e.kind = EffectKind::MageArmor;
        check(bool(g.applyEffect(e)) &&
                  g.effects().armorClass(CharacterId{0}) ==
                      10 + hero.stats.modifiers[1] + std::max(3, hero.stats.modifiers[2]) + int(shield) * 2,
              "Choose one AC formula and count shield once");
        e.kind = EffectKind::Bless;
        check(bool(g.applyEffect(e)), "Establish concentration");
        auto rage = featureCommand(g, 0, Feature::Rage);
        check(g.submit(rage).reason == CommandReason::Resolved && !g.effects().concentration({0}).serial,
              "Rage ends concentration");
        for (std::uint16_t i = 0; i < cat.spellCount(); ++i)
            if (hero.build.spells.racial & spellBit({i})) {
                auto c = spellCommand(g, cat.spell({i})->kind);
                c.character = {0};
                check(g.submit(c).reason == CommandReason::Unavailable, "Rage blocks racial casting");
            }
        DiceRng rng(19);
        EffectController saves(p, g.actors(), rng);
        CombatEvents events;
        check(saves.save(CharacterId{0}, Ability::Strength, 13).roll.mode == RollMode::Advantage &&
                  saves.save(CharacterId{0}, Ability::Dexterity, 13).roll.mode == RollMode::Advantage,
              "Rage aids Strength saves and Danger Sense aids visible Dexterity saves");
        saves.condition(CharacterId{0}, EffectKind::Blind, {1}, 0, events);
        check(saves.save(CharacterId{0}, Ability::Dexterity, 13).roll.mode == RollMode::Normal,
              "Blinding the Barbarian removes Danger Sense");
        DamagePacket packet = DamagePacket::single(9, DamageType::Slashing);
        packet.components[1] = {4, DamageType::Fire};
        packet.count = 2;
        const int hp = hero.hitPoints;
        p.grantTemporary({0}, 5, 60000);
        const auto hit = damageParty(p, {0}, packet, true, 0);
        check(hit.total.damage == 8 && hit.components[0].damage == 4 && hit.components[1].damage == 4 &&
                  hero.hitPoints == hp - 3 && !hero.temporaryHitPoints,
              "Mixed critical hit halves physical component before temporary HP");
        g.update(60, false);
        check(!hero.barbarian.rageUntil && !hero.barbarian.frenzy &&
                  damageParty(p, {0}, DamagePacket::single(3, DamageType::Piercing), false, 60000)
                          .total.damage == 3,
              "Expiry discards unused Frenzy and restores full incoming damage");
        RestRequest rest;
        rest.kind = RestKind::Short;
        rest.world = g.actors().snapshot().world;
        rest.token = g.restToken();
        rest.dice[0] = 1;
        check(bool(g.rest(rest)) && p.rageUses({0}) == 2, "Short rest preserves spent Rage");
        rest.kind = RestKind::Long;
        rest.dice = {};
        rest.token = g.restToken();
        check(bool(g.rest(rest)) && p.rageUses({0}) == 3, "Long rest restores Rage");
    }
    {
        const NpcWorld approach(map, spellSpawn.cell, {"goblin"}, {{{0}, {6, 6}, {0, 1}, Heading::South}});
        GameSession g(map, cat, approach, 5, spellSpawn, false);
        advanceBarbarian(g.party(), 3);
        g.setAnimated(false);
        check(g.submit(featureCommand(g, 0, Feature::Rage)).reason == CommandReason::Resolved,
              "Rage before contact");
        g.update(20, true);
        g.move(Action::Forward);
        levelReady(g, 0);
        auto swing = featureCommand(g, 0, Feature::RecklessFrenzy);
        swing.target = g.actors().id(0);
        check(g.combat().snapshot().gameplayMs < combatRoundMs && !g.combat().bonusReady({0}) &&
                  g.submit(swing).reason != CommandReason::Resolved,
              "Entering initiative preserves bonus-action debt");
        g.submit(featureCommand(g, 0, Feature::EndTurn));
        levelReady(g, 0);
        swing.turn = g.combat().snapshot().turn;
        check(g.submit(swing).reason == CommandReason::Resolved,
              "Frenzy can be the first swing of a later turn");
        levelReady(g, 0);
        auto ordinary = turnCommand(g, PartyAction::Melee);
        ordinary.target = g.actors().id(0);
        check(g.submit(ordinary).reason == CommandReason::Resolved, "Frenzy preserves the ordinary attack");
        g.party().damage({0}, g.party().character({0})->hitPoints);
        check(!g.party().character({0})->barbarian.rageUntil, "Dropping unconscious ends Rage immediately");
    }
    const NpcWorld near(map, spellSpawn.cell, {"goblin"},
                        {{{0}, {6, 7}, {0, 1}, Heading::South}, {{0}, {6, 7}, {1, 1}, Heading::South}});
    bool inspired = false;
    for (unsigned seed = 1; seed <= 48 && !inspired; ++seed) {
        GameSession g(map, cat, near, seed, spellSpawn, false);
        advanceBarbarian(g.party(), 3);
        levelReady(g, 0);
        const auto strike = [&](Feature f, unsigned target) {
            auto c = featureCommand(g, 0, f);
            c.target = g.actors().id(target);
            return c;
        };
        check(g.submit(featureCommand(g, 0, Feature::Rage)).reason == CommandReason::Resolved &&
                  g.submit(strike(Feature::FrenzyStrike, 0)).reason != CommandReason::Resolved,
              "No Frenzy on Rage activation turn");
        levelReady(g, 0);
        check(g.submit(strike(Feature::FrenzyStrike, 0)).reason != CommandReason::Resolved,
              "Activation consumes the bonus action");
        check(g.submit(featureCommand(g, 0, Feature::EndTurn)).reason == CommandReason::Resolved,
              "End activation turn");
        levelReady(g, 0);
        check(g.submit(featureCommand(g, 0, Feature::EndTurn)).reason == CommandReason::Resolved,
              "Keep Frenzy for a later round");
        levelReady(g, 0);
        check(g.party().character({0})->barbarian.frenzy, "Ending turn keeps the swing");
        // Grant the Bard's die at the simulation boundary to isolate its interaction with the two attacks.
        g.party().giveInspiration({1}, {0}, g.combat().snapshot().gameplayMs + 60000);
        const auto settle = [&] {
            const auto d = g.pendingRollDecision();
            if (d.serial) {
                check(g.resolveRollDecision(d.world, d.serial, {true, {}}), "Inspiration resumes same swing");
                inspired = true;
            }
        };
        check(g.submit(strike(Feature::RecklessAttack, 0)).reason == CommandReason::Resolved,
              "Accept first Reckless attack");
        settle();
        levelReady(g, 0);
        check(g.effects().attackMode(g.actors().id(0), CharacterId{0}, true) == RollMode::Advantage &&
                  g.effects().attackMode(g.actors().id(0), CharacterId{1}, true) == RollMode::Normal,
              "Reckless exposes only its attacker");
        check(g.submit(strike(Feature::RecklessFrenzy, 1)).reason != CommandReason::Resolved,
              "Cannot choose Reckless a second time");
        check(g.submit(strike(Feature::FrenzyStrike, 1)).reason == CommandReason::Resolved,
              "Spend saved Frenzy on another target");
        settle();
        check(!g.party().character({0})->barbarian.frenzy && g.party().rageUses({0}) == 2,
              "Extra swing and optional die are spent once");
    }
    check(inspired, "Reached a Bard interrupt inside a Barbarian attack");
}
} // namespace blobber::test

#pragma once
#include "ranger_fixtures.hpp"
#include "progression_fixtures.hpp"
#include "game/commands.hpp"
#include "game/creator.hpp"
#include "support/check.hpp"

namespace blobber::test {
inline void rangerProgression() {
    using namespace rules;
    const auto cat = rangerCatalog();
    const auto ranger = cat.findClass("ranger");
    const auto mark = cat.findSpell("hunters_mark");
    const auto berries = cat.findSpell("goodberry");
    const auto cure = cat.findSpell("cure_wounds");
    for (std::uint16_t race = 0; race < cat.raceCount(); ++race) {
        auto draft = recommendedDraft(cat, {race}, ranger);
        std::snprintf(draft.name.data(), draft.name.size(), "Ranger %u", race);
        CreatorModel model;
        model.draft = draft;
        check(bool(previewBuild(cat, draft)) && !model.applicable(cat, CreatorStep::Cantrips) &&
                  !model.applicable(cat, CreatorStep::Prepared),
              "Ranger creation validates without class spell selection");
        draft.gear[2] = 1;
        const auto equipped = previewBuild(cat, draft);
        check(equipped && equipped.stats.weapons[equipped.loadout.equipment.ranged.index].heavyDisadvantage ==
                              (race == 2), "Longbow preview reports the Small-wielder penalty");
        draft.gear[2] = 0;
        auto preset = *cat.preset();
        preset.characters[0] = finishBuild(cat, draft, "ranger.character");
        for (const auto first : {cure, berries}) {
            Party party(cat, preset);
            const auto novice = *party.character({0});
            const auto ranged = unsigned(EquipmentSlot::Ranged);
            const auto melee = unsigned(EquipmentSlot::MainHand);
            check(!novice.maxMana && !party.spellAccess({0}, mark) &&
                      party.inventory().count({0}, Ammunition::Arrow, cat) == 20,
                  "Novice Ranger starts with arrows and no class mana or spells");
            party.grantNextLevelExperience();
            auto request = party.advancementRequest({0});
            const auto choices = advancementChoices(cat, novice.build.view(), {}, novice.build.spells, {},
                                                    SpellChoice::Learned);
            check(recommendedChoices(cat, choices, SpellChoice::Learned) == spellBit(cure),
                  "Recommendation picks Cure Wounds without requiring a mark selection");
            for (const auto rejected : {SpellMask{0}, spellBit(mark), spellBit(cure) | spellBit(berries)}) {
                request.choices.learned = rejected;
                check(party.advance(request).error == AdvancementError::Choices &&
                          party.character({0})->build.level == 1 && !party.character({0})->mana,
                      "Invalid Ranger grants leave advancement atomic");
            }
            advanceRanger(party, 2, cat.spell(first)->kind);
            const auto& apprentice = *party.character({0});
            check(apprentice.build.spells.learned == (spellBit(first) | spellBit(mark)) &&
                      apprentice.mana == 4 && !apprentice.build.spells.prepared &&
                      apprentice.stats.weapons[ranged].attackBonus == novice.stats.weapons[ranged].attackBonus + 2 &&
                      apprentice.stats.weapons[melee].attackBonus == novice.stats.weapons[melee].attackBonus,
                  "Level two unlocks Archery and exactly the selected spells");
            party.spendMana({0}, 2);
            advanceRanger(party, 3);
            check(party.character({0})->mana == 4 && party.character({0})->maxMana == 6,
                  "Advancement preserves the spent mana deficit");
            advanceRanger(party, 4);
            const auto& hunter = *party.character({0});
            const auto heal = party.spellAccess({0}, cure);
            check(hunter.build.spells.learned == (spellBit(cure) | spellBit(berries) | spellBit(mark)) &&
                      hunter.build.levelFourAsi.increases[4] == 2 && hunter.mana == 4 &&
                      heal && heal.source == SpellSource::Known && heal.casting == Casting::Wisdom &&
                      heal.modifier == hunter.stats.modifiers[4],
                  "Hunter learns the remaining spell and casts from its Wisdom-based known list");
            check(!preparationOptions(cat, hunter.build.view(), hunter.build.spells) &&
                      !validPreparation(cat, hunter.build.view(), hunter.build.spells, spellBit(cure)),
                  "Ranger never prepares known spells");
            if (race == 3)
                check(party.spellAccess({0}, cat.findSpell("fire_bolt")).casting == Casting::Intelligence,
                      "High Elf retains separate Intelligence casting");
            RestRequest shortRest;
            party.damage({0}, 2);
            shortRest.dice[0] = 1;
            DiceRng rng(7);
            check(bool(party.recover(shortRest, {}, rng)) && hunter.mana == 4, "Short rest keeps spent mana");
            RestRequest longRest;
            longRest.kind = RestKind::Long;
            check(bool(party.recover(longRest, {}, rng)) && hunter.mana == 6, "Long rest restores Ranger mana");
        }
    }
}

inline void rangerGoodberry() {
    using namespace rules;
    const auto cat = rangerCatalog();
    const auto map = hybridRoom();
    auto preset = *cat.preset();
    preset.characters[2] = preset.characters[1];
    preset.characters[2].id = "ranger.healing.recipient";
    GameSession game(map, cat, preset, spellPlacements(map, 0), 5, spellSpawn, false);
    auto& party = game.party();
    advanceRanger(party, 2);
    const auto cast = rangerSpell(game, SpellKind::Goodberry);
    check(game.submit(cast).reason == CommandReason::FullHealth && party.character({0})->mana == 4,
          "Goodberry rejects useless casts without spending mana");
    party.damage({1}, 8);
    party.damage({2}, 7);
    party.damage({3}, party.character({3})->hitPoints);
    CommandController ui(game);
    ui.refresh({}, false, false, true);
    ui.selectCharacter(0);
    ui.openSpellbook();
    for (std::size_t i = 0; i < ui.snapshot().magic.count; ++i)
        if (ui.snapshot().magic.entries[i].kind == SpellKind::Goodberry) {
            ui.chooseSpellbook(i);
            break;
        }
    check(ui.snapshot().magic.armed && ui.snapshot().magic.self && ui.confirm() == CommandReason::Resolved,
          "Goodberry confirms through the party spell picker");
    check(party.character({1})->hitPoints == party.character({1})->stats.maxHitPoints - 2 &&
              party.character({2})->hitPoints == party.character({2})->stats.maxHitPoints - 3 &&
              party.character({3})->life == Life::Dying && party.character({0})->mana == 2,
          "Goodberry redistributes ten points with stable ties and excludes dying allies");
    check(std::count_if(game.events().begin(), game.events().end(), [](const auto& e) {
              return e.event == CombatEvent::Healing && e.spell == SpellKind::Goodberry;
          }) == 2, "Goodberry emits one result per healed recipient");
}

inline void rangerMarkAndDamage() {
    using namespace rules;
    const auto cat = rangerCatalog();
    const auto map = hybridRoom();
    const auto places = spellPlacements(map, 1);
    const auto shoot = [&](unsigned seed, bool marked, bool wounded, bool cancel) {
        GameSession game(map, cat, places, seed, spellSpawn, false);
        advanceRanger(game.party(), 3);
        const auto target = game.actors().id(0);
        auto mark = rangerSpell(game, SpellKind::HuntersMark);
        check(game.submit(mark).reason == CommandReason::MissingTarget &&
                  game.party().character({0})->mana == 6,
              "Hunter's Mark requires a target before spending mana");
        mark.enemies[0] = {target.slot, target.world + 1};
        mark.targetCount = 1;
        check(game.submit(mark).reason == CommandReason::InvalidTarget &&
                  game.party().character({0})->mana == 6,
              "Hunter's Mark rejects actors from another world");
        mark.enemies[0] = target;
        check(game.submit(mark).reason == CommandReason::Resolved && game.party().character({0})->mana == 4 &&
                  game.effects().markedBy({0}, target) && !game.effects().markedBy({1}, target),
              "Mark binds to one caster and target and spends two mana");
        const auto status = game.effects().concentrationStatus(CharacterId{0});
        check(status && status->effect == EffectKind::HuntersMark && status->remainingMs == huntersMarkDurationMs,
              "Hunter's Mark uses one hour of concentration");
        if (!marked)
            check(game.submit(featureCommand(game, 0, Feature::DismissConcentration)).reason == CommandReason::Resolved,
                  "Hunter can dismiss the mark without spending an action");
        if (wounded)
            game.actors().damage(target, 1, 0);
        game.update(6.1, false);
        PartyCommand shot;
        shot.character = {0};
        shot.action = PartyAction::Ranged;
        shot.target = target;
        shot.world = game.combat().snapshot().world;
        shot.turn = game.combat().snapshot().turn;
        const auto accepted = game.submit(shot);
        check(accepted.reason == CommandReason::Resolved && accepted.event == CombatEvent::AttackStarted &&
                  !accepted.damage && !accepted.hit && game.submit(shot).reason != CommandReason::Resolved &&
                  game.party().inventory().count({0}, Ammunition::Arrow, cat) == 19,
              "Ranger shot spends one arrow and hides damage until impact");
        if (cancel)
            game.actors().damage(target, 10000, game.timeMs());
        const auto hp = game.actors().actor(target)->hitPoints;
        for (unsigned n = 0; n < 30; ++n)
            for (const auto& event : game.update(.05, false))
                if (event.attackId.serial == accepted.attackId.serial && event.event == CombatEvent::Attack) {
                    check(game.actors().actor(target)->hitPoints == hp - event.damage &&
                              (!cancel || (event.targetGone && !game.effects().concentration({0}).serial)),
                          "Impact commits damage once and a dead target clears the mark");
                    return event;
                }
        throw std::runtime_error("Ranger shot did not resolve");
    };
    unsigned criticalSeed = 0;
    CombatOutcome plain;
    for (unsigned seed = 1; seed <= 64 && !criticalSeed; ++seed) {
        plain = shoot(seed, false, false, false);
        if (plain.critical)
            criticalSeed = seed;
    }
    check(criticalSeed, "Deterministic shot fixtures include a critical");
    const auto markOnly = shoot(criticalSeed, true, false, false);
    const auto slayerOnly = shoot(criticalSeed, false, true, false);
    const auto both = shoot(criticalSeed, true, true, false);
    const auto markBonus = markOnly.damage - plain.damage;
    const auto slayerBonus = slayerOnly.damage - plain.damage;
    check(markBonus >= 2 && markBonus <= 12 && slayerBonus >= 2 && slayerBonus <= 16 &&
              both.damage >= markOnly.damage + 2 && both.damage <= markOnly.damage + 16 &&
              both.critical && both.damageComponents[1].damage == 0,
          "Mark and Colossus Slayer add doubled weapon dice independently and together");
    check(!shoot(criticalSeed, true, true, true).damage, "Canceled Ranger shots apply no bonus damage");

    GameSession concentration(map, cat, places, 5, spellSpawn, false);
    advanceRanger(concentration.party(), 2);
    EffectRequest mark;
    mark.kind = EffectKind::HuntersMark;
    mark.world = concentration.actors().snapshot().world;
    mark.targets[0] = concentration.actors().id(0);
    mark.count = 1;
    mark.caster = CharacterId{1};
    check(concentration.applyEffect(mark).error == EffectError::InvalidCaster,
          "The effect owner rejects marks from non-Rangers");
    mark.caster = CharacterId{0};
    check(bool(concentration.applyEffect(mark)), "Ranger can hold concentration");
    concentration.party().damage({0}, concentration.party().character({0})->hitPoints);
    concentration.update(0, false);
    check(!concentration.effects().concentration({0}).serial, "Incapacitation immediately clears the mark");
}
} // namespace blobber::test

#pragma once
#include "paladin_fixtures.hpp"
#include "game/commands.hpp"
#include "game/creator.hpp"
#include "ui/interaction.hpp"
#include "generation/population.hpp"
#include "support/check.hpp"
#include <cstdio>
#include <cmath>
#include <cstring>
namespace blobber::test {
inline void highSpellRewardProfile() {
    using namespace rules;
    std::vector<SpellDefinition> spells;
    for (unsigned i = 0; i < unsigned(SpellKind::Count); ++i) {
        if (SpellKind(i) == SpellKind::FireBolt) continue;
        const auto& r = *spellRules(SpellKind(i));
        spells.push_back({r.id, r.name, r.description, SpellKind(i)});
    }
    const auto& fire = *spellRules(SpellKind::FireBolt);
    spells.insert(spells.begin() + 32, {fire.id, fire.name, fire.description, SpellKind::FireBolt});
    RaceDefinition race; race.id = "fixture"; race.name = "Fixture";
    ClassDefinition cls; cls.id = "martial"; cls.name = "Martial";
    cls.saves[0] = cls.saves[2] = true;
    const Catalog cat({}, {}, {race}, {cls}, {}, std::nullopt, spells);
    PresetParty party;
    party.characters[0].race = {0}; party.characters[0].characterClass = {0};
    const auto bit = spellBit(cat.findSpell("fire_bolt"));
    check(bit == spellBit({32}), "Reward fixture places attack cantrip above bit 31");
    check(!generation::rewardProfile(cat, party).spellAttack, "Martial party has no spell-attack reward affinity");
    party.characters[0].spells.racial = bit;
    check(generation::rewardProfile(cat, party).spellAttack, "High-index racial attack cantrip enables spell-attack rewards");
}
inline void paladinRulesAndResources() {
    using namespace rules;
    const auto cat = paladinCatalog();
    check(spellBit({31}) && spellBit({32}) && spellBit({63}) && !spellBit({64}), "Spell mask boundary is defined");
    check(spellCount(spellBit({31}) | spellBit({32}) | spellBit({63})) == 3, "Spell mask high bits survive counting");
    for (std::uint16_t race = 0; race < 4; ++race) {
        auto draft = recommendedDraft(cat, {race}, {4});
        std::snprintf(draft.name.data(), draft.name.size(), "Paladin %u", race);
        check(bool(previewBuild(cat, draft)), "Paladin creation across races");
        check(!draft.spells.cantrips && !draft.spells.prepared && !draft.spells.learned, "No level-one class casting");
        CreatorModel model; model.draft = draft;
        check(!model.applicable(cat, CreatorStep::Cantrips) && !model.applicable(cat, CreatorStep::Prepared), "Creation skips paladin class spell choices");
        auto preset = *cat.preset(); preset.characters[0] = finishBuild(cat, draft, "test.paladin");
        Party party(cat, preset);
        auto old = *party.character({0});
        check(old.layOnHands == 5 && old.maxMana == 0 && !old.channelDivinity, "Initial paladin resources");
        if (race == 3) {
            const auto id = cat.findSpell("fire_bolt");
            const auto access = party.spellAccess({0}, id);
            check(access && access.casting == Casting::Intelligence, "High Elf paladin retains racial Intelligence");
        }
        party.damage({1}, 4);
        check(!party.useLayOnHands({0}, {1}, 0) && !party.useLayOnHands({0}, {1}, 5), "Healing rejects invalid/overheal amounts");
        check(party.useLayOnHands({0}, {1}, 3), "Healing spends selected points");
        check(party.character({0})->layOnHands == 2, "Healing pool spent exactly once");
        party.grantNextLevelExperience();
        const auto incomplete = party.advance(party.advancementRequest({0}), true);
        check(incomplete.error == AdvancementError::Choices, "Paladin must select preparation when casting unlocks");
        const int baseAc = old.stats.armorClass;
        for (int level = 2; level <= 4; ++level) {
            advancePaladin(party, level);
            const auto& ch = *party.character({0});
            check(ch.layOnHands == 5 * level - 3, "Advancement preserves healing expenditure");
            check(ch.maxMana == (level == 2 ? 4 : 6), "Half-caster mana budget");
            check(ch.stats.armorClass == baseAc + 1, "Defense starts at two and never stacks");
            check(!ch.build.spells.cantrips && !ch.build.spells.learned, "No cantrip/spellbook progression grants");
            check(validateGrants(cat, ch.build.view(), ch.build.spells) == BuildError::None, "Valid paladin level grants");
            check(preparationLimit(cat, ch.build.view()) == std::max(1, ch.stats.modifiers[5] + level / 2), "Charisma preparation scaling");
            const auto cure = party.spellAccess({0}, cat.findSpell("cure_wounds"));
            if (ch.build.spells.prepared & spellBit(cat.findSpell("cure_wounds")))
                check(cure && cure.casting == Casting::Charisma && cure.modifier == ch.stats.modifiers[5], "Shared healing spell uses CHA");
            check(spellCount(ch.build.spells.domain) == (level >= 3 ? 2u : 0u), "Devotion grants both oath spells");
            if (level >= 3) {
                const auto oath = party.spellAccess({0}, cat.findSpell("protection_from_evil_and_good"));
                check(oath && oath.source == SpellSource::Oath && oath.casting == Casting::Charisma, "High-bit oath spell is accessible");
            }
        }
        auto saved = party.checkpoint(); Party restored(cat, saved);
        check(restored.character({0})->layOnHands == 17 && restored.character({0})->channelDivinity, "Party snapshot preserves paladin resources");
        saved.state.characters[0].layOnHands = 21;
        rejects([&] { Party invalid(cat, saved); }, "Invalid healing pool rejected on restore");
        party.useFeature({0}, Feature::SacredWeapon);
        RestRequest shortRest; DiceRng rng(5);
        check(bool(party.recover(shortRest, {}, rng)), "Short rest restores Channel Divinity");
        check(party.character({0})->channelDivinity && party.character({0})->layOnHands == 17, "Short rest preserves healing expenditure");
        RestRequest longRest; longRest.kind = RestKind::Long;
        check(bool(party.recover(longRest, {}, rng)) && party.character({0})->layOnHands == 20, "Healing expenditure alone can justify long rest");
    }
    Party wounded(cat);
    wounded.damage({1}, wounded.character({1})->stats.maxHitPoints);
    check(wounded.character({1})->life == Life::Dying && wounded.useLayOnHands({0}, {1}, 1), "Lay on Hands revives a dying ally");
    wounded.damage({1}, 10000);
    const auto pool = wounded.character({0})->layOnHands;
    check(!wounded.useLayOnHands({0}, {1}, 1) && wounded.character({0})->layOnHands == pool, "Dead ally rejects healing without spending");
    wounded.damage({0}, 2);
    check(wounded.useLayOnHands({0}, {0}, 2), "Paladin can heal self");
}
inline void paladinEffectsAndUi() {
    using namespace rules;
    const auto cat = paladinCatalog(); const auto map = hybridRoom();
    const NpcWorld empty(map, spellSpawn.cell, {"goblin"}, {});
    GameSession game(map, cat, empty, 9, spellSpawn, false);
    advancePaladin(game.party(), 3);
    game.party().damage({1}, 8);
    CommandController ui(game); ui.refresh({}, false, false, true); ui.selectCharacter(0); ui.openSpellbook();
    auto choose = [&](Feature f) {
        const auto& book = ui.snapshot().magic;
        for (std::size_t i = 0; i < book.count; ++i)
            if (book.entries[i].feature == f) { ui.chooseSpellbook(i); return; }
        throw std::runtime_error("Paladin feature not offered");
    };
    auto physicalFrame = [&](const UiInput& input, bool accept = true) {
        // Gameplay pre-routes buttons before world picking and HUD command routing.
        ui.refresh({}, false, false, input.focused);
        const auto logical = routeHudButtons(ui, input, accept);
        routeHudInput(ui, logical, {}, accept);
    };
    bool defense = false;
    for (std::size_t i = 0; i < ui.snapshot().magic.count; ++i)
        defense |= ui.snapshot().magic.entries[i].category == PickerEntryKind::AutomaticFeature &&
                   ui.snapshot().magic.entries[i].passive == PassiveFeature::Defense;
    check(defense, "Paladin Defense is discoverable as an automatic ability");
    choose(Feature::LayOnHands);
    UiInput pointer; pointer.pointerTracked = true; pointer.pointerHeld = true;
    pointer.pressed[std::size_t(UiKey::Click)] = true;
    pointer.pointer = ArtPoint{float(hudLayout::healingTrack.x + 50), float(hudLayout::healingTrack.y + 2)};
    physicalFrame(pointer);
    check(!ui.snapshot().magic.pending.targetCount && game.party().character({0})->layOnHands == 15,
          "Disabled slider consumes clicks without selecting or spending");
    pointer.pressed = {}; pointer.pointerHeld = false; pointer.pointerReleased = true;
    physicalFrame(pointer);
    game.party().damage({2}, 2);
    ui.refresh({}, false, false, true); ui.magicAlly(2);
    check(ui.snapshot().magic.pending.healingAmount == 2, "New ally defaults to its full missing health");
    const int wounds = std::min(20, game.party().character({0})->stats.maxHitPoints - 1);
    game.party().damage({0}, wounds); ui.refresh({}, false, false, true); ui.magicAlly(0);
    check(ui.snapshot().magic.pending.healingAmount == std::min(15, wounds), "Default is pool-limited when full healing is unavailable");
    ui.magicAlly(1); game.party().heal({0}, wounds);
    check(ui.snapshot().magic.pending.healingAmount == 8 && game.party().character({0})->layOnHands == 15, "Healing selection and maximum are non-mutating");
    pointer.pointerReleased = false; pointer.pointerHeld = true;
    pointer.pressed[std::size_t(UiKey::Click)] = true;
    pointer.pointer->x = float(hudLayout::healingTrack.x) + hudLayout::healingTrack.width * 4.f / 15;
    physicalFrame(pointer);
    check(ui.snapshot().magic.healingDragging, "Physical gameplay prepass retains slider capture");
    check(ui.snapshot().magic.pending.healingAmount == 4, "Slider press selects integer healing power");
    pointer.pressed = {}; pointer.pointer->x = 1000;
    physicalFrame(pointer);
    check(ui.snapshot().magic.pending.healingAmount == 8, "Dragging into overheal region clamps at full heal");
    pointer.pointer->x = 0; pointer.pointerHeld = false; pointer.pointerReleased = true;
    physicalFrame(pointer);
    check(ui.snapshot().magic.pending.healingAmount == 1 && !ui.snapshot().magic.healingDragging,
          "Release outside clamps to one and ends drag");
    UiInputEdges edges;
    std::array<bool, uiKeyCount> down{};
    for (int side : {-5, 5}) {
        ui.healingAmount(4 - ui.snapshot().magic.pending.healingAmount);
        const auto handle = hudLayout::healingHandle(4, 15);
        ArtPoint grab{float(handle.x + handle.width / 2 + side), float(handle.y + 1)};
        down[std::size_t(UiKey::Click)] = true;
        physicalFrame(edges.sample(down, true, grab));
        check(ui.snapshot().magic.healingDragging && ui.snapshot().magic.pending.healingAmount == 4,
              "Grabbing either handle edge preserves the selected amount");
        grab.x += hudLayout::healingTrack.width / 15.f;
        physicalFrame(edges.sample(down, true, grab));
        check(ui.snapshot().magic.pending.healingAmount == 5,
              "Held handle motion survives gameplay refresh and both input stages");
        down[std::size_t(UiKey::Click)] = false;
        grab = {float(hudLayout::spellCast.x + 10), float(hudLayout::spellCast.y + 5)};
        physicalFrame(edges.sample(down, true, grab));
        check(!ui.snapshot().magic.healingDragging && ui.snapshot().magic.armed &&
                  game.party().character({0})->layOnHands == 15,
              "Dropping the handle over Use does not cast or spend points");
    }
    down[std::size_t(UiKey::Click)] = true;
    physicalFrame(edges.sample(down, true, ArtPoint{60, 346}));
    physicalFrame(edges.sample(down, true, ArtPoint{200, 346}), false);
    check(!ui.snapshot().magic.healingDragging, "Unpresented input cancels slider capture");
    down[std::size_t(UiKey::Click)] = false;
    physicalFrame(edges.sample(down, true, ArtPoint{200, 346}));
    pointer.pointerReleased = false; pointer.pointerHeld = true; pointer.pressed[std::size_t(UiKey::Click)] = true;
    pointer.pointer->x = float(hudLayout::healingTrack.x + 20);
    physicalFrame(pointer);
    pointer.focused = false; pointer.pressed = {}; physicalFrame(pointer);
    check(!ui.snapshot().magic.healingDragging, "Focus loss ends slider capture");
    pointer.focused = true; pointer.pointer->x = 1000;
    const int heldAmount = ui.snapshot().magic.pending.healingAmount;
    physicalFrame(pointer);
    check(ui.snapshot().magic.pending.healingAmount == heldAmount,
          "Held pointer cannot resume a cancelled drag without a fresh press");
    UiInput key; key.pressed[std::size_t(UiKey::Up)] = true; key.shift = true;
    routeHudInput(ui, key);
    check(ui.snapshot().magic.pending.healingAmount == 6, "Shift-Up raises healing by five");
    routeHudInput(ui, key);
    check(ui.snapshot().magic.pending.healingAmount == 8, "Keyboard power clamps at full useful healing");
    key.pressed = {}; key.pressed[std::size_t(UiKey::Down)] = true;
    routeHudInput(ui, key);
    key.pressed = {}; key.pressed[std::size_t(UiKey::Up)] = true; key.shift = false;
    routeHudInput(ui, key);
    ui.magicAlly(1);
    check(ui.snapshot().magic.pending.healingAmount == 4, "Same recipient preserves dialed healing");
    check(ui.confirm() == CommandReason::Resolved && game.party().character({0})->layOnHands == 11, "UI confirms exact healing points");
    check(ui.confirm() != CommandReason::Resolved, "UI cannot repeat accepted heal");
    game.update(7, false); ui.refresh({}, false, false, true); ui.openSpellbook(); choose(Feature::SacredWeapon);
    check(ui.confirm() == CommandReason::Resolved, "Sacred Weapon uses shared ability UI");
    check(game.effects().sacredWeaponBonus({0}) == std::max(1, game.party().character({0})->stats.modifiers[5]), "Sacred Weapon applies CHA attack bonus");
    check(!game.party().character({0})->channelDivinity, "Sacred Weapon spends charge");
    auto protection = spellCommand(game, SpellKind::ProtectionFromEvilAndGood);
    protection.character = {0}; protection.allies[0] = {1}; protection.targetCount = 1;
    game.update(7, false); protection.turn = game.combat().snapshot().turn;
    check(game.submit(protection).reason == CommandReason::Resolved, "High-bit oath spell casts");
    check(game.effects().has(CharacterId{1}, EffectKind::ProtectionFromEvilAndGood) && game.effects().sacredWeaponBonus({0}) > 0, "Protection concentration coexists with Sacred Weapon");
    const auto h = game.party().inventory().at({{0}, std::uint16_t(EquipmentSlot::MainHand)});
    game.update(7, false);
    check(bool(game.inventoryChange({InventoryOp::Move, h, {{0}, 20}, 0, game.party().inventory().revision()})), "Unequip Sacred Weapon safely");
    check(!game.effects().sacredWeapon({0}).serial, "Unequip ends bound effect immediately");
    check(game.effects().concentration({0}).serial != 0, "Weapon removal does not remove Protection");
    game.party().damage({0}, 10000); game.update(.1, false);
    check(!game.effects().concentration({0}).serial, "Incapacitation ends Protection concentration");
}
inline void paladinResourcePresentation() {
    using namespace rules;
    const auto cat = paladinCatalog(); const auto map = hybridRoom();
    const NpcWorld empty(map, spellSpawn.cell, {"goblin"}, {});
    GameSession game(map, cat, empty, 9, spellSpawn, false);
    CommandController ui(game);
    for (int level = 1; level <= 4; ++level) {
        if (level > 1) advancePaladin(game.party(), level);
        game.party().damage({1}, 2);
        const int mana = game.party().character({0})->mana;
        check(game.party().useLayOnHands({0}, {1}, 1), "Spend healing pool for resource presentation");
        ui.refresh({}, false, false, true);
        const auto& card = ui.snapshot().cards[0];
        check(card.hasMana == (level >= 2) && card.mana == mana &&
                  card.maxMana == (level == 1 ? 0 : level == 2 ? 4 : 6),
              "Portrait mana remains distinct from spent healing points at every paladin level");
        ui.openInventory(CharacterId{0}, CharacterPage::Character);
        bool healing = false;
        const auto& sheet = ui.snapshot().inventory.sheet;
        char expected[32]; std::snprintf(expected, sizeof(expected), "%d / %d healing points", 4 * level, 5 * level);
        for (unsigned i = 0; i < sheet.featureCount; ++i)
            if (std::strcmp(sheet.features[i].name, "Lay on Hands") == 0) {
                healing = std::strcmp(sheet.features[i].value.data(), expected) == 0 &&
                          std::strstr(sheet.features[i].detail, "One point heals 1 HP") &&
                          std::strstr(sheet.features[i].detail, "long rest");
            }
        check(healing, "Sheet names current/max healing points and explains spending and recovery");
        ui.closeOverlay(); ui.selectCharacter(0); ui.openSpellbook();
        for (std::size_t i = 0; i < ui.snapshot().magic.count; ++i)
            if (ui.snapshot().magic.entries[i].feature == Feature::LayOnHands) {
                ui.chooseSpellbook(i); break;
            }
        ui.magicAlly(1);
        check(ui.snapshot().magic.healingPool == 4 * level &&
                  ui.snapshot().magic.healingCapacity == 5 * level &&
                  ui.snapshot().magic.healingMaximum == level,
              "Dock distinguishes remaining pool, total capacity and useful healing cap");
        ui.cancel();
    }
}
inline void paladinContinuousSlider() {
    using namespace rules;
    const auto cat = paladinCatalog(); const auto map = hybridRoom();
    const NpcWorld empty(map, spellSpawn.cell, {"goblin"}, {});
    GameSession game(map, cat, empty, 9, spellSpawn, false);
    game.party().damage({1}, 8);
    check(game.party().useLayOnHands({0}, {1}, 3), "Prepare a two-point healing pool");
    const int hp = game.party().character({1})->hitPoints;
    CommandController ui(game); ui.refresh({}, false, false, true); ui.selectCharacter(0); ui.openSpellbook();
    for (std::size_t i = 0; i < ui.snapshot().magic.count; ++i)
        if (ui.snapshot().magic.entries[i].feature == Feature::LayOnHands) {
            ui.chooseSpellbook(i); break;
        }
    ui.magicAlly(1);
    UiInputEdges edges; std::array<bool, uiKeyCount> down{};
    auto frame = [&](bool held, float amount) {
        down[std::size_t(UiKey::Click)] = held;
        const ArtPoint point{hudLayout::healingHandleCenter(amount, 2) + 4, float(hudLayout::healingTrack.y - 2)};
        ui.refresh({}, false, false, true);
        const auto logical = routeHudButtons(ui, edges.sample(down, true, point));
        routeHudInput(ui, logical);
    };
    frame(true, 2);
    check(ui.snapshot().magic.healingDragging && ui.snapshot().magic.pending.healingAmount == 2,
          "Two-point handle can be grabbed without jumping");
    frame(true, 1.1f);
    const float first = ui.snapshot().magic.healingDragAmount;
    frame(true, 1.2f);
    check(ui.snapshot().magic.pending.healingAmount == 1 &&
              ui.snapshot().magic.healingDragAmount > first + .09f &&
              std::abs(ui.snapshot().magic.healingDragAmount - 1.2f) < .001f,
          "Handle keeps moving within a single valid healing step");
    frame(true, 1.49f);
    check(ui.snapshot().magic.pending.healingAmount == 1, "Below the midpoint spends one point");
    const float before = ui.snapshot().magic.healingDragAmount;
    frame(true, 1.51f);
    check(ui.snapshot().magic.pending.healingAmount == 2 &&
              std::abs(ui.snapshot().magic.healingDragAmount - before - .02f) < .001f,
          "Crossing a snap threshold changes the integer draft without jumping the handle");
    frame(false, 1.51f);
    check(!ui.snapshot().magic.healingDragging && ui.snapshot().magic.pending.healingAmount == 2,
          "Release selects the nearest integer detent");
    frame(true, 2); frame(true, 1.25f);
    UiInput key; key.pressed[std::size_t(UiKey::Up)] = true;
    routeHudInput(ui, routeHudButtons(ui, key));
    check(!ui.snapshot().magic.healingDragging && ui.snapshot().magic.pending.healingAmount == 2,
          "Keyboard adjustment ends a fractional drag and displays its whole-point value");
    frame(false, 1.25f);
    check(game.party().character({0})->layOnHands == 2 && game.party().character({1})->hitPoints == hp,
          "Continuous motion, snapping and keyboard adjustment never spend or heal");
}
inline void paladinProtectionAndDamage() {
    using namespace rules;
    const auto map = hybridRoom();
    for (auto type : {CreatureType::Undead, CreatureType::Fiend, CreatureType::Beast}) {
        const auto cat = paladinCatalog(type);
        const auto places = spellPlacements(map, 1);
        GameSession game(map, cat, places, 2, spellSpawn, false); advancePaladin(game.party(), 3);
        auto cast = spellCommand(game, SpellKind::ProtectionFromEvilAndGood); cast.character = {0};
        cast.allies[0] = {1}; cast.targetCount = 1;
        check(game.submit(cast).reason == CommandReason::Resolved, "Protection setup");
        const auto mode = game.effects().attackMode(game.actors().id(0), CharacterId{1}, true, false, false);
        check(mode == (type == CreatureType::Beast ? RollMode::Normal : RollMode::Disadvantage), "Protection filters creature types");
        check(game.effects().attackMode(game.actors().id(0), CharacterId{1}, true, true, false) == (type == CreatureType::Beast ? RollMode::Advantage : RollMode::Normal), "Protection cancels advantage once");
        for (unsigned minute = 0; minute < 11; ++minute) game.update(60, false);
        check(!game.effects().has(CharacterId{1}, EffectKind::ProtectionFromEvilAndGood), "Protection expires on gameplay clock");
    }
    for (bool immuneRadiant : {false, true}) {
        const auto cat = paladinCatalog(CreatureType::Undead, 1, 1000,
            {immuneRadiant ? damageBit(DamageType::Radiant) : DamageMask{0}, damageBit(DamageType::Bludgeoning)}, true);
        const auto places = spellPlacements(map, 1);
        GameSession game(map, cat, places, 3, spellSpawn, false);
        DiceRng damageRng(7); EffectController damageEffects(game.party(), game.actors(), damageRng);
        DamagePacket packet; packet.count = 2;
        packet.components = {{{4, DamageType::Bludgeoning}, {5, DamageType::Radiant}}};
        const auto before = game.actors().actor(game.actors().id(0))->hitPoints;
        const auto result = damageNpc(game.actors(), damageEffects, game.actors().id(0), packet, false, 0);
        check(result.resolved.damage == (immuneRadiant ? 8 : 13), "Damage components mitigate separately");
        check(game.actors().actor(game.actors().id(0))->hitPoints == before - result.resolved.damage, "Mixed hit commits combined health once");
        packet.components[0].amount = 2000;
        const auto lethal = damageNpc(game.actors(), damageEffects, game.actors().id(0), packet, false, 1);
        check((lethal.fortitude.status == FortitudeStatus::RadiantBypass) == !immuneRadiant, "Only applied radiant bypasses Fortitude");
    }
}
inline void paladinSmites() {
    using namespace rules;
    const auto map = hybridRoom();
    for (auto type : {CreatureType::Undead, CreatureType::Fiend, CreatureType::Beast}) {
    bool missed = false, hit = false, critical = false;
    for (unsigned seed = 1; seed <= 100 && !(missed && hit && critical); ++seed) {
        const auto cat = paladinCatalog(type, 12);
        const NpcWorld places(map, spellSpawn.cell, {"goblin"}, {{{0}, {6, 7}, {0, 1}, Heading::South}});
        GameSession game(map, cat, places, seed, spellSpawn, false); advancePaladin(game.party(), 2);
        game.update(.1, false);
        for (unsigned step = 0; step < 300; ++step) {
            const auto& state = game.combat().snapshot();
            if (state.awaitingAction && state.activeCharacter && state.activeCharacter->index == 0) break;
            if (state.awaitingAction && state.activeCharacter) game.submit(featureCommand(game, state.activeCharacter->index, Feature::EndTurn));
            game.update(.1, false);
        }
        PartyCommand command = featureCommand(game, 0, Feature::DivineSmite);
        command.enemies[0] = game.actors().id(0); command.targetCount = 1;
        const auto oldMana = game.party().character({0})->mana;
        const auto accepted = game.submit(command);
        check(accepted.reason == CommandReason::Resolved, "Smite accepted with ordinary melee targeting");
        check(!accepted.hit && !accepted.critical && !accepted.smiteDamage, "Accepted Smite does not reveal saved damage rolls");
        check(game.submit(command).reason != CommandReason::Resolved, "Smite duplicate rejected");
        bool resolved = false;
        for (unsigned step = 0; step < 20 && !resolved; ++step) {
            const auto& events = game.update(.05, false);
            for (const auto& out : events)
                if (out.event == CombatEvent::Attack && !out.enemyAttack && out.character.index == 0) {
                    resolved = true;
                    if (!out.hit) { missed = true; check(game.party().character({0})->mana == oldMana, "Smite miss spends no mana"); }
                    else {
                        hit = true; critical |= out.critical;
                        check(game.party().character({0})->mana == oldMana - 2, "Smite hit spends two mana");
                        const int dice = (type == CreatureType::Beast ? 2 : 3) * (out.critical ? 2 : 1);
                        check(out.smiteDamage >= dice && out.smiteDamage <= 8 * dice, "Undead Smite and critical dice bounds");
                        check(out.damage == out.damageComponents[0].damage + out.damageComponents[1].damage, "Smite impact reports both typed components");
                    }
                }
        }
        check(resolved, "Smite resolves through attack timeline");
    }
    check(missed && hit && critical, "Deterministic seeds cover Smite miss/hit/critical");
    }
}
inline void paladinChecks() {
    highSpellRewardProfile(); paladinRulesAndResources(); paladinEffectsAndUi(); paladinResourcePresentation(); paladinContinuousSlider(); paladinProtectionAndDamage(); paladinSmites();
}
} // namespace blobber::test

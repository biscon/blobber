#include "game/arena.hpp"
#include "game/commands.hpp"
#include "support/check.hpp"
#include "support/progression_fixtures.hpp"
#include "ui/interaction.hpp"
#include <iostream>
using namespace blobber;
using namespace blobber::rules;
namespace {
void spells() {
    const auto map = hybridRoom();
    const auto catalog = creationCatalog(true);
    for (unsigned k = unsigned(SpellKind::Aid); k < unsigned(SpellKind::Count); ++k) {
        const auto kind = SpellKind(k);
        const auto places = spellPlacements(map, 3);
        GameSession s(map, catalog, places, 11, spellSpawn, false);
        levelParty(s, 3, kind);
        auto command = spellCommand(s, kind);
        if (kind == SpellKind::Aid) {
            command.allies = {{{0}, {1}, {2}}};
            command.targetCount = 3;
        }
        if (kind == SpellKind::HoldPerson || kind == SpellKind::SpiritualWeapon ||
            kind == SpellKind::ScorchingRay) {
            command.enemies[0] = s.actors().id(0);
            command.targetCount = 1;
            command.darts[0] = 3;
        }
        if (kind == SpellKind::Shatter || kind == SpellKind::Web)
            command.aimedCell = spellSpawn.cell;
        CombatEvents eventBuffer;
        if (kind == SpellKind::LesserRestoration) {
            EffectRequest hold;
            hold.world = s.actors().snapshot().world;
            hold.caster = rules::CharacterId{3};
            hold.kind = EffectKind::HoldPerson;
            hold.targets[0] = CharacterId{0};
            hold.count = 1;
            hold.difficulty = 100;
            check(bool(s.applyEffect(hold)), "paralysis fixture");
            command.restoreParalysis = true;
            command.allies[0] = {0};
            command.targetCount = 1;
        }
        const auto oldMana = s.party().character(command.character)->mana;
        const int oldHp = s.party().character({0})->hitPoints, wizardHp = s.party().character({3})->hitPoints;
        const auto result = s.submit(command);
        check(result.reason == CommandReason::Resolved,
              (std::string(spellRules(kind)->id) + ": " + commandMessage(result.reason)).c_str());
        check(s.party().character(command.character)->mana == oldMana - 3,
              "all level-two spells cost three mana");
        check(s.submit(command).reason != CommandReason::Resolved, "repeated command cannot pay/cast twice");
        if (kind == SpellKind::Aid)
            check(s.party().character({0})->hitPoints == oldHp + 5, "Aid adds HP");
        if (kind == SpellKind::MirrorImage)
            check(s.effects().has(CharacterId{3}, EffectKind::MirrorImage), "Mirror self effect applied");
        if (kind == SpellKind::LesserRestoration)
            check(!s.effects().has(CharacterId{0}, EffectKind::HoldPerson), "selected paralysis cured");
        if (kind == SpellKind::Shatter)
            check(s.party().character({0})->hitPoints == oldHp &&
                      s.party().character({3})->hitPoints < wizardHp,
                  "Sculpt protects companions and leaves caster exposed");
        if (kind == SpellKind::Web) {
            unsigned cells = 0;
            for (const auto& a : s.effects().ground().areas)
                cells += a.active && a.web;
            check(cells == 1 && s.effects().concentration({3}).serial,
                  "Web reserves one logical cast");
            for (const auto& a : s.effects().ground().areas)
                if (a.active && a.web) check(a.footprint.count() == 16, "Web covers sixteen subcells");
        }
        if (kind == SpellKind::ScorchingRay) {
            unsigned rays = 0;
            for (int n = 0; n < 10; ++n)
                for (const auto& e : s.update(.2, false))
                    rays += e.event == CombatEvent::Attack && e.spell == kind;
            check(rays == 3, "three independent delayed ray outcomes even on one target");
        }
        if (kind == SpellKind::SpiritualWeapon) {
            s.update(6, false);
            auto c = featureCommand(s, 1, Feature::SpiritualWeapon);
            c.spell = command.spell;
            c.aimedCell = Cell{7, 4};
            check(s.submit(c).reason == CommandReason::Resolved,
                  "spectral weapon may move without attacking");
            check(s.effects().spiritualWeapon({1})->cell == Cell{7, 4}, "spectral position published");
            c.turn = s.combat().snapshot().turn;
            check(s.submit(c).reason == CommandReason::Cooldown, "spectral followup has its own cooldown");
        }
    }
}
void weaponFollowsMovement() {
    const auto map = hybridRoom();
    const auto catalog = creationCatalog(true);
    for (unsigned h = 0; h < 4; ++h)
        for (bool animated : {false, true}) {
            const Player pose{{6, 6}, Heading(h)};
            const auto f = forward(pose.heading);
            NpcWorld places(map, pose.cell, {"goblin"}, {});
            GameSession game(map, catalog, places, 5, pose, false);
            game.setAnimated(animated);
            EffectRequest request;
            request.world = game.actors().snapshot().world;
            request.kind = EffectKind::SpiritualWeapon;
            request.caster = rules::CharacterId{1};
            request.targets[0] = rules::CharacterId{1};
            request.count = 1;
            check(bool(game.applyEffect(request)), "Follower fixture summons a live weapon");
            auto& effects = const_cast<EffectController&>(game.effects());
            const auto start = pose.cell + f;
            check(effects.moveWeapon({1}, start, true), "Follower fixture positions the weapon");
            const auto ready = effects.spiritualWeapon({1})->readyMs;
            const auto expires = effects.spiritualWeapon({1})->expiresMs;
            const auto mana = game.party().character({1})->mana;
            game.move(Action::Forward);
            if (animated) {
                check(effects.spiritualWeapon({1})->cell == start,
                      "Animated step leaves authoritative weapon at source until completion");
                game.update(.08, true);
                check(effects.spiritualWeapon({1})->cell == start,
                      "Paused movement cannot commit a following step");
                game.move(Action::Forward); // Queue the next step at the first endpoint.
                game.update(.16, false);
                check(effects.spiritualWeapon({1})->cell == start + f,
                      "Queued movement commits the first weapon step exactly once");
                game.setAnimated(false);
                check(effects.spiritualWeapon({1})->cell == start + f * 2,
                      "Disabling animation settles the queued weapon step exactly once");
                game.setAnimated(false);
            }
            const auto expected = start + f * (animated ? 2 : 1);
            check(effects.spiritualWeapon({1})->cell == expected &&
                      effects.ground().weapons[1].cell == expected,
                  "Party advance moves authoritative and rendered weapon cells together");
            check(effects.spiritualWeapon({1})->readyMs == ready &&
                      effects.spiritualWeapon({1})->expiresMs == expires &&
                      game.party().character({1})->mana == mana,
                  "Following does not spend commands, mana or reset duration/cooldown");
            game.move(Action::TurnRight);
            check(effects.spiritualWeapon({1})->cell == expected, "Turning does not translate the weapon");
            auto save = std::make_unique<FloorState>();
            game.exportFloor(*save);
            game.importFloor(*save);
            check(game.effects().spiritualWeapon({1})->cell == expected,
                  "Restore retains the followed weapon cell without applying another step");
        }
    const Map wall({"#######", "#.....#", "#..#..#", "#.....#", "#.....#", "#.....#", "#######"});
    check(followingWeaponCell(wall, {3, 3}, {3, 4}, {3, 3}) == Cell{3, 4},
          "Blocked follower retreats into the vacated party cell instead of the camera");
    check(followingWeaponCell(wall, {3, 3}, {4, 4}, {4, 3}) == Cell{3, 3},
          "Blocked remote follower stays put rather than crossing a wall");
    FloorFixtures doors;
    doors.count = 1;
    doors.entries[0] = {{4, 2}, Heading::North, FixtureKind::Door};
    FloorGeometry geometry(wall, doors);
    check(followingWeaponCell(geometry.view(), {4, 3}, {3, 4}, {3, 3}) == Cell{4, 3},
          "Following respects closed doors as well as walls");
    NpcWorld empty(wall, {3, 3}, {"goblin"}, {});
    GameSession blocked(wall, catalog, empty, 5, {{3, 3}, Heading::North}, false);
    blocked.setAnimated(false);
    EffectRequest request;
    request.world = blocked.actors().snapshot().world;
    request.kind = EffectKind::SpiritualWeapon;
    request.caster = rules::CharacterId{1};
    request.targets[0] = rules::CharacterId{1};
    request.count = 1;
    check(bool(blocked.applyEffect(request)), "Blocked-step fixture summons a weapon");
    auto& effects = const_cast<EffectController&>(blocked.effects());
    check(effects.moveWeapon({1}, {4, 3}, true), "Blocked-step fixture positions the weapon");
    const auto revision = effects.ground().revision;
    blocked.move(Action::Forward);
    blocked.update(.16, false);
    check(blocked.movement().player.cell == Cell{3, 3} && effects.spiritualWeapon({1})->cell == Cell{4, 3} &&
              effects.ground().revision == revision,
          "Rejected party movement cannot move or invalidate the weapon");
}
void weaponNextTurnControls() {
    const auto map = hybridRoom();
    const auto catalog = creationCatalog(true);
    const auto places = spellPlacements(map, 4, true);
    for (bool normalFirst : {false, true})
        for (bool floorOnly : {false, true}) {
            GameSession s(map, catalog, places, 7, spellSpawn, false);
            levelParty(s, 3, SpellKind::SpiritualWeapon);
            levelReady(s, 1);
            const auto round = s.combat().snapshot().round;
            auto cast = spellCommand(s, SpellKind::SpiritualWeapon);
            cast.targetCount = 1;
            cast.enemies[0] = s.actors().id(0);
            check(s.submit(cast).reason == CommandReason::Resolved, "Cleric summons weapon on her turn");
            s.update(1.1, false);
            levelReady(s, 1);
            check(s.combat().snapshot().round > round, "Follow-up reaches cleric's next round");
            const auto mana = s.party().character({1})->mana;
            if (normalFirst) {
                auto defend = featureCommand(s, 1, Feature::None);
                defend.action = PartyAction::Defend;
                check(s.submit(defend).reason == CommandReason::Resolved,
                      "Cleric spends normal action first");
                s.update(1.1, false);
            }
            CommandController ui(s);
            TargetSnapshot targets;
            targets.world = s.actors().snapshot().world;
            targets.revision = s.actors().snapshot().revision;
            targets.ids[0] = s.actors().id(0);
            targets.count = 1;
            ui.refresh(targets, false, false, true);
            ui.selectCharacter(1);
            ui.openSpellbook();
            ui.selectSpellbookTab(MagicHud::abilityTab);
            const auto& book = ui.snapshot().magic;
            std::size_t index = book.entries.size();
            for (std::size_t i = 0; i < book.visibleCount; ++i)
                if (book.entries[book.visible[i]].feature == Feature::SpiritualWeapon)
                    index = book.visible[i];
            check(index < book.count && book.entries[index].reason == CommandReason::MissingTarget,
                  "Next-turn command is available under Abilities, before or after normal action");
            ui.chooseSpellbook(index);
            check(ui.magicSingleTarget() && ui.snapshot().targetPicker.available,
                  "Weapon command uses immediate single-enemy targeting with the normal target list");
            UiInput click;
            click.pointer = ArtPoint{float(hudLayout::viewport.x + 20), float(hudLayout::viewport.y + 20)};
            click.pressed[std::size_t(UiKey::Click)] = true;
            const auto destination = s.effects().spiritualWeapon({1})->cell + Cell{1, 0};
            if (floorOnly) {
                ui.magicEnemy(
                    s.actors().id(0)); // An earlier keyboard selection must not survive a floor click.
                routeHudInput(ui, click, {}, true, {}, destination);
                check(ui.snapshot().magic.armed && !ui.snapshot().magic.pending.target &&
                          !ui.snapshot().magic.pending.targetCount &&
                          ui.snapshot().magic.pending.aimedCell == destination,
                      "Floor click clears old enemy and waits for movement confirmation");
                UiInput confirm;
                confirm.pressed[std::size_t(UiKey::Confirm)] = true;
                routeHudInput(ui, confirm);
                check(s.effects().spiritualWeapon({1})->cell == destination,
                      "Keyboard confirms movement-only command");
            } else {
                routeHudInput(ui, click, s.actors().id(0), true, {}, s.effects().spiritualWeapon({1})->cell);
                check(ui.snapshot().magic.state == CastingState::Resolving,
                      "One enemy click immediately executes Spiritual Weapon, without another confirmation");
                routeHudInput(ui, click, s.actors().id(0));
                check(ui.confirm() != CommandReason::Resolved, "Double input cannot issue a second attack");
            }
            unsigned attacks = 0;
            for (int n = 0; n < 12; ++n)
                for (const auto& e : s.update(.1, false))
                    attacks += e.event == CombatEvent::Attack && e.spell == SpellKind::SpiritualWeapon;
            check(attacks == (floorOnly ? 0u : 1u) && s.party().character({1})->mana == mana,
                  "Click attack or floor-only move resolves the intended action without extra mana");
            if (!normalFirst) {
                check(!s.combat().snapshot().spent[1], "Weapon command preserves normal action");
                auto repeat = featureCommand(s, 1, Feature::SpiritualWeapon);
                repeat.spell = cast.spell;
                repeat.aimedCell = s.effects().spiritualWeapon({1})->cell;
                check(s.submit(repeat).reason == CommandReason::Cooldown,
                      "Weapon cannot be commanded twice in the same round");
            }
        }
}
void immediateActionSurge() {
    const auto map = hybridRoom();
    const auto catalog = creationCatalog(true);
    const auto places = spellPlacements(map, 4, true);
    for (bool normalFirst : {false, true}) {
        GameSession s(map, catalog, places, 7, spellSpawn, false);
        levelParty(s, 2);
        levelReady(s, 0);
        if (normalFirst) {
            auto defend = featureCommand(s, 0, Feature::None);
            defend.action = PartyAction::Defend;
            check(s.submit(defend).reason == CommandReason::Resolved, "Fighter uses normal action first");
            s.update(1.1, false);
        }
        CommandController ui(s);
        ui.refresh(allCombatTargets(s), false, false, true);
        ui.selectCharacter(0);
        ui.openSpellbook();
        ui.selectSpellbookTab(MagicHud::abilityTab);
        const auto& book = ui.snapshot().magic;
        std::size_t row = 0;
        while (row < book.visibleCount && book.entries[book.visible[row]].feature != Feature::ActionSurge)
            ++row;
        check(row < book.visibleCount, "Action Surge is listed in Abilities");
        const auto index = book.visible[row];
        const auto rect = hudLayout::spellEntry(int(row - book.offset));
        UiInput click;
        click.pointer = ArtPoint{float(rect.x + 2), float(rect.y + 2)};
        click.pressed[std::size_t(UiKey::Click)] = true;
        routeHudInput(ui, click);
        check(!s.party().character({0})->actionSurge && !ui.snapshot().magic.casting() &&
                  !ui.snapshot().magic.open,
              "One Abilities click spends Action Surge immediately without a confirmation panel");
        ui.chooseSpellbook(index);
        unsigned surges = 0;
        for (const auto& event : s.events())
            surges += event.event == CombatEvent::Recovery && event.feature == Feature::ActionSurge;
        check(surges == 1, "Repeated selection cannot consume or grant Action Surge again");
        s.update(1.1, false);
        auto defend = featureCommand(s, 0, Feature::None);
        defend.action = PartyAction::Defend;
        check(s.submit(defend).reason == CommandReason::Resolved,
              "Immediately selected surge grants its action");
    }
}
void aidAndMirror() {
    const auto map = hybridRoom();
    const auto catalog = creationCatalog(true);
    const auto places = spellPlacements(map, 1);
    GameSession s(map, catalog, places, 5, spellSpawn, false);
    levelParty(s, 2);
    auto& p = s.party();
    DiceRng rng(5);
    EffectController e(p, s.actors(), rng);
    e.partyCell(spellSpawn.cell);
    CombatEvents events;
    EffectRequest r;
    r.world = s.actors().snapshot().world;
    r.caster = rules::CharacterId{1};
    r.kind = EffectKind::Aid;
    r.count = 1;
    r.targets[0] = CharacterId{0};
    const int original = p.character({0})->hitPoints;
    const auto aid = e.apply(r, 0, events);
    check(bool(aid), "Aid effect accepted");
    p.damage({0}, 3);
    e.apply(r, 0, events);
    check(p.character({0})->hitPoints == original + 2 && p.character({0})->aidBonus == 5,
          "Aid refresh does not heal or stack");
    const auto bonusMax = p.character({0})->stats.maxHitPoints;
    check(bool(p.acquire({{0}, {10}, 1, p.inventory().revision()})), "acquire while Aid active");
    check(p.character({0})->stats.maxHitPoints == bonusMax && p.character({0})->hitPoints == original + 2,
          "inventory derivation preserves Aid maximum and current HP");
    p.grantNextLevelExperience();
    check(bool(p.advance(p.advancementRequest({0}))), "advancement while Aid active");
    const auto max = p.character({0})->stats.maxHitPoints;
    e.advance(28800000, 28800000, true, events);
    check(p.character({0})->stats.maxHitPoints == max - 5 && p.character({0})->hitPoints <= max - 5,
          "Aid expiry removes bonus after advancement");
}
void effects() {
    const auto map = hybridRoom();
    const auto catalog = creationCatalog(true);
    const auto places = spellPlacements(map, 1);
    GameSession s(map, catalog, places, 5, spellSpawn, false);
    levelParty(s, 3);
    auto& p = s.party();
    DiceRng rng(5);
    EffectController e(p, s.actors(), rng);
    e.partyCell(spellSpawn.cell);
    CombatEvents events;
    EffectRequest r;
    r.world = s.actors().snapshot().world;
    r.caster = rules::CharacterId{3};
    r.kind = EffectKind::MirrorImage;
    r.count = 1;
    r.targets[0] = CharacterId{3};
    auto mirror = e.apply(r, 0, events);
    check(bool(mirror), "Mirror accepted");
    std::array<MirrorReservation, 3> reservations{};
    unsigned n = 0;
    for (unsigned trial = 0; trial < 100 && n < 3; ++trial) {
        auto hit = e.mirror(s.actors().id(0), {3}, 20, 100);
        if (hit.effect.serial)
            reservations[n++] = hit;
    }
    check(n == 3 && e.find(mirror.id)->duplicates == 7,
          "pending images reserved, not destroyed at acceptance");
    check(!e.mirror(s.actors().id(0), {3}, 20, 100).effect.serial,
          "reserved images cannot be consumed again");
    e.resolveMirror(reservations[0], true, 0, events);
    check(e.find(mirror.id)->duplicates == 7, "canceling hit releases reservation");
    e.resolveMirror(reservations[1], false, 0, events);
    e.resolveMirror(reservations[2], false, 0, events);
    check(e.find(mirror.id)->duplicates == reservations[0].duplicate, "only committed images destroyed");
    e.remove(mirror.id, 0, events);
    e.resolveMirror(reservations[0], false, 0, events);
    const std::array<Cell, 4> cells{{spellSpawn.cell, spellSpawn.cell + Cell{1, 0},
                                     spellSpawn.cell + Cell{0, -1}, spellSpawn.cell + Cell{1, -1}}};
    e.advance(12000, 12000, true, events);
    check(e.createWeb(cells.data(), 4, {3}, 100, 12000, events), "Web creation accepted");
    check(e.has(CharacterId{0}, EffectKind::Restrained) && !e.mobileParty() && e.canAct(CharacterId{0}),
          "restraint blocks movement, permits actions");
    check(e.attackMode(CharacterId{0}, s.actors().id(0), true) == RollMode::Disadvantage,
          "restrained attack disadvantage");
    const auto old = p.character({0})->hitPoints;
    e.ignite(spellSpawn.cell);
    e.advance(18000, 18000, true, events);
    check(e.gameplayMs() == 18000, "late Web creation never rewinds effect clock");
    check(p.character({0})->hitPoints < old && !e.has(CharacterId{0}, EffectKind::Restrained),
          "burn damages occupants and frees burned cell");
    e.partyCell(spellSpawn.cell + Cell{1, 0});
    e.groundOccupants(spellSpawn.cell + Cell{1, 0}, 18000, events);
    check(e.has(CharacterId{0}, EffectKind::Restrained), "entering surviving Web restrains again");
    e.partyCell(spellSpawn.cell + Cell{-1, 0});
    e.groundOccupants(spellSpawn.cell + Cell{-1, 0}, 18000, events);
    check(!e.has(CharacterId{0}, EffectKind::Restrained) && e.mobileParty(),
          "leaving Web clears restraint for carried companions");
    const auto concentration = e.concentration({3});
    if (concentration.serial)
        e.remove(concentration, 18000, events);
    for (const auto& a : e.ground().areas)
        check(!a.active, "dismissing Web removes whole footprint");
}
void features() {
    const auto map = hybridRoom();
    const auto catalog = creationCatalog(true);
    const auto places = spellPlacements(map, 0);
    GameSession s(map, catalog, places, 9, spellSpawn, false);
    levelParty(s, 3);
    s.party().damage({0}, 5);
    auto wind = featureCommand(s, 0, Feature::SecondWind);
    check(s.submit(wind).reason == CommandReason::Resolved, "normal action first");
    s.update(1.1, false);
    check(s.submit(featureCommand(s, 0, Feature::ActionSurge)).reason == CommandReason::Resolved,
          "Action Surge after normal action");
    s.update(1.1, false);
    s.party().damage({1}, s.party().character({1})->hitPoints - 1);
    const auto pool = s.party().preserveLifePreview({1});
    check(pool[1] > 0 && pool[1] <= s.party().character({1})->stats.maxHitPoints / 2 - 1,
          "Preserve Life preview obeys half-max cap");
    check(s.submit(featureCommand(s, 1, Feature::PreserveLife)).reason == CommandReason::Resolved,
          "Preserve Life executes");
    check(!s.party().character({1})->channelDivinity, "channel charge spent");
    check(s.submit(featureCommand(s, 2, Feature::CunningAction)).reason == CommandReason::Resolved,
          "Cunning Action executes freely");
    check(s.combat().snapshot().defending[2] && !s.combat().snapshot().spent[2],
          "free Defend preserves normal action");
}
void turnAllowances() {
    const auto map = hybridRoom();
    const auto catalog = creationCatalog(true);
    const auto places = spellPlacements(map, 4, true);
    for (unsigned id : {0u, 2u})
        for (bool optionalFirst : {false, true}) {
            GameSession s(map, catalog, places, 7, spellSpawn, false);
            levelParty(s, 3);
            levelReady(s, id);
            auto normal = [&] {
                PartyCommand c = featureCommand(s, id, Feature::None);
                c.action = PartyAction::Defend;
                check(s.submit(c).reason == CommandReason::Resolved, "normal action in either-order turn");
            };
            auto optional = [&] {
                check(s.submit(featureCommand(s, id, id == 0 ? Feature::ActionSurge : Feature::CunningAction))
                              .reason == CommandReason::Resolved,
                      "optional activation in either order");
                s.update(1.1, false);
            };
            const auto round = s.combat().snapshot().round;
            const auto clock = s.combat().snapshot().gameplayMs;
            const auto stale = featureCommand(s, id, Feature::EndTurn);
            if (optionalFirst) {
                optional();
                normal();
            } else {
                normal();
                if (id == 0)
                    optional();
            }
            if (id == 2) {
                check(!s.combat().snapshot().activeCharacter ||
                          s.combat().snapshot().activeCharacter->index != 2,
                      "rogue automatically advances after the last normal action");
                continue;
            }
            check(s.combat().snapshot().round == round && s.combat().snapshot().gameplayMs == clock,
                  "reopened action does not advance round clock");
            if (id == 0) {
                check(s.combat().snapshot().activeCharacter &&
                          s.combat().snapshot().activeCharacter->index == id,
                      "surged action retains actor");
                check(s.submit(stale).reason == CommandReason::StaleTurn,
                      "reopened action invalidates old token");
                normal();
                check(!s.party().character({0})->actionSurge, "surge spends its rest charge");
            }
        }
    GameSession thief(map, catalog, places, 7, spellSpawn, false);
    levelParty(thief, 3);
    levelReady(thief, 2);
    thief.party().damage({2}, 3);
    auto normal = featureCommand(thief, 2, Feature::None);
    normal.action = PartyAction::Defend;
    PartyCommand potion = featureCommand(thief, 2, Feature::None);
    potion.action = PartyAction::Item;
    potion.ally = CharacterId{2};
    for (std::uint16_t i = 0; i < bagCells; ++i) {
        const auto h = thief.party().inventory().at({{2}, i});
        const auto* stack = thief.party().inventory().stack(h);
        if (stack && catalog.item(stack->item)->consumable == Consumable::Healing) {
            potion.item = h;
            break;
        }
    }
    potion.inventoryRevision = thief.party().inventory().revision();
    check(thief.submit(potion).reason == CommandReason::Resolved,
          "Thief can spend its optional allowance before the normal action");
    thief.update(1.1, false);
    check(thief.submit(featureCommand(thief, 2, Feature::CunningAction)).reason != CommandReason::Resolved,
          "potion and Cunning Action share allowance");
    normal = featureCommand(thief, 2, Feature::None);
    normal.action = PartyAction::Defend;
    check(thief.submit(normal).reason == CommandReason::Resolved &&
              (!thief.combat().snapshot().activeCharacter ||
               thief.combat().snapshot().activeCharacter->index != 2),
          "normal action ends the Thief turn after a quick potion");
}

void automaticCunningDefend() {
    const auto map = hybridRoom();
    const auto catalog = creationCatalog(true);
    const auto places = spellPlacements(map, 4, true);
    for (int level : {2, 3})
        for (bool manualFirst : {false, true}) {
            GameSession s(map, catalog, places, 7, spellSpawn, false);
            levelParty(s, level);
            levelReady(s, 2);
            if (manualFirst) {
                check(s.submit(featureCommand(s, 2, Feature::CunningAction)).reason ==
                          CommandReason::Resolved,
                      "rogue can still defend before acting");
                s.update(1.1, false);
            }
            const auto end = featureCommand(s, 2, Feature::EndTurn);
            check(s.submit(end).reason == CommandReason::Resolved && s.combat().snapshot().defending[2],
                  "End Turn leaves a conscious rogue defending");
            unsigned defences = 0;
            for (const auto& event : s.events())
                if (event.event == CombatEvent::Defend && event.character.index == 2 &&
                    event.feature == Feature::CunningAction)
                    ++defences;
            check(defences == (manualFirst ? 0u : 1u),
                  "only an unused allowance emits automatic defence, without duplicate activation");
            check(s.submit(end).reason != CommandReason::Resolved,
                  "repeated End Turn cannot grant another defence");
        }
}
void rogueAttackCompletion() {
    const auto map = hybridRoom();
    const auto catalog = creationCatalog(true);
    const NpcWorld places(map, spellSpawn.cell, {"goblin"},
                          {{{0}, {6, 7}, {0, 0}, Heading::South}, {{0}, {6, 7}, {1, 0}, Heading::South}});
    for (int level : {2, 3})
        for (auto kind : {PartyAction::BackMelee, PartyAction::Ranged})
            for (bool manual : {false, true}) {
                GameSession s(map, catalog, places, 7, spellSpawn, false);
                levelParty(s, level);
                levelReady(s, 2);
                if (manual) {
                    check(s.submit(featureCommand(s, 2, Feature::CunningAction)).reason ==
                              CommandReason::Resolved,
                          "early defence legal");
                    s.update(1.1, false);
                }
                auto attack = featureCommand(s, 2, Feature::None);
                attack.action = kind;
                attack.target = s.actors().id(0);
                check(s.submit(attack).reason == CommandReason::Resolved, "rogue weapon attack accepted");
                check(s.combat().snapshot().activeCharacter &&
                          s.combat().snapshot().activeCharacter->index == 2 &&
                          !s.combat().snapshot().awaitingAction,
                      "attack resolution finishes before initiative advances");
                unsigned defences = 0;
                for (unsigned tick = 0; tick < 40; ++tick) {
                    s.update(.1, false);
                    for (const auto& e : s.events())
                        if (e.event == CombatEvent::Defend && e.character.index == 2 &&
                            e.feature == Feature::CunningAction)
                            ++defences;
                    if (!s.combat().snapshot().activeCharacter ||
                        s.combat().snapshot().activeCharacter->index != 2)
                        break;
                }
                check((!s.combat().snapshot().activeCharacter ||
                       s.combat().snapshot().activeCharacter->index != 2) &&
                          s.combat().snapshot().defending[2] && defences == (manual ? 0u : 1u),
                      "melee/ranged resolution defends once and advances without another input");
            }
}
void rogueInterruptedAttack() {
    const auto map = hybridRoom();
    const auto catalog = creationCatalog(true);
    const NpcWorld places(map, spellSpawn.cell, {"goblin"}, {{{0}, {6, 7}, {0, 0}, Heading::South}});
    for (bool terminal : {false, true}) {
        GameSession s(map, catalog, places, 7, spellSpawn, false);
        levelParty(s, 2);
        levelReady(s, 2);
        auto attack = featureCommand(s, 2, Feature::None);
        attack.action = PartyAction::Ranged;
        attack.target = s.actors().id(0);
        check(s.submit(attack).reason == CommandReason::Resolved,
              "interruption fixture releases a legal attack");
        if (terminal)
            s.actors().damage(s.actors().id(0), 10000, s.timeMs());
        else
            s.party().damage({2}, s.party().character({2})->hitPoints);
        unsigned defence = 0;
        for (unsigned i = 0; i < 40; ++i) {
            s.update(.1, false);
            for (const auto& e : s.events())
                if (e.event == CombatEvent::Defend && e.character.index == 2)
                    ++defence;
            if (s.combat().snapshot().phase == CombatPhase::Victory ||
                !s.combat().snapshot().activeCharacter || s.combat().snapshot().activeCharacter->index != 2)
                break;
        }
        check(!defence, "downed rogue and terminal combat do not gain automatic defence");
        if (terminal)
            check(s.combat().snapshot().phase == CombatPhase::Victory,
                  "target disappearing during attack reaches victory");
        else
            check(!s.combat().snapshot().activeCharacter || s.combat().snapshot().activeCharacter->index != 2,
                  "downed attacker does not hold initiative open");
    }
}
void capacityStress() {
    const auto map = arenaMap();
    const auto catalog = creationCatalog(true);
    const Player spawn{{10, 20}, Heading::North};
    std::vector<NpcInstance> list;
    for (unsigned i = 0; i < 256; ++i)
        list.push_back(
            {{0}, {2 + int(i / 4 % 8), 2 + int(i / 32)}, {int(i % 2), int(i / 2 % 2)}, Heading::South});
    const NpcWorld places(map, spawn.cell, {"goblin"}, list);
    Party party(catalog);
    ActorWorld actors(map, catalog, places);
    DiceRng rng(19);
    EffectController effects(party, actors, rng);
    CombatEvents events;
    effects.partyCell(spawn.cell);
    for (std::uint16_t caster = 0; caster < 4; ++caster) {
        const int x = 2 + 2 * (caster % 2), y = 2 + 2 * (caster / 2);
        const std::array<Cell, 4> cells{{{x, y}, {x + 1, y}, {x, y + 1}, {x + 1, y + 1}}};
        check(effects.createWeb(cells.data(), 4, {caster}, 100, 0, events),
              "full Web capacity with maximum population");
    }
    unsigned active = 0, restrained = 0;
    for (const auto& a : effects.ground().areas)
        active += a.active;
    for (unsigned i = 0; i < 256; ++i)
        restrained += effects.has(actors.id(i), EffectKind::Restrained);
    check(active == 4 && restrained == 64, "four Web casts restrain sixty-four occupants");
    for (const auto& a : effects.ground().areas)
        if (a.active) effects.ignite(a.footprint);
    events.clear();
    effects.advance(6000, 6000, true, events);
    check(events.count < CombatEvents::capacity, "full burn batch fits fixed events");
    for (const auto& a : effects.ground().areas)
        check(!a.active, "all burned cells release capacity");
    for (unsigned i = 0; i < EffectController::capacity; ++i) {
        EffectRequest r;
        r.world = actors.snapshot().world;
        r.caster = rules::CharacterId{3};
        r.kind = EffectKind::MageArmor;
        r.count = 1;
        r.targets[0] = CharacterId{3};
        check(bool(effects.apply(r, 6000, events)), "fill ordinary effect pool");
    }
    const Cell cell{7, 7};
    check(!effects.canCreateWeb(&cell, 1, {3}), "full ordinary pool rejects Web before replacing anything");
    std::cout << "Maximum population/effect/Web stress passed: " << events.count << " events\n";
}

} // namespace
int main(int argc, char**) {
    try {
        if (argc > 1) {
            capacityStress();
            return 0;
        }
        spells();
        weaponNextTurnControls();
        weaponFollowsMovement();
        immediateActionSurge();
        aidAndMirror();
        effects();
        features();
        turnAllowances();
        automaticCunningDefend();
        rogueAttackCompletion();
        rogueInterruptedAttack();
        std::cout << "Level spell checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

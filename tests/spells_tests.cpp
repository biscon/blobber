#include "game/commands.hpp"
#include "support/check.hpp"
#include "support/spell_fixtures.hpp"
#include "ui/interaction.hpp"
#include "ui/spell_layout.hpp"
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
using namespace blobber;
using rules::SpellKind;
namespace {
void resourcesAndHealing() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    const auto places = spellPlacements(map, 0);
    GameSession s(map, catalog, places, 5, spellSpawn, false);
    check(s.party().character({0})->maxMana == 0 && s.party().character({1})->mana == 4 &&
              s.party().character({3})->mana == 4,
          "Only casters start with four mana");
    check(rules::maximumMana(rules::Casting::Wisdom, 2) == 6 &&
              rules::maximumMana(rules::Casting::Intelligence, 3) == 14,
          "Defined later-level mana progression");
    auto heal = spellCommand(s, SpellKind::CureWounds);
    heal.allies[0] = {0};
    heal.targetCount = 1;
    check(s.submit(heal).reason == CommandReason::FullHealth && s.party().character({1})->mana == 4,
          "Full healing target rejects without cost");
    s.party().damage({0}, 1000);
    check(s.submit(heal).reason == CommandReason::DeadAlly, "Healing never resurrects");
    s.party().damage({2}, s.party().character({2})->hitPoints);
    s.party().deathSave({2}, 5);
    heal.allies[0] = {2};
    check(s.submit(heal).reason == CommandReason::Resolved, "Cure Wounds accepts a downed living ally");
    const auto* rogue = s.party().character({2});
    check(rogue->life == rules::Life::Conscious && rogue->hitPoints >= 7 && rogue->failures == 0 &&
              rogue->successes == 0,
          "Life bonus and healing reset death saves");
    check(s.party().character({1})->mana == 2 && s.events().count == 2 &&
              s.events().values[0].event == CombatEvent::Cast &&
              s.events().values[1].event == CombatEvent::Healing,
          "Complete cast and healing event batch, exact mana cost");
    s.update(6, false);
    s.party().damage({3}, s.party().character({3})->hitPoints);
    auto spare = spellCommand(s, SpellKind::SpareTheDying);
    spare.targetCount = 1;
    spare.allies[0] = {3};
    check(s.submit(spare).reason == CommandReason::Resolved &&
              s.party().character({3})->life == rules::Life::Stable &&
              s.party().character({3})->hitPoints == 0,
          "Cantrip stabilizes without healing");
    check(s.party().character({1})->mana == 2, "Cantrip spends no mana");
}
void features() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    const auto places = spellPlacements(map, 0);
    GameSession s(map, catalog, places, 7, spellSpawn, false);
    s.party().damage({0}, 6);
    auto wind = featureCommand(s, 0, rules::Feature::SecondWind);
    check(s.submit(wind).reason == CommandReason::Resolved && !s.party().character({0})->secondWind,
          "Second Wind heals and spends charge");
    s.update(6, false);
    wind.turn = s.combat().snapshot().turn;
    check(s.submit(wind).reason == CommandReason::NoCharges, "No encounter-based Second Wind refill");
    auto recovery = featureCommand(s, 3, rules::Feature::ArcaneRecovery);
    check(s.submit(recovery).reason == CommandReason::FullMana, "Full mana recovery rejects");
    s.party().spendMana({3}, 3);
    check(s.submit(recovery).reason == CommandReason::Resolved && s.party().character({3})->mana == 3 &&
              !s.party().character({3})->arcaneRecovery,
          "Safe recovery restores exactly two and spends charge");
    s.update(6, false);
    recovery.turn = s.combat().snapshot().turn;
    check(s.submit(recovery).reason == CommandReason::NoCharges, "Recovery cannot repeat");
    const auto near = spellPlacements(map, 4, true);
    GameSession threatened(map, catalog, near, 7, spellSpawn, false);
    spellReady(threatened, 3);
    threatened.party().spendMana({3}, 2);
    check(threatened.submit(featureCommand(threatened, 3, rules::Feature::ArcaneRecovery)).reason ==
              CommandReason::Unsafe,
          "Recovery refuses active contact");
}
void effectsAndValidation() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    const auto places = spellPlacements(map);
    GameSession s(map, catalog, places, 11, spellSpawn, false);
    auto bless = spellCommand(s, SpellKind::Bless);
    bless.targetCount = 3;
    bless.allies = {{{0}, {1}, {2}}};
    check(s.submit(bless).reason == CommandReason::Resolved &&
              s.effects().has(rules::CharacterId{0}, EffectKind::Bless),
          "Bless applies selected recipients");
    check(s.effects().concentration({1}).serial != 0 && s.events().count == 4,
          "Concentration and all application events are published");
    s.update(6, false);
    auto faith = spellCommand(s, SpellKind::ShieldOfFaith);
    faith.targetCount = 1;
    faith.allies[0] = {0};
    check(s.submit(faith).reason == CommandReason::Resolved &&
              !s.effects().has(rules::CharacterId{0}, EffectKind::Bless) &&
              s.effects().armorClass(rules::CharacterId{0}) == s.party().character({0})->stats.armorClass + 2,
          "New concentration replaces old and modifies effective armor only");
    const auto end = featureCommand(s, 1, rules::Feature::DismissConcentration);
    check(s.submit(end).reason == CommandReason::Resolved && !s.effects().concentration({1}).serial,
          "Dismiss concentration is free during cooldown");
    s.update(6, false);
    auto bane = spellCommand(s, SpellKind::Bane);
    bane.targetCount = 1;
    bane.enemies[0] = s.actors().id(0);
    check(s.submit(bane).reason == CommandReason::InsufficientMana, "Mana exhaustion blocks leveled spell");
    auto armor = spellCommand(s, SpellKind::MageArmor);
    armor.targetCount = 1;
    armor.allies[0] = {0};
    check(s.submit(armor).reason == CommandReason::Armored, "Mage Armor rejects armored target");
    armor.allies[0] = {3};
    check(s.submit(armor).reason == CommandReason::Resolved &&
              s.effects().armorClass(rules::CharacterId{3}) == s.party().character({3})->stats.armorClass + 3,
          "Mage Armor applies to unarmored wizard");
    s.update(6, false);
    armor = spellCommand(s, SpellKind::MageArmor);
    armor.targetCount = 1;
    armor.allies[0] = {3};
    armor.character = {0};
    check(s.submit(armor).reason == CommandReason::InvalidSpell, "Class eligibility is authoritative");
}
void projectiles() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    const auto places = spellPlacements(map);
    GameSession s(map, catalog, places, 5, spellSpawn, false);
    auto missile = spellCommand(s, SpellKind::MagicMissile);
    missile.targetCount = 3;
    for (int i = 0; i < 3; ++i) {
        missile.enemies[i] = s.actors().id(i);
        missile.darts[i] = 1;
    }
    const auto started = s.submit(missile);
    check(started.event == CombatEvent::AttackStarted && s.party().character({3})->mana == 2,
          "Missiles cost once at acceptance");
    for (int i = 0; i < 3; ++i)
        check(s.actors().actor(s.actors().id(i))->hitPoints == 1000, "Missiles do not damage at acceptance");
    s.update(.341, false);
    check(s.actors().actor(s.actors().id(0))->hitPoints == 1000, "No damage before exact arrival");
    s.update(60, true);
    check(s.timeMs() == 341, "Pause freezes flight");
    s.update(.001, false);
    int impacts = 0;
    for (const auto& e : s.events())
        if (e.event == CombatEvent::Attack)
            ++impacts;
    check(impacts == 3, "All three recipients resolve in one arrival batch");
    const int damage = 1000 - s.actors().actor(s.actors().id(0))->hitPoints;
    check(damage >= 2 && damage <= 5, "Missile damage in range");
    for (int i = 1; i < 3; ++i)
        check(s.actors().actor(s.actors().id(i))->hitPoints == 1000 - damage,
              "One shared dart roll applies to each allocation");
    s.update(.1, false);
    for (const auto& e : s.events())
        check(e.event != CombatEvent::Attack, "Impacts never replay");
    GameSession dead(map, catalog, places, 5, spellSpawn, false);
    missile = spellCommand(dead, SpellKind::MagicMissile);
    missile.targetCount = 1;
    missile.enemies[0] = dead.actors().id(0);
    missile.darts[0] = 3;
    dead.submit(missile);
    dead.party().damage({3}, 1000);
    dead.actors().remove(dead.actors().id(0));
    const auto& batch = dead.update(.342, false);
    bool gone = false;
    for (const auto& e : batch)
        gone |= e.event == CombatEvent::Attack && e.targetGone && !e.damage;
    check(gone, "Released missiles survive caster death and never retarget a removed recipient");
}
void impactAlerts() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    NpcWorld places(map, spellSpawn.cell, {"goblin"},
                    {{{0}, {6, 5}, {0, 0}, Heading::North},
                     {{0}, {9, 4}, {0, 0}, Heading::North},
                     {{0}, {5, 5}, {0, 0}, Heading::North}});
    GameSession s(map, catalog, places, 5, spellSpawn, false);
    auto c = spellCommand(s, SpellKind::MagicMissile);
    c.targetCount = 2;
    c.enemies[0] = s.actors().id(0);
    c.enemies[1] = s.actors().id(1);
    c.darts[0] = 2;
    c.darts[1] = 1;
    check(s.submit(c).event == CombatEvent::AttackStarted, "Staggered missile fixture accepted");
    check(s.combat().snapshot().phase == CombatPhase::Exploration &&
              s.ai().actors[0].awareness == Awareness::Routine,
          "Casting alone never starts pursuit");
    const auto first = arrowFlightMs(spellSpawn.cell, {6, 5});
    const auto last = arrowFlightMs(spellSpawn.cell, {9, 4});
    s.update(double(first - 1) / 1000, false);
    s.update(60, true);
    check(s.ai().actors[0].awareness == Awareness::Routine &&
              s.ai().actors[1].awareness == Awareness::Routine,
          "Flight and pause preserve unawareness");
    s.update(.001, false);
    check(s.ai().actors[0].awareness == Awareness::Pursuing &&
              s.ai().actors[2].awareness == Awareness::Pursuing &&
              s.ai().actors[1].awareness == Awareness::Routine,
          "First missile alerts only its local group");
    s.update(double(last - first - 1) / 1000, false);
    check(s.ai().actors[1].awareness == Awareness::Routine, "Distant second recipient remains unaware");
    s.update(.001, false);
    check(s.ai().actors[1].awareness == Awareness::Pursuing, "Second recipient alerts at its own arrival");
    GameSession coarse(map, catalog, places, 5, spellSpawn, false);
    c.world = coarse.combat().snapshot().world;
    c.turn = coarse.combat().snapshot().turn;
    c.enemies[0] = coarse.actors().id(0);
    c.enemies[1] = coarse.actors().id(1);
    coarse.submit(c);
    coarse.update(1, false);
    for (unsigned i = 0; i < 3; ++i)
        check(coarse.ai().actors[i].awareness == s.ai().actors[i].awareness &&
                  coarse.actors().snapshot().actors[i].hitPoints == s.actors().snapshot().actors[i].hitPoints,
              "Coarse update preserves missile damage and alert results");
    const auto frail = spellCatalog(1);
    GameSession lethal(map, frail, places, 5, spellSpawn, false);
    c = spellCommand(lethal, SpellKind::MagicMissile);
    c.targetCount = 1;
    c.enemies[0] = lethal.actors().id(0);
    c.darts[0] = 3;
    lethal.submit(c);
    lethal.update(double(first) / 1000, false);
    check(lethal.actors().actor(c.enemies[0])->life == ActorLife::Dead &&
              lethal.ai().actors[2].awareness == Awareness::Pursuing,
          "Lethal impact still alerts nearby survivors");
    GameSession removed(map, catalog, places, 5, spellSpawn, false);
    c = spellCommand(removed, SpellKind::MagicMissile);
    c.targetCount = 1;
    c.enemies[0] = removed.actors().id(0);
    c.darts[0] = 3;
    removed.submit(c);
    removed.actors().remove(c.enemies[0]);
    removed.update(1, false);
    check(removed.ai().actors[2].awareness == Awareness::Routine, "Missing recipient causes no local alert");
    NpcWorld sees(map, spellSpawn.cell, {"goblin"}, {{{0}, {6, 5}, {0, 0}, Heading::South}});
    GameSession sight(map, catalog, sees, 5, spellSpawn, true);
    c = spellCommand(sight, SpellKind::MagicMissile);
    c.targetCount = 1;
    c.enemies[0] = sight.actors().id(0);
    c.darts[0] = 3;
    sight.submit(c);
    sight.update(.1, false);
    check(sight.ai().actors[0].awareness == Awareness::Pursuing &&
              sight.actors().actor(c.enemies[0])->hitPoints == 1000,
          "Normal sight can reveal the party before a projectile arrives");
}
void meleeAlertTiming() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    NpcWorld places(map, spellSpawn.cell, {"goblin"},
                    {{{0}, {6, 7}, {0, 1}, Heading::North}, {{0}, {6, 4}, {0, 0}, Heading::North}});
    for (bool spell : {false, true})
        for (bool cancel : {false, true}) {
            GameSession s(map, catalog, places, 5, spellSpawn, false);
            spellReady(s, spell ? 1 : 0);
            auto command =
                spell ? spellCommand(s, SpellKind::InflictWounds) : turnCommand(s, PartyAction::Melee);
            command.target = s.actors().id(0);
            command.targetCount = 1;
            command.enemies[0] = s.actors().id(0);
            check(s.submit(command).event == CombatEvent::AttackStarted, "Delayed strike accepted");
            s.update(.199, false);
            check(s.ai().actors[1].awareness == Awareness::Routine,
                  "Melee windup does not reveal attack to nearby group");
            if (cancel)
                s.party().damage(command.character, 1000);
            s.update(.001, false);
            check(s.ai().actors[1].awareness == (cancel ? Awareness::Routine : Awareness::Pursuing),
                  "Melee weapon/spell alerts at strike; canceled windup does not alert");
        }
}
void allDamageSpells() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    for (const auto kind :
         {SpellKind::SacredFlame, SpellKind::AcidSplash, SpellKind::FireBolt, SpellKind::GuidingBolt,
          SpellKind::InflictWounds, SpellKind::BurningHands, SpellKind::Thunderwave, SpellKind::Bane}) {
        const bool close = kind == SpellKind::InflictWounds || kind == SpellKind::BurningHands ||
                           kind == SpellKind::Thunderwave;
        const auto places = spellPlacements(map, close ? 12 : 3, close);
        GameSession s(map, catalog, places, 5, spellSpawn, false);
        auto c = spellCommand(s, kind);
        if (close) {
            spellReady(s, c.character.index);
            c = spellCommand(s, kind);
        }
        if (!rules::spellRules(kind)->area) {
            c.targetCount = kind == SpellKind::AcidSplash ? 2 : 1;
            for (std::size_t i = 0; i < c.targetCount; ++i)
                c.enemies[i] = s.actors().id(kind == SpellKind::InflictWounds ? 2 : i);
        }
        check(s.submit(c).reason == CommandReason::Resolved,
              "Every curated hostile spell accepts a legal request");
        if (kind == SpellKind::FireBolt || kind == SpellKind::GuidingBolt) {
            check(s.ai().actors[0].awareness == Awareness::Routine, "Projectile spell cast does not alert");
            s.update(.341, false);
            check(s.ai().actors[0].awareness == Awareness::Routine, "Projectile spell flight does not alert");
            s.update(.001, false);
            check(s.ai().actors[0].awareness == Awareness::Pursuing, "Projectile spell arrival alerts");
        } else if (kind == SpellKind::InflictWounds)
            s.update(.5, false);
        bool result = false;
        for (const auto& e : s.events())
            result |= e.event == CombatEvent::Attack || e.event == CombatEvent::Effect;
        check(result, "Each hostile spell publishes authoritative results");
        if (kind == SpellKind::BurningHands || kind == SpellKind::Thunderwave) {
            int hits = 0;
            for (const auto& e : s.events())
                if (e.event == CombatEvent::Attack)
                    ++hits;
            check(hits == 12, "Area includes all twelve occupants including protected rear slots");
            for (unsigned i = 0; i < 4; ++i)
                check(s.party().character({static_cast<std::uint16_t>(i)})->hitPoints ==
                          s.party().character({static_cast<std::uint16_t>(i)})->stats.maxHitPoints,
                      "Area excludes party origin");
        }
    }
}
void invalidAndPool() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    const auto places = spellPlacements(map);
    GameSession a(map, catalog, places, 19, spellSpawn, false),
        b(map, catalog, places, 19, spellSpawn, false);
    auto c = spellCommand(a, SpellKind::MagicMissile);
    c.targetCount = 2;
    c.enemies[0] = a.actors().id(0);
    c.enemies[1] = c.enemies[0];
    c.darts[0] = 2;
    c.darts[1] = 1;
    check(a.submit(c).reason == CommandReason::InvalidTargets, "Duplicate recipients rejected");
    c.enemies[1] = a.actors().id(1);
    c.darts[1] = 2;
    check(a.submit(c).reason == CommandReason::InvalidTargets, "Exactly three darts required");
    c.targetCount = 1;
    c.darts[0] = 3;
    a.submit(c);
    c = spellCommand(b, SpellKind::MagicMissile);
    c.targetCount = 1;
    c.darts[0] = 3;
    c.enemies[0] = b.actors().id(0);
    b.submit(c);
    a.update(.4, false);
    b.update(.4, false);
    check(a.actors().actor(a.actors().id(0))->hitPoints == b.actors().actor(b.actors().id(0))->hitPoints,
          "Invalid commands preserve RNG stream");
    GameSession full(map, catalog, places, 19, spellSpawn, false);
    EffectRequest request;
    request.world = full.actors().snapshot().world;
    request.caster = {3};
    request.kind = EffectKind::MageArmor;
    request.targets[0] = rules::CharacterId{3};
    request.count = 1;
    for (std::size_t i = 0; i < EffectController::capacity; ++i)
        check(bool(full.applyEffect(request)), "Fill effect pool");
    auto armor = spellCommand(full, SpellKind::MageArmor);
    armor.targetCount = 1;
    armor.allies[0] = {3};
    check(full.submit(armor).reason == CommandReason::EffectCapacity &&
              full.party().character({3})->mana == 4,
          "Effect pool rejects before mana/action consumption");
}
void ui() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    const auto places = spellPlacements(map);
    GameSession s(map, catalog, places, 5, spellSpawn, false);
    CommandController ui(s);
    ui.refresh(allCombatTargets(s), false, false, true);
    ui.selectCharacter(3);
    UiInput open;
    open.pressed[static_cast<std::size_t>(UiKey::Four)] = true;
    routeHudInput(ui, open);
    check(ui.snapshot().magic.entries[13].charges == 1, "Spellbook reports unused recovery charge");
    check(ui.snapshot().modal() && ui.snapshot().magic.count == 14,
          "Wizard book contains thirteen spells and recovery");
    const auto time = s.timeMs();
    s.update(60, ui.snapshot().modal());
    check(s.timeMs() == time, "Book pauses simulation");
    ui.selectSpellbookTab(1);
    UiInput choose;
    choose.pointer = ArtPoint{hudLayout::spellEntry(1).x + 2, hudLayout::spellEntry(1).y + 2};
    choose.pressed[static_cast<std::size_t>(UiKey::Click)] = true;
    routeHudInput(ui, choose, s.actors().id(0)); // Modal click must not also target/cast.
    check(ui.snapshot().magic.pending.targetCount == 0, "Picker closing click does not select a target");
    check(!ui.snapshot().modal() && ui.snapshot().magic.missiles && s.party().character({3})->mana == 4,
          "Selection arms without casting or spending");
    ui.magicEnemy(s.actors().id(0));
    ui.magicEnemy(s.actors().id(1));
    ui.magicEnemy(s.actors().id(2));
    check(ui.snapshot().magic.pending.targetCount == 3 && ui.snapshot().magic.pending.darts[0] == 1,
          "Darts can be distributed across three bodies");
    check(ui.confirm() == CommandReason::Resolved && ui.snapshot().selection.action == PartyAction::Melee,
          "UI commits then clears targeting");
    ui.refresh(allCombatTargets(s), false, false, true);
    s.update(2, false);
    ui.refresh(allCombatTargets(s), false, false, true);
    ui.selectCharacter(1);
    ui.openSpellbook();
    ui.chooseSpellbook(2); // Bless
    ui.magicAlly(0);
    ui.magicAlly(1);
    ui.magicAlly(2);
    check(ui.snapshot().magic.pending.targetCount == 3, "Portrait targeting supports three allies");
    ui.refresh(allCombatTargets(s), false, false, false);
    check(ui.snapshot().magic.pending.targetCount == 3 && ui.snapshot().magic.armed,
          "Focus loss preserves spell selections");
}
void classFeaturesUi() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    const auto places = spellPlacements(map, 0);
    GameSession s(map, catalog, places, 5, spellSpawn, false);
    CommandController ui(s);
    const auto refresh = [&] { ui.refresh(allCombatTargets(s), false, false, true); };
    const auto key = [&](UiKey k) {
        UiInput input;
        input.pressed[static_cast<std::size_t>(k)] = true;
        routeHudInput(ui, input);
    };
    refresh();
    ui.selectCharacter(2);
    ui.hover(HoverKind::Action, 3);
    check(!std::strcmp(ui.snapshot().hover.lines[0].data(), "Class Features [4]"),
          "Non-caster action tooltip names Class Features");
    key(UiKey::Four);
    check(ui.snapshot().magic.open && !ui.snapshot().magic.spellcaster && ui.snapshot().magic.count == 2 &&
              ui.snapshot().magic.entries[0].passive == PassiveFeature::SneakAttack &&
              ui.snapshot().magic.entries[1].passive == PassiveFeature::RearRowMelee,
          "Rogue has two browsable automatic features");
    const auto turn = s.combat().snapshot().turn;
    const auto time = s.timeMs();
    const auto resources = s.party().snapshot();
    for (int i = 0; i < 8; ++i) {
        key(i % 2 ? UiKey::Up : UiKey::Down);
        key(UiKey::Confirm);
        UiInput click;
        const auto r = hudLayout::spellEntry(i % 2);
        click.pointer = ArtPoint{float(r.x + 2), float(r.y + 2)};
        click.pressed[static_cast<std::size_t>(UiKey::Click)] = true;
        routeHudInput(ui, click);
        s.update(1, ui.snapshot().modal());
        refresh();
        check(ui.snapshot().magic.open && !ui.snapshot().magic.armed &&
                  ui.snapshot().magic.cursor == std::size_t(i % 2) && s.combat().snapshot().turn == turn &&
                  s.timeMs() == time,
              "Passive mouse/Enter browsing stays paused and never commits an action");
    }
    for (unsigned i = 0; i < rules::partySize; ++i) {
        const auto* c = s.party().character({static_cast<std::uint16_t>(i)});
        check(c->mana == resources.characters[i].mana &&
                  c->secondWind == resources.characters[i].secondWind &&
                  c->hitPoints == resources.characters[i].hitPoints,
              "Passive browsing spends no resources or health");
    }
    key(UiKey::Cancel);
    ui.selectCharacter(0);
    ui.openSpellbook();
    check(ui.snapshot().magic.count == 2 &&
              ui.snapshot().magic.entries[0].category == PickerEntryKind::ActivatedFeature &&
              ui.snapshot().magic.entries[0].charges == 1 &&
              ui.snapshot().magic.entries[0].reason == CommandReason::FullHealth &&
              ui.snapshot().magic.entries[1].passive == PassiveFeature::Defense,
          "Fighter lists checked Second Wind and selected automatic Defense");
    ui.chooseSpellbook(0);
    check(ui.snapshot().magic.open, "Full-health Second Wind cannot leave picker");
    key(UiKey::Cancel);
    s.party().damage({0}, 6);
    refresh();
    ui.openSpellbook();
    ui.chooseSpellbook(0);
    check(ui.snapshot().magic.armed && ui.snapshot().magic.charges == 1 &&
              ui.snapshot().magic.pending.action == PartyAction::Feature,
          "Confirmation dock retains explicit feature identity and remaining uses");
    ui.hover(HoverKind::Portrait, 1);
    check(!std::strcmp(ui.snapshot().hover.lines[1].data(), "This feature needs no recipient selection."),
          "Feature confirmation does not call the ability a spell in portrait hints");
    key(UiKey::Cancel);
    check(s.party().character({0})->secondWind && s.combat().snapshot().turn == turn,
          "Canceling Second Wind spends neither use nor action");
    ui.openSpellbook();
    ui.chooseSpellbook(0);
    check(ui.confirm() == CommandReason::Resolved && !s.party().character({0})->secondWind &&
              s.party().character({0})->mana == 0 && s.combat().snapshot().cooldownMs[0] == combatRoundMs,
          "Second Wind spends one use and normal readiness, never mana");
    s.update(6, false);
    refresh();
    ui.openSpellbook();
    check(ui.snapshot().magic.entries[0].charges == 0 &&
              ui.snapshot().magic.entries[0].reason == CommandReason::NoCharges,
          "Exhausted feature exposes zero uses and rejection reason");
    key(UiKey::Cancel);
    s.party().spendMana({3}, 4);
    refresh();
    ui.selectCharacter(3);
    ui.hover(HoverKind::Action, 3);
    check(!std::strcmp(ui.snapshot().hover.lines[0].data(), "Spells [4]"),
          "An exhausted caster still has the Spells tooltip");
    ui.openSpellbook();
    check(ui.snapshot().magic.spellcaster && ui.snapshot().magic.mana == 0,
          "Casting capability, not remaining mana, identifies a spellbook");

    // A legacy class with no active or passive features has a safe empty picker.
    const auto legacy = partyFixture().catalog();
    NpcWorld empty(map, spellSpawn.cell, {}, {});
    GameSession noFeatures(map, legacy, empty, 5, spellSpawn, false);
    CommandController emptyUi(noFeatures);
    emptyUi.refresh({}, false, false, true);
    emptyUi.selectCharacter(2);
    emptyUi.openSpellbook();
    emptyUi.chooseSpellbook(0);
    emptyUi.scrollSpellbook(100);
    check(emptyUi.snapshot().magic.open && !emptyUi.snapshot().magic.count &&
              !emptyUi.snapshot().magic.cursor && !emptyUi.snapshot().magic.offset,
          "Empty picker stays bounded and cannot activate a nonexistent feature");

    // Largest current spell family plus two activated features exercises mixed-list paging.
    auto mixedFixture = partyFixture();
    auto& cls = mixedFixture.classes[1];
    cls.casting = rules::Casting::Wisdom;
    cls.features = static_cast<rules::ClassFeatures>(rules::ClassFeature::SecondWind);
    std::vector<rules::SpellDefinition> definitions;
    for (unsigned i = 0; i < static_cast<unsigned>(SpellKind::Count); ++i) {
        const auto kind = static_cast<SpellKind>(i);
        const auto& rule = *rules::spellRules(kind);
        if (rule.casting != cls.casting)
            continue;
        cls.spells[cls.spellCount++] = {static_cast<std::uint16_t>(definitions.size())};
        definitions.push_back({rule.id, rule.name, rule.description, kind});
    }
    const rules::Catalog mixedCatalog({}, {}, mixedFixture.races, mixedFixture.classes, mixedFixture.items,
                                      mixedFixture.preset, definitions);
    GameSession mixed(map, mixedCatalog, empty, 5, spellSpawn, false);
    mixed.party().damage({1}, 2);
    EffectRequest blessing;
    blessing.world = mixed.actors().snapshot().world;
    blessing.caster = {1};
    blessing.kind = EffectKind::Bless;
    blessing.targets[0] = rules::CharacterId{0};
    blessing.count = 1;
    check(bool(mixed.applyEffect(blessing)), "Mixed-list fixture has dismissible concentration");
    CommandController mixedUi(mixed);
    mixedUi.refresh({}, false, false, true);
    mixedUi.selectCharacter(1);
    mixedUi.openSpellbook();
    mixedUi.selectSpellbookTab(MagicHud::abilityTab);
    mixedUi.scrollSpellbook(100);
    const auto mixedCount = mixedUi.snapshot().magic.count;
    check(mixedCount == definitions.size() + 2 && mixedCount <= MagicHud::capacity &&
              mixedUi.snapshot().magic.offset == 0 && mixedUi.snapshot().magic.visibleCount == 2 &&
              mixedUi.snapshot().magic.entries[mixedCount - 1].feature ==
                  rules::Feature::DismissConcentration,
          "Abilities are separately filtered within bounded catalog storage");
    mixedUi.chooseSpellbook(mixedCount - 2);
    check(mixedUi.snapshot().magic.armed &&
              mixedUi.snapshot().magic.pending.feature == rules::Feature::SecondWind,
          "Scrolled activated feature creates the correct command");
    mixedUi.backToSpellbook();
    check(mixedUi.snapshot().magic.open && mixedUi.snapshot().magic.cursor == mixedCount - 2 &&
              mixedUi.snapshot().magic.offset == 0 && mixedUi.snapshot().magic.visibleCount == 2 &&
              mixed.party().character({1})->secondWind,
          "Back restores the ability tab and cursor without spending the feature");

    const auto nearby = spellPlacements(map, 1, true);
    GameSession combat(map, catalog, nearby, 5, spellSpawn, false);
    combat.party().damage({0}, 6);
    spellReady(combat, 0);
    CommandController combatUi(combat);
    combatUi.refresh(allCombatTargets(combat), false, false, true);
    combatUi.openSpellbook();
    combatUi.chooseSpellbook(0);
    const auto committed = combatUi.snapshot().magic.pending;
    check(combatUi.confirm() == CommandReason::Resolved && combat.combat().snapshot().spent[0] &&
              !combat.party().character({0})->secondWind && combat.party().character({0})->mana == 0,
          "Second Wind in initiative spends the regular action and one charge");
    check(combat.submit(committed).reason != CommandReason::Resolved,
          "Repeated Second Wind confirmation cannot commit twice");
}
void uiRevision() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    const auto places = spellPlacements(map);
    GameSession s(map, catalog, places, 5, spellSpawn, false);
    CommandController ui(s);
    auto refresh = [&](bool focus = true) { ui.refresh(allCombatTargets(s), false, false, focus); };
    auto key = [&](UiKey k) {
        UiInput input;
        input.pressed[static_cast<std::size_t>(k)] = true;
        routeHudInput(ui, input);
    };
    auto click = [&](UiRect r, std::optional<ActorId> id = {}, bool right = false, bool shift = false) {
        UiInput input;
        input.pointer = ArtPoint{float(r.x + 2), float(r.y + 2)};
        input.shift = shift;
        input.pressed[static_cast<std::size_t>(right ? UiKey::RightClick : UiKey::Click)] = true;
        routeHudInput(ui, input, id);
    };
    refresh();
    ui.selectCharacter(3);
    ui.openSpellbook();
    click(hudLayout::viewport, s.actors().id(0));
    check(!ui.snapshot().modal() && !ui.snapshot().selection.target && s.party().character({3})->mana == 4,
          "Outside picker click dismisses without underlying attack");
    ui.openSpellbook();
    ui.chooseSpellbook(13);
    check(ui.snapshot().magic.open && ui.snapshot().magic.entries[13].reason == CommandReason::FullMana,
          "Unavailable ability stays in picker with a specific reason");
    ui.chooseSpellbook(3);
    const auto a = s.actors().id(0), b = s.actors().id(1), c = s.actors().id(2);
    click(hudLayout::viewport, a);
    click(hudLayout::viewport, a);
    check(ui.snapshot().magic.assigned == 2 && ui.snapshot().magic.pending.darts[0] == 2,
          "Each missile click adds exactly one dart");
    click(hudLayout::viewport, a, true);
    check(ui.snapshot().magic.assigned == 1 && ui.snapshot().magic.armed,
          "Right click removes instead of cancelling");
    click(hudLayout::viewport, a, false, true);
    check(ui.snapshot().magic.pending.targetCount == 0, "Shift click removes final dart and recipient");
    click(hudLayout::viewport, a);
    click(hudLayout::viewport, b);
    click(hudLayout::viewport, c);
    click(hudLayout::viewport, a);
    check(ui.snapshot().magic.pending.darts[0] == 1 && ui.snapshot().magic.assigned == 3 &&
              std::strstr(ui.snapshot().magic.feedback.data(), "Remove a dart"),
          "Full allocation never redistributes");
    key(UiKey::One);
    check(ui.snapshot().magic.armed && s.party().character({3})->mana == 4,
          "Casting consumes ordinary action shortcuts");
    const auto logCount = ui.snapshot().log.size();
    refresh(false);
    UiInput blur;
    blur.focused = false;
    routeHudInput(ui, blur);
    check(ui.snapshot().magic.assigned == 3 && ui.snapshot().suspended &&
              ui.confirm() == CommandReason::Unfocused,
          "Blur preserves allocation and prevents casts");
    refresh();
    check(ui.snapshot().magic.assigned == 3 && ui.snapshot().log.size() <= logCount + 1,
          "Refocus does not emit repeated cancellation logs");
    ui.toggleLog();
    check(ui.snapshot().magic.armed && ui.snapshot().modal(), "History preserves pending cast");
    ui.closeOverlay();
    ui.inspect(2);
    check(ui.snapshot().inventory.page == CharacterPage::Character && ui.snapshot().modal() &&
              ui.snapshot().magic.armed && ui.snapshot().magic.assigned == 3,
          "Character inspection pauses and preserves the pending spell and dart assignment");
    ui.closeOverlay();
    check(!ui.snapshot().modal() && ui.snapshot().magic.armed, "Closing Character resumes the pending spell");
    const auto original = s.actors().actor(b)->npc;
    for (int y = original.cell.y - 1; y >= 1; --y)
        check(s.actors().move(b, {6, y}, original.subcell, spellSpawn.cell).changed,
              "Move selected enemy beyond six cells");
    refresh();
    check(ui.snapshot().magic.assigned == 2, "Range loss returns only that enemy's dart");
    for (int y = 2; y <= original.cell.y; ++y)
        check(s.actors().move(b, {6, y}, original.subcell, spellSpawn.cell).changed,
              "Restore recipient fixture");
    refresh();
    ui.magicEnemy(b);
    auto hidden = allCombatTargets(s);
    for (std::size_t i = 0; i < hidden.count; ++i)
        if (sameActor(hidden.ids[i], b)) {
            for (std::size_t j = i + 1; j < hidden.count; ++j)
                hidden.ids[j - 1] = hidden.ids[j];
            --hidden.count;
            break;
        }
    ui.refresh(hidden, false, false, true);
    check(ui.snapshot().magic.assigned == 2, "Loss of visible body removes only that assignment");
    refresh();
    ui.magicEnemy(b);
    s.actors().damage(b, 1000, s.timeMs());
    refresh();
    check(ui.snapshot().magic.assigned == 2 && ui.snapshot().magic.pending.targetCount == 2,
          "Dead target alone is removed and its dart becomes unassigned");
    click(hudLayout::viewport, a);
    key(UiKey::Back);
    check(ui.snapshot().magic.open && ui.snapshot().magic.cursor == 3 &&
              ui.snapshot().magic.pending.targetCount == 0,
          "Back remembers spell and clears assignments without costs");
    ui.chooseSpellbook(3);
    click(hudLayout::viewport, a);
    click(hudLayout::viewport, a);
    click(hudLayout::viewport, a);
    UiInput commit;
    commit.pointer = ArtPoint{float(hudLayout::spellCast.x + 2), float(hudLayout::spellCast.y + 2)};
    commit.pressed[static_cast<std::size_t>(UiKey::Click)] = true;
    commit.pressed[static_cast<std::size_t>(UiKey::Confirm)] = true;
    routeHudInput(ui, commit);
    check(s.party().character({3})->mana == 2 && ui.snapshot().magic.state == CastingState::Resolving,
          "Simultaneous button and Enter commit once and enter resolving");
    routeHudInput(ui, commit);
    check(s.party().character({3})->mana == 2, "Resolving rejects replay");
    s.update(2, false);
    refresh();
    check(ui.snapshot().magic.state == CastingState::Normal, "Existing cast hold ends resolving");
    ui.selectCharacter(1);
    ui.openSpellbook();
    ui.browseSpellbook(7);
    const auto offset = ui.snapshot().magic.offset;
    check(offset == 0, "Cantrip list fits without scrolling");
    ui.chooseSpellbook(2);
    click(hudLayout::card(0));
    click(hudLayout::card(1));
    click(hudLayout::card(2));
    refresh();
    check(ui.snapshot().cards[1].castCaster && ui.snapshot().cards[1].castRecipient,
          "Caster and recipient markers coexist on self-selection");
    click(hudLayout::card(3));
    check(ui.snapshot().magic.pending.targetCount == 3 &&
              std::strstr(ui.snapshot().magic.feedback.data(), "Remove a target"),
          "Ally maximum neither casts nor replaces anyone");
    click(hudLayout::recipientChip(1));
    check(ui.snapshot().magic.pending.targetCount == 2 && ui.snapshot().availability == CommandReason::Ready,
          "Recipient chip removal allows fewer legal recipients");
    const auto mana = s.party().character({1})->mana;
    key(UiKey::Cancel);
    check(s.party().character({1})->mana == mana && !s.effects().concentration({1}).serial,
          "Cancellation leaves mana and concentration untouched");
    ui.openSpellbook();
    ui.browseSpellbook(7);
    ui.chooseSpellbook(7);
    const auto returnedOffset = ui.snapshot().magic.offset;
    key(UiKey::Back);
    check(ui.snapshot().magic.offset == returnedOffset && ui.snapshot().magic.cursor == 7,
          "Back restores scrolled list position");
    ui.closeOverlay();
    s.update(6, false);
    refresh();
    ui.selectCharacter(3);
    ui.openSpellbook();
    ui.chooseSpellbook(2);
    refresh();
    check(ui.snapshot().magic.allyReasons[0] == CommandReason::Armored &&
              ui.snapshot().magic.allyReasons[3] == CommandReason::Ready,
          "Portrait eligibility matches spell restrictions");
    click(hudLayout::card(0));
    check(s.party().character({3})->mana == 2 && ui.snapshot().magic.armed,
          "Invalid single target never commits");
    click(hudLayout::card(3));
    check(s.party().character({3})->mana == 0 &&
              s.effects().has(rules::CharacterId{3}, EffectKind::MageArmor),
          "Valid single portrait click commits immediately");
}
void markerLayoutAndLog() {
    HudSnapshot h;
    h.magic.missiles = true;
    h.magic.affectedCount = 3;
    h.targets.world = 55;
    for (unsigned i = 0; i < 3; ++i) {
        h.magic.affected[i] = {static_cast<std::uint16_t>(i), 55};
        h.targets.anchors[i] = {250, 100, true, 90, 250, 90};
    }
    const auto layout = spellMarkerLayout(h);
    for (std::size_t i = 0; i < 3; ++i) {
        check(layout[i].has_value(), "Every overlapping visible recipient retains a badge");
        for (std::size_t j = 0; j < i; ++j) {
            const auto a = *layout[i], b = *layout[j];
            check(a.y + a.height <= b.y || b.y + b.height <= a.y || a.x + a.width <= b.x ||
                      b.x + b.width <= a.x,
                  "Dart badges never overlap at shared screen anchors");
        }
    }
    h.targets.anchors[1].valid = false;
    check(!spellMarkerLayout(h)[1], "Hidden bodies do not receive revealing badges");
    CombatLog log;
    log.append("First line: a long message about a previous round and the actions taken there, preserved "
               "while browsing.");
    log.append(
        "Second line: another long message that wraps differently when the casting dock takes more width.");
    log.append("Newest line.");
    char original[CombatLog::messageBytes];
    log.line(log.lineCount(hudLayout::logColumns) - 3, hudLayout::logColumns, original, sizeof(original));
    const auto* lastWord = std::strrchr(original, ' ');
    lastWord = lastWord ? lastWord + 1 : original;
    const auto offset = log.rewrapOffset(2, hudLayout::logColumns, hudLayout::castingLogColumns);
    char line[CombatLog::messageBytes];
    log.line(log.lineCount(hudLayout::castingLogColumns) - offset - 1, hudLayout::castingLogColumns, line,
             sizeof(line));
    check(*lastWord && std::strstr(line, lastWord), "Dock rewrap retains the logical message position");
}
void sharedArea() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    for (int h = 0; h < 4; ++h) {
        const Player pose{spellSpawn.cell, static_cast<Heading>(h)};
        std::vector<NpcInstance> instances;
        for (int i = 0; i < 12; ++i)
            instances.push_back({{0},
                                 pose.cell + forward(pose.heading) + right(pose.heading) * (i / 4 - 1),
                                 {i % 2, (i / 2) % 2},
                                 Heading::North});
        NpcWorld places(map, pose.cell, {"goblin"}, instances);
        GameSession s(map, catalog, places, 5, pose, false);
        spellReady(s, 3);
        const auto preview = spellArea(s.actors(), s.movement().player);
        check(preview.count == 12,
              "Shared area includes protected and body-occluded occupants for every heading");
        auto command = spellCommand(s, SpellKind::BurningHands);
        check(s.submit(command).reason == CommandReason::Resolved, "Area preview validates for commit");
        std::size_t impacts = 0;
        for (const auto& e : s.events())
            if (e.event == CombatEvent::Attack) {
                check(impacts < preview.count && sameActor(e.enemy, preview.actors[impacts]),
                      "Preview and resolution recipients agree in stable order");
                ++impacts;
            }
        check(impacts == preview.count, "Every previewed occupant receives a result");
    }
}
void guidingAndSneak() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    const auto places = spellPlacements(map);
    GameSession s(map, catalog, places, 5, spellSpawn, false);
    auto bolt = spellCommand(s, SpellKind::GuidingBolt);
    bolt.targetCount = 1;
    bolt.enemies[0] = s.actors().id(0);
    check(s.submit(bolt).event == CombatEvent::AttackStarted && !s.effects().guiding(bolt.enemies[0]),
          "Guiding marker is absent before impact");
    s.update(.342, false);
    check(s.effects().guiding(bolt.enemies[0]), "Surviving Guiding Bolt hit installs marker at impact");
    auto fire = spellCommand(s, SpellKind::FireBolt);
    fire.targetCount = 1;
    fire.enemies[0] = s.actors().id(0);
    check(s.submit(fire).event == CombatEvent::AttackStarted && !s.effects().guiding(fire.enemies[0]),
          "Next spell attack consumes marker at acceptance");
    GameSession expiring(map, catalog, places, 5, spellSpawn, false);
    bolt = spellCommand(expiring, SpellKind::GuidingBolt);
    bolt.targetCount = 1;
    bolt.enemies[0] = expiring.actors().id(0);
    expiring.submit(bolt);
    expiring.update(.342, false);
    expiring.update(5.999, false);
    check(expiring.effects().guiding(bolt.enemies[0]), "Marker survives until its exact six-second deadline");
    expiring.update(.001, false);
    check(!expiring.effects().guiding(bolt.enemies[0]), "Marker expires at deadline");
    const auto close = spellPlacements(map, 12, true);
    GameSession melee(map, catalog, close, 5, spellSpawn, false);
    spellReady(melee, 2);
    auto c = turnCommand(melee, PartyAction::Melee);
    c.target = melee.actors().id(2);
    check(melee.combat().validate(c, spellSpawn) == CommandReason::Ready &&
              !melee.combat().exposed(rules::CharacterId{2}),
          "Rogue may attack from a protected rear slot without moving");
    check(melee.combat().sneakPreview({2}, *c.target, false, spellSpawn),
          "Mobile front ally enables melee Sneak Attack");
    melee.submit(c);
    melee.update(.2, false);
    bool checked = false;
    for (const auto& e : melee.events())
        if (e.event == CombatEvent::Attack && !e.enemyAttack && e.character.index == 2) {
            check(e.hit && e.damage >= 5 && e.damage <= 15,
                  "Rogue strike includes weapon and Sneak Attack dice");
            checked = true;
        }
    check(checked && !melee.combat().sneakPreview({2}, *c.target, false, spellSpawn),
          "Accepted hit spends the Sneak Attack allowance");
    check(melee.party().snapshot().formation[2].index == 2, "Rear attack preserves formation identity");
}
void geometryAndPush() {
    auto rows = std::vector<std::string>(13, std::string(13, '.'));
    rows.front() = rows.back() = std::string(13, '#');
    for (auto& row : rows)
        row.front() = row.back() = '#';
    rows[6][6] = '#';
    const Map map(rows);
    const auto catalog = spellCatalog(1000, 1, 1);
    auto base = spellPlacements(map, 12, true);
    auto instances = base.instances();
    for (int i = 0; i < 4; ++i)
        instances.push_back({{0}, {5, 6}, {i % 2, i / 2}, Heading::South});
    NpcWorld places(map, spellSpawn.cell, {"goblin"}, instances);
    GameSession s(map, catalog, places, 5, spellSpawn, false);
    spellReady(s, 3);
    auto c = spellCommand(s, SpellKind::Thunderwave);
    check(s.submit(c).reason == CommandReason::Resolved, "Area command accepts without explicit recipients");
    int pushed = 0;
    for (int i = 0; i < 12; ++i) {
        const auto* a = s.actors().actor(s.actors().id(i));
        if (i < 8)
            check(a->npc.cell.y == 7, "Wall and full destination block pushes without chain movement");
        else
            pushed += a->npc.cell.y == 6;
        check(a->hitPoints < 1000, "Blocked displacement still receives spell damage");
    }
    check(pushed > 0, "Failed-save survivors enter vacant destination slots");
    for (int i = 12; i < 16; ++i)
        check(s.actors().actor(s.actors().id(i))->hitPoints == 1000, "Area excludes the second row");
    const auto far = spellPlacements(map);
    GameSession illegal(map, catalog, far, 5, spellSpawn, false);
    auto flame = spellCommand(illegal, SpellKind::SacredFlame);
    flame.targetCount = 1;
    flame.enemies[0] = illegal.actors().id(0);
    check(illegal.submit(flame).reason == CommandReason::NotVisible &&
              illegal.party().character({1})->mana == 4,
          "Walls block hostile targeting before costs");
}
void timingAndInterruption() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    const NpcWorld places(map, spellSpawn.cell, {"goblin"},
                          {{{0}, {6, 6}, {0, 0}, Heading::South},
                           {{0}, {6, 4}, {0, 0}, Heading::South},
                           {{0}, {6, 2}, {0, 0}, Heading::South}});
    GameSession large(map, catalog, places, 5, spellSpawn, false),
        split(map, catalog, places, 5, spellSpawn, false);
    for (auto* s : {&large, &split}) {
        auto c = spellCommand(*s, SpellKind::MagicMissile);
        c.targetCount = 3;
        for (int i = 0; i < 3; ++i) {
            c.enemies[i] = s->actors().id(i);
            c.darts[i] = 1;
        }
        s->submit(c);
    }
    large.update(.45, false);
    split.update(.234, false);
    split.update(.108, false);
    split.update(.108, false);
    for (int i = 0; i < 3; ++i)
        check(large.actors().actor(large.actors().id(i))->hitPoints ==
                  split.actors().actor(split.actors().id(i))->hitPoints,
              "Large and split updates preserve staggered recipient impacts");
    const auto close = spellPlacements(map, 12, true);
    GameSession canceled(map, catalog, close, 5, spellSpawn, false);
    spellReady(canceled, 1);
    auto inflict = spellCommand(canceled, SpellKind::InflictWounds);
    inflict.targetCount = 1;
    inflict.enemies[0] = canceled.actors().id(2);
    canceled.submit(inflict);
    canceled.update(.199, false);
    check(canceled.actors().actor(inflict.enemies[0])->hitPoints == 1000,
          "Melee spell waits for 200 ms strike");
    canceled.party().damage({1}, 1000);
    canceled.update(.001, false);
    check(canceled.actors().actor(inflict.enemies[0])->hitPoints == 1000 &&
              canceled.party().character({1})->mana == 2,
          "Caster incapacitation cancels unreleased melee spell without refund");
}
// Public-owner capacity fixture; deliberate viewpoint jumps book all 256 NPCs.
// Keep separate from routine CTest and from claims about ordinary gameplay.
void populationStress() {
    std::vector<std::string> rows(40, std::string(40, '.'));
    for (int i = 0; i < 40; ++i)
        rows[0][i] = rows[39][i] = rows[i][0] = rows[i][39] = '#';
    const Map map(rows);
    const Player player{{20, 30}, Heading::North};
    const auto catalog = spellCatalog(1000, 1, 10, true);
    std::vector<NpcInstance> instances;
    for (int y = 1; y <= 16; ++y)
        for (int x = 1; x <= 16; ++x)
            instances.push_back({{0}, {x, y}, {0, 0}, Heading::South});
    const NpcWorld placements(map, player.cell, {"goblin"}, instances);
    double totalUs = 0, maxUs = 0;
    for (int repeat = 0; repeat < 100; ++repeat) {
        Party party(catalog);
        ActorWorld actors(map, catalog, placements);
        rules::DiceRng rng(5);
        NpcAiController ai(map, actors, 5);
        EffectController effects(party, actors, rng);
        CombatEvents events;
        CombatController combat(map, party, actors, rng, ai, effects);
        Player pose = player;
        for (std::size_t i = 0; i < NpcWorld::maxInstances; ++i) {
            pose.cell = {instances[i].cell.x, instances[i].cell.y + 2};
            ai.alert(actors.id(i), pose.cell);
            combat.observe(pose);
            events.clear();
            check(combat.update(pose, false, 0, events).event == CombatEvent::AttackStarted,
                  "Every NPC books a pending slot beside spell casters");
        }
        pose.cell = {15, 18}; // All three recipients lie inside the view cone.
        for (std::uint16_t i = 0; i < rules::partySize; ++i) {
            PartyCommand command;
            command.character = {i};
            command.action = PartyAction::Spell;
            command.spell = catalog.findSpell("magic_missile");
            command.world = combat.snapshot().world;
            command.turn = combat.snapshot().turn;
            command.targetCount = 3;
            for (std::size_t j = 0; j < 3; ++j) {
                command.enemies[j] = actors.id(NpcWorld::maxInstances - 3 + j);
                command.darts[j] = 1;
            }
            events.clear();
            check(combat.submit(command, pose, 0, events).event == CombatEvent::AttackStarted,
                  "Four triple-recipient spells coexist with full NPC population");
        }
        events.clear();
        const auto start = std::chrono::steady_clock::now();
        combat.resolveImpacts(pose, 3000, events);
        const auto elapsed =
            std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
        totalUs += elapsed;
        maxUs = std::max(maxUs, elapsed);
        unsigned impacts = 0, releases = 0;
        for (const auto& event : events) {
            impacts += event.event == CombatEvent::Attack;
            releases += event.event == CombatEvent::ProjectileReleased;
        }
        check(impacts == 268 && releases == 260,
              "Full population and twelve spell recipients drain 528 events without loss");
        combat.resolveImpacts(pose, 3000, events);
        check(events.count == 528, "Spell burst never repeats consequences");
    }
    std::cout << "256 NPCs + 12 spell recipients, 528 drain events, 100 bursts; mean_us=" << totalUs / 100
              << " max_us=" << maxUs << '\n';
}
void trace() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    const auto places = spellPlacements(map);
    GameSession s(map, catalog, places, 5, spellSpawn, false);
    auto c = spellCommand(s, SpellKind::MagicMissile);
    c.targetCount = 1;
    c.enemies[0] = s.actors().id(0);
    c.darts[0] = 3;
    s.submit(c);
    std::cout << "cast t=" << s.timeMs() << " mana=" << s.party().character({3})->mana
              << " enemyHP=" << s.actors().actor(c.enemies[0])->hitPoints << '\n';
    s.update(.342, false);
    for (const auto& e : s.events())
        if (e.event == CombatEvent::Attack)
            std::cout << "impact t=" << e.occurredMs << " darts=" << unsigned(e.darts)
                      << " damage=" << e.damage << " enemyHP=" << s.actors().actor(e.enemy)->hitPoints
                      << '\n';
}
} // namespace
int main(int argc, char** argv) {
    try {
        resourcesAndHealing();
        features();
        effectsAndValidation();
        projectiles();
        impactAlerts();
        meleeAlertTiming();
        allDamageSpells();
        invalidAndPool();
        ui();
        classFeaturesUi();
        uiRevision();
        sharedArea();
        markerLayoutAndLog();
        guidingAndSneak();
        geometryAndPush();
        timingAndInterruption();
        if (argc > 1 && !std::strcmp(argv[1], "--trace"))
            trace();
        if (argc > 1 && !std::strcmp(argv[1], "--stress")) {
            populationStress();
            const auto start = std::chrono::steady_clock::now();
            for (int i = 0; i < 100; ++i) {
                allDamageSpells();
                invalidAndPool();
            }
            std::cout
                << "100 area/effect-capacity cycles: "
                << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count()
                << " ms\n";
        }
        std::cout << "Spell checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

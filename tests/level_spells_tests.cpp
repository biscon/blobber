#include "game/arena.hpp"
#include "game/commands.hpp"
#include "support/check.hpp"
#include "support/progression_fixtures.hpp"
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
            hold.caster = {3};
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
            check(cells == 4 && s.effects().concentration({3}).serial,
                  "Web reserves full four-cell footprint");
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
    r.caster = {1};
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
    r.caster = {3};
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
                optional();
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
    check(thief.submit(normal).reason == CommandReason::Resolved, "Thief spends normal action first");
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
          "Thief potion consumes optional allowance after normal action");
    thief.update(1.1, false);
    check(thief.submit(featureCommand(thief, 2, Feature::CunningAction)).reason != CommandReason::Resolved,
          "potion and Cunning Action share allowance");
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
    check(active == 16 && restrained == 64, "sixteen Web cells restrain sixty-four occupants");
    for (const auto& a : effects.ground().areas)
        effects.ignite(a.cell);
    events.clear();
    effects.advance(6000, 6000, true, events);
    check(events.count < CombatEvents::capacity, "full burn batch fits fixed events");
    for (const auto& a : effects.ground().areas)
        check(!a.active, "all burned cells release capacity");
    for (unsigned i = 0; i < EffectController::capacity; ++i) {
        EffectRequest r;
        r.world = actors.snapshot().world;
        r.caster = {3};
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
        aidAndMirror();
        effects();
        features();
        turnAllowances();
        std::cout << "Level spell checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

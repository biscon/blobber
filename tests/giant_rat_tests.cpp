#include "game/commands.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include <cstring>
#include <iostream>
using namespace blobber;
namespace {
using Mode = rules::RollMode;
constexpr Player pose{{3, 3}, Heading::North};
rules::Catalog ratCatalog(bool pack = true) {
    auto f = partyFixture();
    rules::MonsterDefinition rat;
    rat.id = "giant.rat";
    rat.abilities = {7, 15, 11, 2, 10, 4};
    rat.armorClass = 12;
    rat.hitPoints = 7;
    rat.experience = 25;
    rat.attacks[0] = {0};
    rat.attackCount = 1;
    rat.traits = pack ? static_cast<rules::Traits>(rules::Trait::PackTactics) : 0;
    auto ally = rat;
    ally.id = "fixture.ally";
    ally.humanoid = true;
    ally.traits = 0;
    return {
        {{"giant.rat.bite", rules::AttackKind::Melee, 4, {1, 4, 2}, rules::DamageType::Piercing}},
        {rat, ally},
        f.races,
        f.classes,
        f.items,
        f.preset,
        {{"hold_person", "Hold Person", "Owned spell eligibility fixture", rules::SpellKind::HoldPerson}}};
}
NpcTimings timing() {
    NpcTimings t{};
    t[0] = {{{600, 600}}, 2};
    t[1] = {{{300, 300}}, 2};
    t[2] = {{{300, 150, 150, 300}}, 4};
    t[3] = {{{150, 150}}, 2};
    t[4] = {{{200, 200, 200, 600}}, 4};
    return t;
}
NpcInstance supporter(Cell cell = {3, 2}, Cell subcell = {0, 0}, unsigned type = 0) {
    return {{std::uint16_t(type)}, cell, subcell, Heading::North};
}
struct Fixture {
    NpcWorld placement;
    Party party;
    ActorWorld actors;
    rules::DiceRng rng;
    NpcAiController ai;
    EffectController effects;
    CombatController combat;
    CombatEvents events;
    Fixture(GeometryView map, const rules::Catalog& c, std::vector<NpcInstance> allies = {},
            unsigned seed = 5)
        : placement(map.base(), pose.cell, {"giant.rat", "fixture.ally"}, instances(std::move(allies))),
          party(c), actors(map, c, placement), rng(seed), ai(map, actors, seed), effects(party, actors, rng),
          combat(map, party, actors, rng, ai, effects, {timing(), timing()}) {
        for (unsigned i = 1; i < 4; ++i)
            party.damage({std::uint16_t(i)}, 10000); // One target: no random target-selection roll.
    }
    static std::vector<NpcInstance> instances(std::vector<NpcInstance> allies) {
        allies.insert(allies.begin(), {{0}, {3, 2}, {0, 1}, Heading::South});
        return allies;
    }
    void begin(unsigned initiativeSeed = 8) {
        // Isolate attack-roll tests from earlier condition-save RNG in fixture setup.
        rng = rules::DiceRng(initiativeSeed);
        combat.observe(pose);
        check(combat.contact(pose), "Rat starts ordinary melee contact");
        combat.update(pose, false, 0, events);
    }
    CombatOutcome accept(rules::DiceRng& expected, std::uint64_t now = 450) {
        check(combat.snapshot().activeEnemy && combat.snapshot().activeEnemy->slot == 0,
              "Fixture rat acts first without party Defend affecting its roll");
        expected = rng;
        const auto hp = party.character({0})->hitPoints;
        watching = true;
        allocations = 0;
        auto out = combat.update(pose, false, now, events);
        watching = false;
        check(allocations == 0, "Support scan and accepted attack allocate no engine memory");
        check(out.event == CombatEvent::AttackStarted && out.enemy.slot == 0 && out.impactMs == 450,
              "Rat uses a registered quick bite at 450 ms");
        check(!out.packTactics && out.attackRollMode == Mode::Normal && out.roll == 0 && out.damage == 0 &&
                  !out.hit && !out.critical && party.character({0})->hitPoints == hp,
              "Windup keeps accepted combat result and pack roll context private");
        return out;
    }
    CombatOutcome impact(Mode mode, rules::DiceRng expected, bool supported, std::uint64_t when = 900) {
        const auto roll = expected.d20(mode).value;
        const auto hit = rules::resolveAttack(roll, 4, effects.armorClass(rules::CharacterId{0})).value;
        const bool critical = hit.outcome == rules::AttackOutcome::Critical;
        const bool lands = hit.outcome != rules::AttackOutcome::Miss;
        const auto damage = lands ? expected.damage({1, 4, 2}, critical).value.total : 0;
        const auto hp = party.character({0})->hitPoints;
        const bool concentrationSave = damage > 0 && effects.concentration({0}).serial != 0;
        events.clear();
        combat.resolveImpacts(pose, when - 1, events);
        check(party.character({0})->hitPoints == hp, "No early bite damage");
        watching = true;
        allocations = 0;
        combat.resolveImpacts(pose, when, events);
        watching = false;
        check(allocations == 0, "Rat impact uses existing bounded storage");
        CombatOutcome result;
        unsigned count = 0;
        for (const auto& e : events)
            if (e.event == CombatEvent::Attack && e.enemy.slot == 0) {
                result = e;
                ++count;
            }
        check(count == 1 && result.enemyAttack && result.packTactics == supported &&
                  result.attackRollMode == mode && result.roll == roll.selected && result.hit == lands &&
                  result.critical == critical && result.rawDamage == damage && result.damage == damage &&
                  result.damageType == rules::DamageType::Piercing,
              "Resolved bite carries exact selected d20, pack eligibility, mode and 1d4+2/2d4+2 damage");
        check(party.character({0})->hitPoints == std::max<std::int64_t>(0, hp - damage),
              "Bite uses ordinary party health resolution");
        if (concentrationSave)
            expected.d20(); // Existing concentration consequence of damaging the fixture caster.
        check(rng.d20().value.selected == expected.d20().value.selected,
              "Support evaluation adds no RNG, advantage uses exactly one extra d20");
        return result;
    }
    CombatOutcome strike(Mode mode, bool supported) {
        rules::DiceRng expected(0);
        accept(expected);
        return impact(mode, expected, supported);
    }
};
void damageOutcomes() {
    const auto c = ratCatalog();
    const auto map = combatMap();
    for (const bool supported : {false, true}) {
        bool hit = false, miss = false, critical = false;
        for (const unsigned seed : {2u, 6u, 8u, 11u}) {
            Fixture f(map, c, supported ? std::vector<NpcInstance>{supporter()} : std::vector<NpcInstance>{});
            f.begin(seed);
            const auto out = f.strike(supported ? Mode::Advantage : Mode::Normal, supported);
            hit |= out.hit && !out.critical;
            miss |= !out.hit;
            critical |= out.critical;
        }
        check(hit && miss && critical, "Selected fixed seeds exercise normal, missed and critical bites");
    }
}

void supportGeometry() {
    const auto c = ratCatalog();
    const auto map = combatMap();
    for (unsigned scenario = 0; scenario < 10; ++scenario) {
        std::vector<NpcInstance> allies;
        bool supported = scenario != 0 && scenario != 6 && scenario != 7 && scenario != 8;
        if (scenario == 1)
            allies = {supporter()}; // Same cell, rear paired subcell.
        if (scenario == 2)
            allies = {supporter({4, 3})};
        if (scenario == 3)
            allies = {supporter({4, 2})}; // Clear diagonal.
        if (scenario == 4)
            allies = {supporter({3, 4})}; // Behind party, no facing/view requirement.
        if (scenario == 5)
            allies = {supporter({3, 2}, {0, 0}, 1)}; // Different species.
        if (scenario == 6)
            allies = {supporter({4, 1})}; // Beyond melee contact.
        if (scenario == 7 || scenario == 8)
            allies = {supporter()};
        if (scenario == 9)
            allies = {supporter(), supporter({4, 3}), supporter({2, 3})};
        Fixture f(map, c, allies);
        if (scenario == 7)
            f.actors.damage(f.actors.id(1), 7, 0);
        if (scenario == 8)
            f.actors.remove(f.actors.id(1));
        f.begin();
        f.strike(supported ? Mode::Advantage : Mode::Normal, supported);
    }
    const Map corner({"#######", "#.....#", "#.....#", "#...#.#", "#.....#", "#.....#", "#######"});
    Fixture blocked(corner, c, {supporter({4, 2})});
    blocked.begin();
    blocked.strike(Mode::Normal, false);
    WorldDefinition world;
    world.surfaces.resize(49);
    world.surfaces[3 * 7 + 4] = {2, 6};
    const Map elevated({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"}, world);
    Fixture tooHigh(elevated, c, {supporter({4, 3})});
    tooHigh.begin();
    tooHigh.strike(Mode::Normal, false);
    FloorFixtures doors;
    doors.count = 1;
    doors.entries[0] = {{4, 3}, Heading::South, FixtureKind::Door};
    FloorGeometry geometry(map, doors);
    Fixture behindDoor(geometry.view(), c, {supporter({4, 2})});
    behindDoor.begin();
    behindDoor.strike(Mode::Normal, false); // Closed door blocks a diagonal side, not just distance.
    const auto ordinary = ratCatalog(false);
    Fixture noTrait(map, ordinary, {supporter()});
    noTrait.begin();
    noTrait.strike(Mode::Normal, false);
}
void conditions() {
    const auto c = ratCatalog();
    const auto map = combatMap();
    for (const auto kind : {EffectKind::Sleep, EffectKind::Knockdown, EffectKind::Blind, EffectKind::Slow}) {
        Fixture f(map, c, {supporter()});
        f.effects.condition(f.actors.id(1), kind, {0}, 0, f.events);
        const bool supported = kind != EffectKind::Sleep;
        check(f.effects.conscious(f.actors.id(1)) == supported,
              "Prone, blind and slow differ from incapacitation");
        f.begin();
        f.strike(supported ? Mode::Advantage : Mode::Normal, supported);
    }
    Fixture held(map, c, {supporter({3, 2}, {0, 0}, 1)});
    EffectRequest hold;
    hold.world = held.actors.snapshot().world;
    hold.caster = rules::CharacterId{0};
    hold.kind = EffectKind::HoldPerson;
    hold.targets[0] = held.actors.id(1);
    hold.count = 1;
    hold.difficulty = 1000;
    check(bool(held.effects.apply(hold, 0, held.events)) && held.effects.has(held.actors.id(1), hold.kind),
          "Existing Hold Person incapacitates the humanoid supporter");
    held.begin();
    held.strike(Mode::Normal, false);
    Fixture restrained(map, c, {supporter({4, 3})});
    const Cell webCell{4, 3};
    check(restrained.effects.createWeb(&webCell, 1, {0}, 1000, 0, restrained.events),
          "Real Web area created");
    restrained.effects.groundCheck(restrained.actors.id(1), webCell, 0, restrained.events);
    check(restrained.effects.has(restrained.actors.id(1), EffectKind::Restrained) &&
              restrained.effects.conscious(restrained.actors.id(1)) &&
              !restrained.effects.mobile(restrained.actors.id(1)),
          "Restrained ally remains conscious despite navigation immobilization");
    restrained.begin();
    restrained.strike(Mode::Advantage, true);
    check(!restrained.effects.conscious(ActorId{1, restrained.actors.snapshot().world + 1}),
          "Foreign actor IDs cannot provide conscious support");
    for (unsigned scenario = 0; scenario < 3; ++scenario) {
        Fixture f(map, c, {supporter()});
        if (scenario != 1)
            f.effects.condition(f.actors.id(0), EffectKind::Blind, {0}, 0, f.events);
        if (scenario != 0)
            f.effects.condition(rules::CharacterId{0}, EffectKind::Blind, {0}, 0, f.events);
        f.begin();
        f.strike(scenario == 1 ? Mode::Advantage : Mode::Normal, true);
    }
}
void defendCancellation() {
    const auto c = ratCatalog();
    const auto map = combatMap();
    Fixture f(map, c, {supporter()});
    f.begin(1); // Selected initiative puts the sole living party member before the rat.
    check(f.combat.snapshot().activeCharacter.has_value(), "Defender acts first in owned fixture");
    f.combat.update(pose, false, 450, f.events);
    PartyCommand command;
    command.character = {0};
    command.action = PartyAction::Defend;
    command.world = f.combat.snapshot().world;
    command.turn = f.combat.snapshot().turn;
    check(f.combat.submit(command, pose, 450, f.events).event == CombatEvent::Defend,
          "Normal party Defend action accepted");
    rules::DiceRng expected(0);
    f.accept(expected, 900);
    f.impact(Mode::Normal, expected, true, 1350);
    Fixture solo(map, c);
    solo.effects.condition(solo.actors.id(0), EffectKind::Blind, {0}, 0, solo.events);
    solo.begin();
    solo.strike(Mode::Disadvantage, false);
}

void savedWindups() {
    const auto c = ratCatalog();
    const auto map = combatMap();
    for (unsigned action = 0; action < 3; ++action) {
        Fixture f(map, c, {supporter()});
        f.begin();
        rules::DiceRng expected(0);
        f.accept(expected);
        if (action == 0)
            f.actors.damage(f.actors.id(1), 7, 600);
        if (action == 1)
            f.actors.remove(f.actors.id(1));
        if (action == 2)
            f.effects.condition(f.actors.id(1), EffectKind::Sleep, {0}, 600, f.events);
        f.impact(Mode::Advantage, expected, true);
    }
    Fixture canceled(map, c, {supporter()});
    canceled.begin();
    rules::DiceRng expected(0);
    canceled.accept(expected);
    canceled.effects.condition(canceled.actors.id(0), EffectKind::Sleep, {0}, 600, canceled.events);
    canceled.combat.reconcileEffects(600, canceled.events);
    bool found = false;
    for (const auto& e : canceled.events)
        if (e.event == CombatEvent::AttackCanceled) {
            found = true;
            check(!e.packTactics && e.roll == 0 && e.damage == 0 && e.attackRollMode == Mode::Normal,
                  "Canceled bite does not disclose saved pack result");
        }
    check(found, "Incapacitated attacker cancels its own bite");
}
void restoredPack() {
    const auto c = ratCatalog();
    const auto map = combatMap();
    const Player spawn{{3, 5}, Heading::North};
    const NpcWorld places(map, spawn.cell, {"giant.rat"},
                          {{{0}, {3, 2}, {0, 1}, Heading::South}, {{0}, {3, 2}, {0, 0}, Heading::South}});
    GameSession original(map, c, places, 8, spawn, false, {timing()});
    auto saved = std::make_unique<FloorState>();
    original.exportFloor(*saved);
    GameSession resumed(map, c, places, 99, spawn, false, {timing()});
    resumed.importFloor(*saved);
    original.restorePose(pose, false);
    resumed.restorePose(pose, false);
    bool packAttack = false;
    for (unsigned step = 0; step < 200 && !packAttack; ++step) {
        const auto& a = original.update(.01, false);
        const auto& b = resumed.update(.01, false);
        check(a.count == b.count, "Restored pack preserves event scheduling");
        for (std::size_t i = 0; i < a.count; ++i) {
            const auto& x = a.values[i];
            const auto& y = b.values[i];
            check(x.event == y.event && x.roll == y.roll && x.damage == y.damage &&
                      x.packTactics == y.packTactics && x.attackRollMode == y.attackRollMode,
                  "Restored world identities preserve accepted support and RNG results");
            packAttack |= x.event == CombatEvent::Attack && x.enemyAttack && x.packTactics;
        }
    }
    check(packAttack, "Pack advantage survives a real floor-state export/import");
}

void rewardsAndFeedback() {
    const auto c = ratCatalog();
    const auto map = combatMap();
    const NpcWorld p(map, pose.cell, {"giant.rat"}, {{{0}, {3, 2}, {0, 1}, Heading::South}});
    GameSession s(map, c, p, 5, pose, false, {timing()});
    CommandController ui(s);
    PartyCommand hold;
    hold.character = {0};
    hold.spell = c.findSpell("hold_person");
    hold.enemies[0] = s.actors().id(0);
    check(s.combat().spellRecipient(hold, pose, 0) == CommandReason::InvalidTarget,
          "Beast rat is not a legal Hold Person target");
    check(!s.effects().sleepImmune(s.actors().id(0)), "Standard rat remains susceptible to Sleep");
    check(ui.snapshot().enemyPortraits[0] == EnemyPortrait::GiantRat,
          "Initiative identifies giant rats independently of goblins");
    CombatOutcome out;
    out.event = CombatEvent::Attack;
    out.enemyAttack = true;
    out.enemy = s.actors().id(0);
    out.recipient = {0};
    out.packTactics = true;
    out.attackRollMode = Mode::Advantage;
    watching = true;
    allocations = 0;
    ui.record(out);
    watching = false;
    check(allocations == 0 && std::strstr(ui.snapshot().log.newest(0), "Pack Tactics: advantage"),
          "Pack attack feedback is explanatory and allocation-free");
    out.attackRollMode = Mode::Normal;
    ui.record(out);
    check(std::strstr(ui.snapshot().log.newest(0), "canceled by disadvantage"),
          "Cancellation feedback does not claim an advantaged roll");
    s.actors().damage(s.actors().id(0), 7, 0);
    s.update(0, true);
    for (unsigned i = 0; i < 4; ++i)
        check(s.party().character({std::uint16_t(i)})->experience == 6 &&
                  s.party().character({std::uint16_t(i)})->experienceFraction == 3,
              "25 XP splits exactly in twelfths without loss");
    s.update(0, true);
    check(s.party().character({0})->experience == 6, "Rat reward occurs once");
    for (const auto& stack : s.loot().snapshot().stacks)
        check(!stack.active, "Standard rat drops no equipment, gold or potions");
}
} // namespace
int main() {
    try {
        damageOutcomes();
        supportGeometry();
        conditions();
        defendCancellation();
        savedWindups();
        restoredPack();
        rewardsAndFeedback();
        std::cout << "Giant rat checks passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}

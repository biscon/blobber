#include "game/commands.hpp"
#include "game/tactical_navigation.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/spell_fixtures.hpp"
#include <iostream>
using namespace blobber;
using K = rules::SpellKind;
namespace {
Map room() {
    std::vector<std::string> rows(25, std::string(25, '.'));
    rows.front() = rows.back() = std::string(25, '#');
    for (auto& r : rows)
        r.front() = r.back() = '#';
    return Map(rows);
}
void shapes() {
    const auto map = room();
    const Player pose{{12, 15}, Heading::North};
    for (auto pair : {std::pair<K, unsigned>{K::Shatter, 12}, {K::Sleep, 36}, {K::Grease, 4}, {K::Web, 16}}) {
        const auto fp = spellFootprint(map, pose, 3, pair.first, {24, 24});
        check(fp.count() == pair.second, "Each fixed template uses five-foot square centres");
        check(fp.contains({24, 24}), "Ground templates straddle a four-cell corner");
    }
    const auto grease = spellFootprint(map, pose, 3, K::Grease, {24, 24});
    for (int y = 23; y <= 24; ++y)
        for (int x = 23; x <= 24; ++x)
            check(grease.contains({x, y}), "Corner-aligned Grease covers one square from four cells");
    const auto circle = spellFootprint(map, pose, 3, K::Shatter, {24, 24});
    check(!circle.contains({22, 22}) && circle.contains({23, 22}),
          "Shatter excludes square bounding-box corners");
    for (unsigned h = 0; h < 4; ++h) {
        const Player p{pose.cell, Heading(h)};
        const auto frontLeft = formationSubcell(p, 0), f = forward(p.heading), r = right(p.heading);
        for (unsigned slot = 0; slot < 4; ++slot) {
            const auto fan = spellFootprint(map, p, slot, K::BurningHands, {});
            const auto square = spellFootprint(map, p, slot, K::Thunderwave, {});
            check(fan.count() == 6 && square.count() == 8, "Fixed forward templates have six/eight squares");
            check(fan == spellFootprint(map, p, 0, K::BurningHands, {}) &&
                      square == spellFootprint(map, p, 0, K::Thunderwave, {}) &&
                      fan == spellFootprint(map, p, slot, K::ColorSpray, {}),
                  "All caster slots use identical party-front templates");
            for (int d = -1; d <= 4; ++d)
                for (int lane = -2; lane <= 3; ++lane) {
                    const auto tile = frontLeft + f * d + r * lane;
                    const bool inFan =
                        (d == 1 && lane >= 0 && lane <= 1) || (d == 2 && lane >= -1 && lane <= 2);
                    const bool inSquare = d >= 1 && d <= 2 && lane >= -1 && lane <= 2;
                    check(fan.contains(tile) == inFan, "Fan is two near squares then four far squares");
                    check(square.contains(tile) == inSquare,
                          "Thunderwave is two symmetric four-wide rows immediately beyond party front");
                }
            for (unsigned partySlot = 0; partySlot < 4; ++partySlot)
                check(!fan.contains(formationSubcell(p, partySlot)) &&
                          !square.contains(formationSubcell(p, partySlot)),
                      "Forward spells exclude the entire occupied party cell in every heading");
        }
    }
    auto blockedRows = std::vector<std::string>(25, std::string(25, '.'));
    blockedRows.front() = blockedRows.back() = std::string(25, '#');
    for (auto& row : blockedRows)
        row.front() = row.back() = '#';
    blockedRows[14][12] = '#';
    const Map blocked(blockedRows);
    for (auto kind : {K::BurningHands, K::ColorSpray, K::Thunderwave}) {
        const auto fp = spellFootprint(blocked, pose, 3, kind, {});
        check(fp.count() < (kind == K::Thunderwave ? 8u : 6u), "Walls clip fixed forward shapes");
        for (unsigned bit = 0; bit < 64; ++bit)
            if (fp.mask & (UINT64_C(1) << bit))
                check(!blocked.solid(dungeonCell(fp.tile(bit))) && !(dungeonCell(fp.tile(bit)) == pose.cell),
                      "Clipped forward masks contain only floor outside the party cell");
    }
    check(!validSpellAim(map, pose, 3, {24, 0}) && !validSpellAim(map, pose, 3, {-1, 24}),
          "Range and map boundaries reject invalid aims");
    const Map wall(
        {"#########", "#...#...#", "#...#...#", "#...#...#", "#...#...#", "#...#...#", "#########"});
    const auto clipped = spellFootprint(wall, {{2, 4}, Heading::North}, 0, K::Sleep, {7, 5});
    for (unsigned bit = 0; bit < 64; ++bit)
        if (clipped.mask & (UINT64_C(1) << bit))
            check(clipped.tile(bit).x < 8, "Ground area cannot leak through an unbroken wall");
}
void terrainAndSleep() {
    const auto map = room();
    const auto cat = spellCatalog();
    const Player pose{{12, 15}, Heading::North};
    NpcWorld places(map, pose.cell, {"goblin"}, {{{0}, {12, 10}, {0, 0}, Heading::South}});
    Party party(cat);
    ActorWorld actors(map, cat, places);
    rules::DiceRng rng{5};
    CombatEvents events;
    EffectController e(party, actors, rng);
    e.partyPose(pose);
    e.bindEvents(events, 0);
    const auto id = actors.id(0);
    const SubcellFootprint strip{{24, 20}, 3};
    check(e.createAreaFootprint(strip, {3}, 1, 0, events).serial != 0,
          "Partial terrain cast reserves one logical area");
    check(e.areaAt({24, 20}) && !e.areaAt({24, 21}), "Terrain membership is subcell precise");
    events.clear();
    allocations = 0;
    watching = true;
    e.cross(id, {23, 20}, {24, 20}, 0);
    watching = false;
    check(events.count > 0 && !allocations, "Entering partial terrain saves without allocation");
    events.clear();
    e.cross(id, {24, 20}, {25, 20}, 0);
    check(!events.count, "Moving inside one continuous cast does not repeat entry saves");
    const SubcellFootprint web{{24, 20}, UINT64_C(0x303)};
    check(e.createWebFootprint(web, {3}, 100, 0, events), "Web owns a full logical footprint");
    e.ignite(SubcellFootprint{{24, 20}, 1});
    e.advance(6000, 6000, true, events);
    bool survivor = false;
    for (const auto& a : e.ground().areas)
        if (a.active && a.web) {
            survivor = true;
            check(a.footprint.count() == 3 && !a.footprint.contains({24, 20}),
                  "Fire removes only ignited Web squares");
        }
    check(survivor, "Partial burning retains the rest of the cast");
    e.condition(id, EffectKind::Sleep, {3}, 6000, events);
    check(e.sleepRemaining(id) == 12000, "Sleep starts with a two-round lifetime");
    e.advance(17999, 17999, true, events);
    check(e.has(id, EffectKind::Sleep), "Sleep remains immediately before its deadline");
    e.advance(18000, 18000, true, events);
    check(!e.has(id, EffectKind::Sleep), "Sleep expires at exactly twelve gameplay seconds");
}
void routes() {
    const auto map = room();
    const auto cat = combatCatalog();
    const Player p{{12, 20}, Heading::North};
    NpcWorld places(map, p.cell, {"goblin"},
                    {{{0}, {12, 10}, {0, 0}, Heading::South},
                     {{0}, {12, 12}, {0, 0}, Heading::South},
                     {{0}, {13, 10}, {0, 0}, Heading::South}});
    ActorWorld actors(map, cat, places);
    TacticalNavigation nav(map, actors);
    TacticalNavigation::WakeGoals goals;
    goals.count = 2;
    goals.budgetFeet = 20;
    goals.allies[0] = actors.id(1);
    goals.allies[1] = actors.id(2);
    goals.rank[0] = 0;
    goals.rank[1] = 1;
    std::size_t work = 0;
    TacticalNavigation::Result result;
    for (unsigned i = 0; i < 100; ++i) {
        work = 0;
        allocations = 0;
        watching = true;
        result = nav.next(actors.id(0), p, p.cell, false, nullptr, work, &goals);
        watching = false;
        check(!allocations, "Wake route uses preallocated workspace");
        if (result.status != TacticalNavigation::Status::Pending)
            break;
    }
    check(result.status == TacticalNavigation::Status::Step && result.target.slot == 2 &&
              result.totalCostFeet == 5,
          "Rescue chooses cheapest reachable route before initiative preference");
    nav.cancel();
    goals.budgetFeet = 0;
    work = 0;
    do {
        work = 0;
        result = nav.next(actors.id(0), p, p.cell, false, nullptr, work, &goals);
    } while (result.status == TacticalNavigation::Status::Pending);
    check(result.status == TacticalNavigation::Status::Blocked, "Rescue never borrows Dash movement");
    check(wakeChance(5) == 0 && wakeChance(6) == 10 && wakeChance(10) == 20 && wakeChance(18) == 40 &&
              wakeChance(30) == 50,
          "Intelligence gates and caps occasional rescue decisions");
}
rules::Catalog rescueCatalog(int intelligence) {
    auto base = combatCatalog(1000, -100, {1, 4, -100});
    auto f = partyFixture();
    auto m = *base.monster({0});
    m.abilities[3] = intelligence;
    return {{*base.attack({0})}, {m}, f.races, f.classes, f.items, f.preset};
}
bool rescueTurn(unsigned seed, int intelligence, bool expireSoon = false) {
    const auto map = room();
    const auto cat = rescueCatalog(intelligence);
    const Player pose{{12, 20}, Heading::North};
    NpcWorld places(map, pose.cell, {"goblin"},
                    {{{0}, {12, 19}, {0, 1}, Heading::South},
                     {{0}, {12, 16}, {0, 0}, Heading::South},
                     {{0}, {12, 15}, {0, 0}, Heading::South}});
    Party party(cat);
    ActorWorld actors(map, cat, places);
    rules::DiceRng rng(seed);
    NpcAiController ai(map, actors, seed);
    EffectController e(party, actors, rng);
    CombatController combat(map, party, actors, rng, ai, e);
    CombatEvents events;
    ai.bindEffects(&e);
    e.partyPose(pose);
    e.bindEvents(events, 0);
    combat.enableRoaming(false);
    e.condition(actors.id(expireSoon ? 0 : 2), EffectKind::Sleep, {0}, 0, events);
    if (expireSoon)
        combat.restTime(6000, 6000, events);
    for (unsigned i = 0; i < 3; ++i)
        ai.alert(actors.id(i), pose.cell);
    check(combat.contact(pose, expireSoon ? 6000 : 0), "Rescue fixture enters contact");
    unsigned moves = 0, attacks = 0, dashes = 0, wakes = 0;
    bool started = false;
    std::uint64_t now = expireSoon ? 6000 : 0;
    for (unsigned tick = 0; tick < 2000; ++tick) {
        if (combat.snapshot().activeEnemy && combat.snapshot().activeEnemy->slot == 1)
            started = true;
        events.clear();
        now += 100;
        allocations = 0;
        watching = true;
        combat.resolveImpacts(pose, now, events, 100);
        events.append(combat.update(pose, false, now, events));
        watching = false;
        check(!allocations, "Rescue decision, search and execution allocate no engine C++ storage");
        for (const auto& event : events)
            if (event.enemy.world && event.enemy.slot == 1) {
                moves += event.event == CombatEvent::Move;
                attacks += event.event == CombatEvent::AttackStarted;
                dashes += event.event == CombatEvent::Dash;
                wakes += event.event == CombatEvent::Wake;
                if (event.event == CombatEvent::Wake)
                    check(event.awakened.slot == 2, "Wake event identifies both allies");
            }
        if (combat.snapshot().awaitingAction) {
            PartyCommand c;
            c.world = combat.snapshot().world;
            c.turn = combat.snapshot().turn;
            c.character = *combat.snapshot().activeCharacter;
            c.action = PartyAction::Defend;
            combat.submit(c, pose, now, events);
        }
        if (started && (!combat.snapshot().activeEnemy || combat.snapshot().activeEnemy->slot != 1)) {
            if (wakes)
                check(wakes == 1 && moves == 1 && !attacks && !dashes &&
                          !e.has(actors.id(2), EffectKind::Sleep),
                      "Successful rescue moves once and spends the sole action waking its ally");
            return wakes != 0;
        }
    }
    check(false, "Rescue turn must finish");
    return false;
}
void rescueDecisions() {
    unsigned rescued = 0;
    for (unsigned seed = 1; seed <= 30; ++seed)
        rescued += rescueTurn(seed, 10);
    check(rescued > 0 && rescued < 30,
          "Intelligent enemies sometimes rescue, sometimes continue normal combat");
    for (unsigned seed = 1; seed <= 8; ++seed)
        check(!rescueTurn(seed, 10, true),
              "Enemies leave already-spent allies to wake naturally before their next action");
    for (unsigned seed = 1; seed <= 8; ++seed)
        check(!rescueTurn(seed, 5), "Low-intelligence enemies never deliberately wake allies");
}
} // namespace
int main() {
    shapes();
    terrainAndSleep();
    routes();
    rescueDecisions();
    std::cout << "Precise spell geometry, terrain, Sleep and rescue routes passed\n";
}

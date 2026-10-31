#include "game/commands.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include <iostream>
using namespace blobber;
namespace {
rules::Catalog bugbearCatalog() {
    auto f = partyFixture();
    rules::MonsterDefinition m;
    m.id = "bugbear";
    m.abilities = {15, 14, 13, 8, 11, 9};
    m.armorClass = 16;
    m.hitPoints = 27;
    m.humanoid = true;
    m.experience = 200;
    m.attacks[0] = {0};
    m.attackCount = 1;
    m.loot[0] = {{8}, 1, 0, 1, 100}; // Dedicated fixture shield.
    m.lootCount = 1;
    return {{{"bugbear.morningstar", rules::AttackKind::Melee, 4, {2, 8, 2}, rules::DamageType::Piercing}},
            {m},
            f.races,
            f.classes,
            f.items,
            f.preset};
}
NpcTimings timing() {
    NpcTimings t{};
    t[0] = {{{600, 600}}, 2};
    t[2] = {{{700, 250, 250, 350}}, 4};
    t[3] = {{{150, 150}}, 2};
    t[4] = {{{200, 200, 200, 600}}, 4};
    return t;
}
void strikes() {
    const auto c = bugbearCatalog();
    const auto map = combatMap();
    const Player pose{{3, 3}, Heading::North};
    const NpcWorld placement(map, pose.cell, {"bugbear"}, {{{0}, {3, 2}, {0, 1}, Heading::South}});
    bool hit = false, critical = false, miss = false;
    // Stop after one of each observable outcome; this is a tiny deterministic
    // rules fixture, not a balance or seed-performance sweep.
    for (unsigned seed = 1; seed <= 128 && !(hit && critical && miss); ++seed) {
        Party party(c);
        for (unsigned i = 1; i < 4; ++i)
            party.damage({std::uint16_t(i)}, 10000); // Exactly one legal target, no target-selection RNG.
        ActorWorld actors(map, c, placement);
        rules::DiceRng rng(seed);
        NpcAiController ai(map, actors, seed);
        EffectController effects(party, actors, rng);
        CombatController combat(map, party, actors, rng, ai, effects, {timing()});
        CombatEvents events;
        combat.observe(pose);
        check(combat.contact(pose), "Bugbear uses ordinary melee contact");
        combat.update(pose, false, 0, events);
        if (!combat.snapshot().activeEnemy)
            continue;
        auto expectedRng = rng;
        const auto roll = expectedRng.d20().value;
        const auto expected = rules::resolveAttack(roll, 4, effects.armorClass(rules::CharacterId{0})).value;
        const bool crit = expected.outcome == rules::AttackOutcome::Critical;
        const bool lands = expected.outcome != rules::AttackOutcome::Miss;
        const auto damage = lands ? expectedRng.damage({2, 8, 2}, crit).value.total : 0;
        const auto hp = party.character({0})->hitPoints;
        const auto accepted = combat.update(pose, false, 450, events);
        check(accepted.event == CombatEvent::AttackStarted && accepted.impactMs == 950,
              "Heavy melee retains the registered 950 ms strike");
        check(accepted.damage == 0 && accepted.roll == 0 && party.character({0})->hitPoints == hp,
              "Accepted windup does not disclose saved rolls or apply early damage");
        events.clear();
        combat.resolveImpacts(pose, 1399, events);
        check(party.character({0})->hitPoints == hp, "Bugbear damage waits until impact");
        watching = true;
        allocations = 0;
        combat.resolveImpacts(pose, 1400, events);
        watching = false;
        check(allocations == 0, "Bugbear impact uses allocation-free engine storage");
        bool found = false;
        for (const auto& e : events)
            if (e.event == CombatEvent::Attack) {
                check(e.enemyAttack && e.roll == roll.selected && e.hit == lands && e.critical == crit &&
                          e.rawDamage == damage && e.damage == damage &&
                          e.damageType == rules::DamageType::Piercing,
                      "Bugbear resolves exactly 2d8+2, or 4d8+2 critical, with Brute included once");
                found = true;
            }
        check(found, "Bugbear publishes one resolved attack");
        check(party.character({0})->hitPoints == std::max<std::int64_t>(0, hp - damage),
              "Normal party health path receives the complete damage spike");
        hit |= lands && !crit;
        critical |= crit;
        miss |= !lands;
    }
    check(hit && critical && miss, "Fixed fixture reaches normal, critical and missed strikes");
}
void identityAndRewards() {
    const auto c = bugbearCatalog();
    const auto map = combatMap();
    const Player pose{{3, 3}, Heading::North};
    const NpcWorld placement(map, pose.cell, {"bugbear"},
                             {{{0}, {3, 2}, {0, 0}, Heading::South}, {{0}, {3, 2}, {0, 1}, Heading::South}});
    GameSession session(map, c, placement, 5, pose, false, {timing()});
    CommandController ui(session);
    check(ui.snapshot().enemyPortraits[0] == EnemyPortrait::Bugbear,
          "Initiative uses bugbear identity rather than a goblin portrait");
    check(!session.combat().exposed(session.actors().id(0)) &&
              session.combat().exposed(session.actors().id(1)),
          "Large-looking bugbear retains ordinary paired-subcell front protection");
    const auto front = session.actors().id(1);
    session.actors().damage(front, 27, 0);
    session.update(0, true);
    check(session.combat().exposed(session.actors().id(0)), "Front death exposes its paired bugbear");
    for (unsigned i = 0; i < 4; ++i)
        check(session.party().character({std::uint16_t(i)})->experience == 50,
              "One bugbear grants its 200 XP exactly once across the party");
    for (unsigned i = 0; i < 3; ++i)
        session.update(0, true);
    check(session.party().character({0})->experience == 50, "Repeated updates do not duplicate XP");
}
} // namespace
int main() {
    try {
        strikes();
        identityAndRewards();
        std::cout << "Bugbear checks passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}

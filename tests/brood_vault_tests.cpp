#include "generation/caves.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/progression_fixtures.hpp"
#include <algorithm>
#include <iostream>
using namespace blobber;
namespace {
constexpr Player entrance{{3, 3}, Heading::North};
std::vector<NpcInstance> placements() {
    std::vector<NpcInstance> result;
    for (const auto cell : {Cell{3, 2}, Cell{2, 2}, Cell{4, 2}, Cell{2, 3}, Cell{4, 3}}) {
        NpcInstance npc{{0}, cell, {0, 1}, Heading::South};
        npc.vault = VaultPart(result.size() + 1);
        if (npc.vault == VaultPart::WestSac)
            npc.subcell = {1, 0};
        result.push_back(npc);
    }
    return result;
}

void advance(GameSession& game, unsigned round) {
    for (int tick = 0; tick < 1500 && game.combat().snapshot().round < round; ++tick) {
        watching = true;
        game.update(.1, false);
        watching = false;
        const auto& turn = game.combat().snapshot();
        if (turn.activeCharacter && turn.awaitingAction) {
            auto c = turnCommand(game, PartyAction::Feature);
            c.feature = rules::Feature::EndTurn;
            check(game.submit(c).reason == CommandReason::Resolved, "Playable initiative opportunity");
        }
    }
    check(game.combat().snapshot().round >= round && allocations == 0,
          "Bounded, allocation-free hatch cycle");
}

void boundaries() {
    const auto map = combatMap();
    const auto catalog = spellCatalog();
    auto list = placements();
    // Occupy the western hatch with a stationary resident. Neither the resident nor the party can be
    // displaced.
    NpcInstance blocker{{0}, {2, 3}, {1, 0}};
    blocker.role = NpcRole::Resident;
    list.push_back(blocker);
    const NpcWorld npcs(map, entrance.cell, {"goblin"}, list);
    GameSession game(map, catalog, npcs, 5, entrance, false);
    auto& world = game.actors();
    const auto west = world.id(1), east = world.id(2);
    check(!world.id(3).world && !world.occupied({4, 3}), "Reserved young are dormant");
    auto shatter =
        spellArea(world, game.party(), game.effects(), entrance, rules::SpellKind::Shatter, Cell{2, 2});
    check(std::count_if(shatter.actors.begin(), shatter.actors.begin() + shatter.count,
                        [&](ActorId id) { return sameActor(id, west); }) == 1,
          "Shatter includes a covered sac once");
    const auto web =
        spellArea(world, game.party(), game.effects(), entrance, rules::SpellKind::Web, Cell{2, 2});
    check(std::none_of(web.actors.begin(), web.actors.begin() + web.count,
                       [&](ActorId id) { return sameActor(id, west); }),
          "Silk is not a Web spell creature");
    allocations = 0;
    advance(game, 2);
    check(game.combat().stirring(west) && game.combat().stirring(east), "Independent staggered warnings");
    advance(game, 4);
    check(!world.id(3).world && world.id(4).world && game.combat().stirring(west),
          "Blocked spawn retries; east hatches");
    const auto old = world.id(4);
    world.damage(old, 10000, game.timeMs());
    advance(game, 7);
    check(!world.actor(old) && world.id(4).generation == old.generation + 1 &&
              !world.damage(old, 1, game.timeMs()),
          "Replacement rejects every old actor handle");
    // Killing the mother leaves the existing eastern child in the objective.
    const auto child = world.id(4);
    world.damage(world.id(0), 10000, game.timeMs());
    game.update(.1, false);
    check(!world.id(3).world && !game.combat().stirring(west) && !game.objective().completed,
          "Guardian death silences both sacs and surviving young delay completion");
    world.damage(child, 10000, game.timeMs());
    game.update(.1, false);
    check(game.objective().completed && game.objective().rewardIssued,
          "Vault clears after last living young");
    const auto xp = game.party().character({0})->experience;
    game.update(.1, false);
    check(game.party().character({0})->experience == xp, "Clear reward commits once");
}

void lethalBoundary() {
    const auto map = combatMap();
    const auto catalog = spellCatalog();
    const NpcWorld npcs(map, entrance.cell, {"goblin"}, placements());
    for (unsigned target : {0u, 1u}) {
        GameSession game(map, catalog, npcs, 5, entrance, false);
        bool atBoundary = false;
        for (unsigned i = 0; i < 300 && !atBoundary; ++i) {
            game.update(.1, false);
            const auto& turn = game.combat().snapshot();
            if (turn.awaitingAction)
                game.submit(turnCommand(game));
            atBoundary = turn.phase == CombatPhase::RoundEnd && turn.round == 1;
        }
        check(atBoundary && game.combat().stirring(game.actors().id(1)), "West is due on next boundary");
        game.actors().damage(game.actors().id(target), 10000, game.timeMs());
        for (unsigned i = 0; i < 20; ++i)
            game.update(.1, false);
        check(!game.actors().id(3).world && !game.combat().stirring(game.actors().id(1)) &&
                  (target ? game.combat().snapshot().round >= 2 : game.objective().completed),
              "A lethal hit on the boundary wins over the due hatch");
    }
}
void capacity() {
    const auto catalog = spellCatalog();
    std::vector<NpcInstance> full(NpcWorld::maxInstances - 4);
    bool rejected = false;
    try {
        generation::placeBroodVault({}, {}, catalog, full);
    } catch (const std::invalid_argument& e) {
        rejected = std::string_view(e.what()).find("capacity") != std::string_view::npos;
    }
    check(rejected && full.size() == NpcWorld::maxInstances - 4, "Capacity failure leaves roster intact");
}

void shatterAndInterruptedBite() {
    const auto map = combatMap();
    const auto catalog = creationCatalog(true);
    const NpcWorld npcs(map, entrance.cell, {"goblin"}, placements());
    NpcTimings timing{};
    timing[0] = {{600}, 1};
    timing[static_cast<unsigned>(NpcAnimation::Attack)] = {{250, 250, 200, 150}, 4};
    GameSession game(map, catalog, npcs, 5, entrance, false, {timing});
    levelParty(game, 3, rules::SpellKind::Shatter);
    levelReady(game,3);
    auto cast = spellCommand(game, rules::SpellKind::Shatter);
    cast.aimedCell = Cell{3, 2};
    const auto sac = game.actors().id(1);
    const auto reason = game.submit(cast).reason;
    check(reason == CommandReason::Resolved, commandMessage(reason));
    unsigned hits = 0, saves = 0;
    for (const auto& e : game.events()) {
        if (!sameActor(e.enemy, sac))
            continue;
        hits += e.event == CombatEvent::Attack && e.damage > 0;
        saves += e.event == CombatEvent::SpellSave;
    }
    check(hits == 1 && !saves && game.actors().actor(sac)->hitPoints < 6,
          "Covered silk takes one object hit");
    GameSession bite(map, catalog, npcs, 5, entrance, false, {timing});
    CombatOutcome started;
    for (unsigned i = 0; i < 300 && !started.attackId.serial; ++i) {
        if (bite.combat().snapshot().awaitingAction)
            bite.submit(turnCommand(bite));
        for (const auto& e : bite.update(.1, false))
            if (e.event == CombatEvent::AttackStarted && sameActor(e.enemy, bite.actors().id(0)))
                started = e;
    }
    check(started.attackId.serial, "Mother starts bite windup");
    bite.actors().damage(started.enemy, 10000, bite.timeMs());
    bool canceled = false;
    for (unsigned i = 0; i < 15; ++i)
        for (const auto& e : bite.update(.1, false))
            if (e.attackId.serial == started.attackId.serial) {
                canceled |= e.event == CombatEvent::AttackCanceled;
                check(e.event != CombatEvent::Attack, "Interrupted bite cannot deal damage");
            }
    check(canceled, "Killing mother interrupts unreleased bite");
}

void releaseAndTargets() {
    const auto map = combatMap();
    const auto catalog = spellCatalog();
    const NpcWorld npcs(map, entrance.cell, {"goblin"}, placements());
    GameSession game(map, catalog, npcs, 5, entrance, false);
    spellReady(game, 0);
    auto melee = turnCommand(game, PartyAction::Melee);
    melee.target = game.actors().id(0);
    check(game.combat().validate(melee, entrance) == CommandReason::Ready,
          "Front fighter can reach the mother");
    melee.target = game.actors().id(1);
    check(game.combat().validate(melee, entrance) == CommandReason::AboveReach, "Sac overhead rejects sword");
    spellReady(game, 3);
    auto bolt = spellCommand(game, rules::SpellKind::FireBolt);
    bolt.targetCount = 1;
    bolt.enemies[0] = game.actors().id(1);
    check(game.submit(bolt).reason == CommandReason::Resolved, "Fire Bolt targets a warning sac");
    // Player projectiles release at acceptance. Destroy the recipient while the bolt is in flight.
    game.actors().damage(bolt.enemies[0], 6, game.timeMs());
    bool canceled = false;
    for (unsigned i = 0; i < 15; ++i)
        for (const auto& e : game.update(.1, false))
            canceled |= e.event == CombatEvent::Attack && e.targetGone && e.damage == 0;
    check(canceled && game.actors().actor(bolt.enemies[0])->hasDeath,
          "Released bolt cannot damage a destroyed sac");
    advance(game, 4);
    check(!game.actors().id(3).world && game.actors().id(4).world,
          "Lethal sac damage prevents pending hatch");
}
} // namespace
int main() try {
    boundaries();
    capacity();
    shatterAndInterruptedBite();
    lethalBoundary();
    releaseAndTargets();
    std::cout << "Vault lifecycle, selection and reward checks passed\n";
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}

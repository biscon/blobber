#include "cave_review.hpp"
#include "game/perception.hpp"
#include "game/session.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace blobber::cave_review {
using namespace generation;
namespace {
void require(bool condition, const char* reason) {
    if (!condition)
        throw std::runtime_error(reason);
}
void checkBullBite(const CaveFloor& floor, const rules::Catalog& catalog, const NpcInstance& bull,
                   const std::vector<std::string>& types, const std::vector<NpcTimings>& timings,
                   const FloorFixtures& fixtures, const Party::Saved& party, std::uint32_t seed) {
    const FloorGeometry geometry(floor.map, fixtures);
    for (auto h : {Heading::East, Heading::North, Heading::South, Heading::West}) {
        const Player stance{bull.cell + forward(h) * -1, h};
        if (!geometry.view().canStep(stance.cell, bull.cell))
            continue;
        const NpcWorld actors(floor.map, stance.cell, types, {bull});
        GameSession game(floor.map, std::make_unique<Party>(catalog, party), actors, seed, stance, true,
                         timings, {}, fixtures, {});
        rules::CharacterId warned;
        bool defended = false;
        for (unsigned tick = 0; tick < 600; ++tick) {
            for (const auto& event : game.update(.1, false)) {
                if (event.event == CombatEvent::BiteWarning)
                    warned = event.recipient;
                if (event.event == CombatEvent::Attack && event.enemyAttack) {
                    require(defended && event.guardedBite && event.recipient.index == warned.index &&
                                event.attackRollMode == rules::RollMode::Disadvantage,
                            "Bull bite lost its named defender response");
                    std::cout
                        << "Bull Cave Newt warned its defender and rolled the guarded bite at disadvantage\n";
                    return;
                }
            }
            const auto& turn = game.combat().snapshot();
            if (!turn.awaitingAction || !turn.activeCharacter)
                continue;
            PartyCommand cmd;
            cmd.world = turn.world;
            cmd.turn = turn.turn;
            cmd.character = *turn.activeCharacter;
            if (!defended && cmd.character.index == game.combat().warnedTarget(game.actors().id(0)).index) {
                cmd.action = PartyAction::Defend;
                defended = true;
            } else {
                cmd.action = PartyAction::Feature;
                cmd.feature = rules::Feature::EndTurn;
            }
            require(game.submit(cmd).reason == CommandReason::Resolved, "Bull review command rejected");
        }
        break;
    }
    throw std::runtime_error("Bull bite did not resolve");
}
} // namespace
// Keep the production positions, terrain and closed fixtures. Isolate the group so
// waiting through several unanswered attacks cannot recruit an unrelated encounter.
void checkPressure(const CaveFloor& floor, const rules::Catalog& catalog, const NpcWorld& all,
                   const std::vector<std::string>& types, const std::vector<NpcTimings>& timings,
                   std::uint32_t seed) {
    const auto& f = floor.layout;
    const bool scouts = f.depth == 5;
    const auto recipe = scouts ? EncounterRecipe::GalleryScouts : EncounterRecipe::VaultApproach;
    const auto group = std::find_if(f.encounters.begin(), f.encounters.begin() + f.encounterCount,
                                    [&](const Encounter& e) { return e.recipe == recipe; });
    require(group != f.encounters.begin() + f.encounterCount, "Missing pressure encounter");
    const auto& placements = all.instances();
    const std::vector<NpcInstance> members(placements.begin() + group->first,
                                           placements.begin() + group->first + group->count);
    const auto& north = f.connections[5];
    const Cell mouth = scouts ? f.connections[2].socketB : north.socketA + Cell{-3, 1};
    const NpcWorld actors(floor.map, mouth, types, members);
    auto fixtures = f.fixtures;
    // The isolated encounter owns neither the guardian roster nor its reward gate.
    fixtures.entries[f.guardianGate] = fixtures.entries[--fixtures.count];
    const auto party = populationParty(catalog, f.depth, false);
    if (!scouts) {
        const auto bull = std::find_if(placements.begin(), placements.end(), [&](const NpcInstance& npc) {
            return npc.monster && npc.monster->index == catalog.findMonster("bull.cave.newt").index;
        });
        require(bull != placements.end(), "Vault pack lacks its bull");
        checkBullBite(floor, catalog, *bull, types, timings, fixtures, party, seed);
    }
    for (unsigned stance = 0; stance < (scouts ? 3u : 1u); ++stance) {
        GameSession game(floor.map, std::make_unique<Party>(catalog, party), actors, seed,
                         {mouth, Heading::East}, true, timings, {}, fixtures, {});
        game.setAnimated(true);
        if (scouts) {
            game.discover();
            require(bool(game.interact(game.nearbyFixture())), "Scouts door cannot open from outside");
            game.update(.3, false);
            if (stance == 1) {
                game.move(Action::Back);
                game.update(.18, false);
            } else if (stance == 2) {
                game.move(Action::Forward);
                game.update(.18, false);
                game.move(Action::Right);
                game.update(.18, false);
            }
            const Cell expected = mouth + (stance == 1 ? Cell{-1, 0} : stance == 2 ? Cell{1, 1} : Cell{});
            require(game.movement().player.cell == expected, "Cannot reach scouting stance before contact");
            unsigned rays = 0;
            for (unsigned n = 2; n < 4; ++n)
                rays += inBowRange(expected, members[n].cell) &&
                        lineOfSight(game.geometry(), expected, members[n].cell);
            require(rays == (stance == 2 ? 1u : 2u), "Wrong number of scout firing lines");
        }
        std::bitset<NpcWorld::maxInstances> attackers, stationaryArchers;
        bool northLane = false, southLane = false;
        std::bitset<NpcWorld::maxInstances> crossedNorth, crossedSouth;
        std::bitset<maxCells> contactCells;
        for (unsigned tick = 0; tick < 1200; ++tick) {
            const auto& events = game.update(.1, false);
            if (!scouts)
                for (unsigned i = 0; i < group->count; ++i) {
                    const auto c = game.actors().snapshot().actors[i].npc.cell;
                    if (c == north.socketA)
                        crossedNorth.set(i);
                    if (c == north.socketA + Cell{0, 2})
                        crossedSouth.set(i);
                }
            for (const auto& e : events) {
                if (!e.enemyAttack)
                    continue;
                if (scouts && e.event == CombatEvent::AttackStarted) {
                    if (e.kind == rules::AttackKind::Ranged && e.enemy.slot >= 2) {
                        attackers.set(e.enemy.slot);
                        if (game.actors().snapshot().actors[e.enemy.slot].npc.cell ==
                            members[e.enemy.slot].cell)
                            stationaryArchers.set(e.enemy.slot);
                    }
                } else if (!scouts && e.event == CombatEvent::Attack) {
                    const auto cell = game.actors().snapshot().actors[e.enemy.slot].npc.cell;
                    northLane |= crossedNorth[e.enemy.slot];
                    southLane |= crossedSouth[e.enemy.slot];
                    contactCells.set(unsigned(cell.y * f.width + cell.x));
                    attackers.set(e.enemy.slot);
                }
            }
            const auto& turn = game.combat().snapshot();
            if ((scouts && (stance == 2 ? turn.round >= 3 : stationaryArchers.count() == 2)) ||
                (northLane && southLane && contactCells.count() >= 2))
                break;
            if (turn.awaitingAction && turn.activeCharacter) {
                PartyCommand cmd;
                cmd.world = turn.world;
                cmd.turn = turn.turn;
                cmd.character = *turn.activeCharacter;
                cmd.action = PartyAction::Feature;
                cmd.feature = rules::Feature::EndTurn;
                require(game.submit(cmd).reason == CommandReason::Resolved,
                        "Cannot pass pressure review turn");
            }
        }
        std::cout << encounterRecipe(recipe).name << " at " << game.movement().player.cell.x << ','
                  << game.movement().player.cell.y << ": " << attackers.count()
                  << (scouts ? " archers fired" : " distinct melee attackers");
        if (scouts)
            std::cout << ", " << stationaryArchers.count() << " from their opening positions";
        if (!scouts)
            std::cout << ", north/south " << northLane << '/' << southLane << ", " << contactCells.count()
                      << " occupied attack cells";
        std::cout << '\n';
        require(scouts ? stationaryArchers.count() == (stance == 2 ? 1u : 2u)
                       : northLane && southLane && contactCells.count() >= 2,
                "Expected independent attacks did not resolve");
    }
}
} // namespace blobber::cave_review

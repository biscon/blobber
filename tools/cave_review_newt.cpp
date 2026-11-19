#include "cave_review.hpp"
#include "game/perception.hpp"
#include "platform/game_view.hpp"
#include "rlgl.h"
#include "rules/creation.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace blobber::cave_review {
using Site = generation::CaveSite;
namespace {
void require(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
} // namespace

void placeGalleries(const generation::CaveSites& sites, const std::vector<std::string>& definitions,
                    std::vector<NpcInstance>& actors) {
    auto guard = [&](const char* species, Cell cell, Cell subcell, Heading facing) {
        const auto it = std::find(definitions.begin(), definitions.end(), species);
        require(it != definitions.end(), "Missing Galleries creature");
        actors.push_back({{std::uint16_t(it - definitions.begin())}, cell, subcell, facing});
    };
    guard("blind.cave.newt", sites[Site::SoloNewt], {0, 1}, Heading::South);
    guard("venomous.spider", sites[Site::NorthContact] + Cell{2, 1}, {0, 0}, Heading::East);
    guard("blind.cave.newt", sites[Site::SouthContact] + Cell{2, -1}, {0, 1}, Heading::West);
}

void equipMeleeProofParty(rules::PresetParty& party) {
    // This extra headless walk checks the approach with no rear-row ranged option.
    const auto fighter = party.characters.front();
    const char* portraits[]{"alden", "dwarf.fighter", "halfling.fighter", "human.adventurer"};
    for (unsigned i = 0; i < party.characters.size(); ++i) {
        const auto identity = party.characters[i];
        auto& member = party.characters[i];
        member = fighter;
        member.id = identity.id;
        member.name = identity.name;
        member.race = i == 3 ? fighter.race : identity.race;
        member.portrait = rules::findPortrait(portraits[i]);
    }
}

void checkGalleries(GameSession& game, const generation::CaveFloor& floor,
                    const std::vector<NpcTimings>& timings, bool north, bool meleeOnly, const std::filesystem::path& output,
                    WorldRenderer* renderer, Presenter* presenter, bool soloOnly) {
    CommandController controls(game, timings);
    NpcAnimator animator(game.actors(), timings, &game.combat());
    std::optional<GameView> view;
    if (renderer)
        view.emplace(*renderer, &floor.layout);
    TargetSnapshot targets;
    auto refresh = [&] {
        if (view) {
            animator.setTime(game.timeMs(), &game.effects());
            view->update(game, animator.snapshot(), {});
            targets = view->picking()->targets();
        } else {
            targets = {};
            const auto& actors = game.actors().snapshot();
            targets.world = actors.world;
            targets.revision = actors.revision;
            const auto pose = game.movement().player;
            for (unsigned i = 0; i < actors.count; ++i) {
                const auto& a = actors.actors[i];
                if (a.life == ActorLife::Alive &&
                    inPartyView(game.geometry(), pose.cell, pose.heading, a.npc.cell, a.npc.subcell)) {
                    targets.ids[targets.count++] = game.actors().id(i);
                    targets.candidates[targets.candidateCount++] = game.actors().id(i);
                }
            }
        }
        controls.refresh(targets, game.actionBusy(), false, true);
    };
    auto photo = [&](const char* name) {
        if (!view)
            return;
        view->present(*presenter, game, floor.map, false, true, &controls.snapshot());
        presenter->capture(output / name);
        const auto error = reinterpret_cast<unsigned (*)()>(rlGetProcAddress("glGetError"));
        require(error && error() == 0, "OpenGL error during Galleries review");
    };
    const auto fork = floor.sites[Site::Fork];
    const bool answerWarning = soloOnly ? north : floor.layout.encounterCount == 0;
    bool warning = false, defended = false, impact = false, firstAttack = false, together = false;
    bool pairVisible = false;
    const auto& catalog = game.party().catalog();
    bool firedBow = false, castBolt = false;
    if (view && north) {
        // Art inspection only. No gameplay tick or roll elapses before the threshold walk.
        const auto solo = floor.sites[Site::SoloNewt];
        const char* files[]{"idle-front.png", "idle-side.png", "idle-back.png", "idle-other-side.png"};
        for (unsigned i = 0; i < 4; ++i) {
            const auto heading = Heading(i);
            bool pickable = false;
            for (int distance : {1, 2, 3}) {
                const auto cell = solo + forward(heading) * -distance;
                if (floor.map.movementBlocked(cell) || game.actors().occupied(cell))
                    continue;
                game.restorePose({cell, heading}, false);
                refresh();
                pickable = std::any_of(targets.ids.begin(), targets.ids.begin() + targets.count,
                                       [](ActorId id) { return id.slot == 0; });
                if (pickable)
                    break;
            }
            require(pickable, "Newt idle view must expose body pixels");
            photo(files[i]);
        }
        game.restorePose(floor.layout.spawn, true);
    }

    auto fight = [&](bool mixed) {
        for (unsigned tick = 0; tick < 4000; ++tick) {
            const auto& events = game.update(.05, false);
            controls.record(events);
            const auto& turn = game.combat().snapshot();
            if (mixed)
                together |= turn.enemyCount >= 2;
            bool warningNow = false, impactNow = false;
            for (const auto& event : events) {
                if (!mixed && !impact && event.event == CombatEvent::BiteWarning)
                    defended = false;
                warningNow |= !mixed && !warning && event.event == CombatEvent::BiteWarning;
                if (!mixed && !impact && event.event == CombatEvent::Attack && event.enemyAttack) {
                    const auto* victim = game.party().character(event.recipient);
                    if (soloOnly && victim && event.hit)
                        std::cout << victim->build.name << " AC " << victim->stats.armorClass << ", "
                                  << victim->stats.maxHitPoints << " max HP, "
                                  << (answerWarning ? "Defend" : "End Turn") << ", roll " << event.roll
                                  << ", hit " << event.hit << ", damage " << event.damage << ", remaining HP "
                                  << victim->hitPoints << '\n';
                    require(!answerWarning ||
                                (defended && event.attackRollMode == rules::RollMode::Disadvantage),
                            "Teaching bite failed to honor Defend");
                    impactNow = !soloOnly || event.hit;
                }
            }
            warning |= warningNow;
            impact |= impactNow;
            if (north && impactNow) {
                refresh();
                photo("impact.png");
            }
            if (turn.phase == CombatPhase::Victory || (soloOnly && impact))
                return;
            require(turn.phase != CombatPhase::Defeat, "Galleries party defeated");
            if (!turn.awaitingAction || !turn.activeCharacter)
                continue;
            refresh();
            PartyCommand c;
            c.world = turn.world;
            c.turn = turn.turn;
            c.character = *turn.activeCharacter;
            if (!mixed && !impact) {
                if (answerWarning && !defended &&
                    game.combat().warnedTarget(game.actors().id(0)).index == c.character.index) {
                    if (north && view) {
                        const auto pose = animator.snapshot().frames[0];
                        require(pose.clip == NpcAnimation::Attack && pose.active <= 1, "Missing planted bite warning");
                        photo(pose.active ? "raised-jaw.png" : "brace.png");
                        const auto turnId = turn.turn;
                        controls.record(game.update(.48, false));
                        refresh();
                        const auto breath = animator.snapshot().frames[0];
                        require(turn.turn == turnId && turn.awaitingAction &&
                                    breath.clip == pose.clip && breath.active == 1 - pose.active,
                                "Warning must breathe while the named defender chooses");
                        photo(breath.active ? "raised-jaw.png" : "brace.png");
                    }
                    c.action = PartyAction::Defend;
                    require(controls.submit(c) == CommandReason::Resolved, "Named defender rejected");
                    defended = true;
                } else
                    controls.endTurn();
                continue;
            }
            if (mixed && !north && firstAttack && pairVisible && targets.count && !meleeOnly && (!firedBow || !castBolt)) {
                const auto target = targets.ids[0];
                if (c.character.index == 2 && !firedBow) {
                    c.action = PartyAction::Ranged;
                    c.target = target;
                    require(controls.submit(c) == CommandReason::Resolved, "Rear rogue's shortbow rejected");
                    firedBow = true;
                } else if (c.character.index == 3 && !castBolt) {
                    c.action = PartyAction::Spell;
                    c.spell = catalog.findSpell("fire_bolt");
                    c.enemies[0] = target;
                    c.targetCount = 1;
                    require(controls.submit(c) == CommandReason::Resolved, "Rear wizard's Fire Bolt rejected");
                    castBolt = true;
                } else {
                    controls.endTurn();
                }
                continue;
            }
            if (mixed && targets.count >= 2 && !pairVisible) {
                pairVisible = true;
                if (!north)
                    photo("both-species.png");
            }
            if (mixed && !north && firstAttack && !pairVisible && turn.round <= 3) {
                c.action = PartyAction::Defend;
                if (controls.validate(c) == CommandReason::Ready)
                    controls.submit(c);
                else
                    controls.endTurn();
                continue;
            }
            c.action = PartyAction::Melee;
            unsigned count = 0;
            ActorId first;
            for (std::size_t i = 0; i < targets.count; ++i) {
                c.target = targets.ids[i];
                if (controls.validate(c) == CommandReason::Ready) {
                    if (!count)
                        first = *c.target;
                    ++count;
                }
            }
            if (!count) {
                controls.endTurn();
                continue;
            }
            if (mixed && !firstAttack) {
                const auto pose = game.movement().player;
                std::cout << (north ? "North" : "South") << " bank from (" << pose.cell.x << ","
                          << pose.cell.y << ") heading " << unsigned(pose.heading) << ", first melee target "
                          << game.actors().appearanceId(game.actors().actor(first)->npc.type)
                          << ", legal targets " << count << '\n';
                require(first.slot == (north ? 1 : 2) && count == 1, "Wrong first melee target on bank");
                photo(north ? "north-first.png" : "south-first.png");
                firstAttack = true;
            }
            c.target = first;
            require(controls.submit(c) == CommandReason::Resolved, "Visible melee target rejected");
        }
        throw std::runtime_error("Galleries encounter timed out");
    };

    game.setAnimated(true);
    walkTo(game, floor.sites[Site::SoloNewt], {}, nullptr, true);
    fight(false);
    if (soloOnly) {
        require(warning && impact && (!answerWarning || defended), "Missing equipped-party bite comparison");
        return;
    }
    require(warning && (defended || !answerWarning) && impact && game.combat().continueExploring(),
            "Solo lesson incomplete");
    walkTo(game, fork);
    for (unsigned tick = 0; tick < 60; ++tick)
        game.update(.1, false);
    require(game.combat().snapshot().phase == CombatPhase::Exploration, "No safe decision at the fork");
    if (north) {
        refresh();
        photo("fork.png");
    }
    const auto shoulder = fork + Cell{0, north ? -2 : 2};
    walkTo(game, shoulder, {}, nullptr, true);
    if (realTime(game.combat().snapshot().phase))
        walkTo(game, floor.sites[Site::FarBank] + Cell{0, north ? -2 : 2}, {}, nullptr, true);
    fight(true);
    if (!north)
        require(pairVisible, "Both living species must be visible in the south fight");
    require(north || meleeOnly || (firedBow && castBolt), "Both rear characters must contribute from their starting slots");
    require(firstAttack && together, "Both species must participate in the mixed encounter");
}
} // namespace blobber::cave_review

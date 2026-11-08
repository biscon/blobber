#pragma once
#include "assets/rules_loader.hpp"
#include "game/perception.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include <iostream>

namespace blobber::test {
// Opt-in visual integration: owned safe mechanics, production goblin artwork.
inline void combatPositioningReview(const std::filesystem::path& assets,
                                    const std::filesystem::path& output) {
    if (std::filesystem::exists(output) && !std::filesystem::is_empty(output))
        throw std::runtime_error("Choose a new or empty combat positioning review directory");
    std::filesystem::create_directories(output);
    const auto definitions = loadNpcDefinitions(assets / "npcs/level.json");
    check(!definitions.types.empty() && definitions.types.front() == "goblin", "Production goblin binding");
    const auto fixture = combatCatalog(1000, -100, {1, 4, -100});
    auto party = partyFixture();
    std::vector<rules::MonsterDefinition> monsters;
    for (const auto& type : definitions.types) {
        auto monster = *fixture.monster({0});
        monster.id = type;
        monsters.push_back(std::move(monster));
    }
    const rules::Catalog catalog({*fixture.attack({0})}, std::move(monsters), party.races, party.classes,
                                 party.items, party.preset);
    WorldDefinition world;
    world.assetPack = "underkeep";
    std::vector<std::string> rows(13, std::string(13, '.'));
    rows.front() = rows.back() = std::string(13, '#');
    for (auto& row : rows)
        row.front() = row.back() = '#';
    const Map map(rows, world);
    const Player pose{{6, 6}, Heading::North};
    NpcWorld places(map, pose.cell, definitions.types,
                    {{{0}, {6, 5}, {0, 1}, Heading::South},
                     {{0}, {6, 5}, {1, 1}, Heading::South},
                     {{0}, {5, 6}, {1, 1}, Heading::East},
                     {{0}, {7, 6}, {0, 1}, Heading::West}});
    GameSession game(map, catalog, places, 5, pose, false, definitions.timings);
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Positioning review stays hidden");
    GpuResources resources;
    {
        Presenter presenter(resources, assets);
        const auto baseline = resources.counts();
        {
            WorldRenderer renderer(resources, world_assets::load(assets / "prepared/world"));
            GameView view(renderer);
            NpcAnimator animator(game.actors(), definitions.timings);
            CommandController commands(game, definitions.timings);
            auto capture = [&](const std::string& name) {
                animator.setTime(game.timeMs(), &game.effects());
                view.update(game, animator.snapshot(), {});
                commands.refresh(view.picking()->targets(), false, false, true);
                view.present(presenter, game, map, false, true, &commands.snapshot());
                presenter.capture(output / (name + ".png"));
                using Error = unsigned (*)();
                check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
                      "Combat positioning render has no GL errors");
            };
            capture("initial-two-visible-two-flanking");
            std::array<bool, 4> moved{}, attacked{}, captured{};
            std::array<std::uint64_t, 4> movedRound{}, movedTurn{};
            for (unsigned tick = 0; tick < 500 && !(attacked[2] && attacked[3]); ++tick) {
                if (game.combat().snapshot().awaitingAction)
                    commands.record(game.submit(turnCommand(game)));
                else {
                    const auto& events = game.update(.1f, false);
                    commands.record(events);
                    const auto step = events.step;
                    if (step.enemy.slot >= 4)
                        continue;
                    if (step.event == CombatEvent::Move) {
                        check(step.enemy.slot >= 2, "Attack-ready centre goblins hold their positions");
                        movedRound[step.enemy.slot] = game.combat().snapshot().round;
                        movedTurn[step.enemy.slot] = game.combat().snapshot().turn;
                        moved[step.enemy.slot] = true;
                        const auto& arriving = game.actors().actor(step.enemy)->npc;
                        if (!captured[step.enemy.slot] &&
                            inPartyView(map, pose.cell, pose.heading, arriving.cell, arriving.subcell)) {
                            captured[step.enemy.slot] = true;
                            commands.record(game.update(.15, false));
                            capture("flank-" + std::to_string(step.enemy.slot) + "-walking");
                            commands.record(game.update(.15, false));
                            capture("flank-" + std::to_string(step.enemy.slot) + "-arrived");
                        }
                    }
                    if (step.event == CombatEvent::AttackStarted) {
                        const auto& actor = *game.actors().actor(step.enemy);
                        check(inPartyView(map, pose.cell, pose.heading, actor.npc.cell, actor.npc.subcell),
                              "Every attack in the scripted encounter is frontal");
                        attacked[step.enemy.slot] = true;
                        if (step.enemy.slot >= 2)
                            check(movedRound[step.enemy.slot] == game.combat().snapshot().round &&
                                      movedTurn[step.enemy.slot] == game.combat().snapshot().turn,
                                  "Flanker movement and attack retain the same round and turn");
                    }
                }
                check(game.movement().player == pose,
                      "Advancing flank actors keep the populated front facing");
            }
            check(moved[2] && moved[3] && attacked[2] && attacked[3],
                  "Both formerly offscreen goblins move and attack in the same initiative turns");
            for (unsigned i = 0; i < 100 && !game.combat().snapshot().awaitingAction; ++i)
                commands.record(game.update(.1f, false));
            capture("all-four-engaged");
            check(view.picking() && view.picking()->targets().count == 4,
                  "Final review frame has four actually picked enemy bodies");
        }
        const auto released = resources.counts();
        check(released.textures == baseline.textures && released.targets == baseline.targets &&
                  released.meshes == baseline.meshes && released.shaders == baseline.shaders,
              "Positioning review releases world GPU resources");
    }
    std::cout << "Hidden combat positioning review passed; both flanks joined the fight\n";
}
} // namespace blobber::test

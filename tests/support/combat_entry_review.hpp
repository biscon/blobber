#pragma once
#include "assets/rules_loader.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "support/hybrid_fixtures.hpp"
#include <iostream>
namespace blobber::test {
// Opt-in visual integration diagnostic; mechanical regressions use owned CPU fixtures.
inline void combatEntryReview(const std::filesystem::path& assets, const std::filesystem::path& output) {
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Choose a new combat entry review directory");
    const auto definitions = loadNpcDefinitions(assets / "npcs/level.json");
    check(!definitions.types.empty() && definitions.types.front() == "goblin", "Review goblin appearance");
    const auto& timings = definitions.timings;
    const auto fixture = hybridCatalog(false);
    auto party = partyFixture();
    std::vector<rules::MonsterDefinition> monsters;
    for (const auto& type : definitions.types) {
        auto monster = *fixture.monster({0});
        monster.id = type;
        monsters.push_back(std::move(monster));
    }
    const rules::Catalog catalog({*fixture.attack({0}), *fixture.attack({1})}, std::move(monsters),
                                 party.races, party.classes, party.items, party.preset);
    WorldDefinition world;
    world.assetPack = "underkeep";
    std::vector<std::string> rows(13, std::string(13, '.'));
    rows.front() = rows.back() = std::string(13, '#');
    for (auto& row : rows)
        row.front() = row.back() = '#';
    const Map map(rows, world);
    Window window(1920, 1080, true);
    GpuResources resources;
    {
        Presenter presenter(resources, assets);
        const auto baseline = resources.counts();
        {
            WorldRenderer renderer(resources, world_assets::load(assets / "prepared/world"));
            auto capture = [&](GameSession& game, const char* name, bool needsMelee) {
                NpcAnimator animator(game.actors(), timings);
                animator.setTime(game.timeMs(), &game.effects());
                GameView view(renderer);
                view.update(game, animator.snapshot(), {});
                CommandController commands(game, timings);
                commands.refresh(view.picking()->targets(), false, false, true);
                if (needsMelee) {
                    bool reachable = false;
                    const auto& targets = view.picking()->targets();
                    for (unsigned i = 0; i < targets.count; ++i) {
                        auto command = turnCommand(game, PartyAction::Melee);
                        command.target = targets.ids[i];
                        reachable |=
                            game.combat().validate(command, game.movement().player) == CommandReason::Ready;
                    }
                    check(reachable, "Rendered, pickable exposed melee target at fighter decision");
                }
                view.present(presenter, game, map, false, true, &commands.snapshot());
                presenter.capture(output / (std::string(name) + ".png"));
                using Error = unsigned (*)();
                check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0, "Combat entry GL draw");
            };
            auto fighter = [](GameSession& game) {
                for (unsigned i = 0; i < 150; ++i) {
                    const auto& state = game.combat().snapshot();
                    if (state.awaitingAction && state.activeCharacter->index == 0)
                        return;
                    if (state.awaitingAction)
                        game.submit(turnCommand(game));
                    else
                        game.update(.16, false);
                }
                throw std::runtime_error("Review did not reach fighter decision");
            };
            const Player pose{{6, 6}, Heading::North};
            {
                NpcWorld placements(
                    map, pose.cell, definitions.types,
                    {{{0}, {7, 5}, {1, 1}, Heading::South}, {{0}, {7, 5}, {1, 0}, Heading::South}});
                GameSession game(map, catalog, placements, 5, pose, false, timings);
                fighter(game);
                check(game.movement().player.heading == Heading::East, "Entry faces hidden protector");
                capture(game, "entry-faces-exposed-front", true);
            }
            {
                NpcWorld placements(
                    map, pose.cell, definitions.types,
                    {{{0}, {6, 5}, {0, 1}, Heading::South}, {{0}, {7, 4}, {1, 0}, Heading::South}});
                GameSession game(map, catalog, placements, 5, pose, true, timings);
                for (unsigned i = 0; i < 150; ++i) {
                    const auto* approaching = game.actors().actor(game.actors().id(1));
                    if (approaching->npc.cell == Cell{6, 5} && !actorWalking(*approaching, game.timeMs()))
                        break;
                    if (game.combat().snapshot().awaitingAction)
                        game.submit(turnCommand(game));
                    else
                        game.update(.16, false);
                }
                const auto* arrived = game.actors().actor(game.actors().id(1));
                check(arrived->npc.cell == Cell{6, 5} && arrived->npc.subcell.y == 1,
                      "Turn-based approach fills a near-row subslot");
                fighter(game);
                capture(game, "turn-based-near-row-arrival", true);
            }
        }
        const auto released = resources.counts();
        check(released.textures == baseline.textures && released.targets == baseline.targets &&
                  released.meshes == baseline.meshes && released.shaders == baseline.shaders,
              "Combat entry review releases world resources");
    }
    std::cout << "Combat entry render/picking review passed\n";
}
} // namespace blobber::test

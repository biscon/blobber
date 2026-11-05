#pragma once
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
namespace blobber::test {
// Production-art review uses real accepted commands and in-process session time only.
inline void meleeSwipeReview(const std::filesystem::path& assets, const std::filesystem::path& output) {
    std::filesystem::create_directories(output);
    const auto definitions = loadNpcDefinitions(assets / "npcs/level.json");
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Swipe review remains hidden");
    GpuResources resources;
    Presenter presenter(resources, assets);
    const auto baseline = resources.counts();
    for (bool miss : {false, true}) {
        const auto fixture = combatCatalog(1000, -100, {1, 4, -100}, miss ? 30 : 1);
        auto party = partyFixture();
        std::vector<rules::MonsterDefinition> monsters;
        for (const auto& type : definitions.types) {
            auto monster = *fixture.monster({0});
            monster.id = type;
            monsters.push_back(monster);
        }
        const rules::Catalog catalog({*fixture.attack({0})}, std::move(monsters), party.races, party.classes,
                                     party.items, party.preset);
        WorldDefinition definition;
        definition.assetPack = "underkeep";
        const Map map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"},
                      definition);
        const Player pose{{3, 3}, Heading::North};
        NpcWorld places(map, pose.cell, definitions.types, {{{0}, {3, 2}, {0, 1}, Heading::South}});
        GameSession game(map, catalog, places, 5, pose, false, definitions.timings);
        WorldRenderer renderer(resources, world_assets::load(assets / "prepared/world"));
        GameView view(renderer);
        NpcAnimator animator(game.actors(), definitions.timings);
        CommandController commands(game, definitions.timings);
        auto present = [&] {
            animator.setTime(game.timeMs(), &game.effects());
            view.update(game, animator.snapshot(), {});
            commands.refresh(view.picking()->targets(), false, false, true);
            view.present(presenter, game, map, false, true, &commands.snapshot());
        };
        commands.record(game.update(0, false));
        for (unsigned i = 0; i < 200; ++i) {
            const auto& state = game.combat().snapshot();
            if (state.awaitingAction && state.activeCharacter->index == 0)
                break;
            if (state.awaitingAction)
                commands.record(game.submit(turnCommand(game)));
            else
                commands.record(game.update(.1, false));
        }
        present();
        auto command = turnCommand(game, PartyAction::Melee);
        command.target = game.actors().id(0);
        const auto accepted = game.submit(command);
        check(accepted.event == CombatEvent::AttackStarted, "Review uses accepted melee command");
        commands.record(accepted);
        const auto start = game.timeMs();
        const auto& attack = game.combat().attacks().attacks[NpcWorld::maxInstances];
        check(attack.impactMs == start + 200 && attack.hit != miss, "Review deadline and outcome");
        present(); // Capture geometry on acceptance, before any time elapses.
        unsigned impacts = 0;
        for (unsigned offset : {134u, 170u, 200u, 290u, 340u}) {
            const auto& events = game.update(double(start + offset - game.timeMs()) / 1000, false);
            for (const auto& event : events)
                if (event.event == CombatEvent::Attack && event.attackId.serial == accepted.attackId.serial)
                    ++impacts;
            commands.record(events);
            present();
            if (!miss || offset == 170)
                presenter.capture(output /
                                  (std::string(miss ? "miss-" : "hit-") + std::to_string(offset) + ".png"));
            using Error = unsigned (*)();
            check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
                  "Swipe review GL state valid");
        }
        check(impacts == 1, "Exactly one authoritative melee impact across swipe phases");
    }
    const auto released = resources.counts();
    check(released.textures == baseline.textures && released.targets == baseline.targets &&
              released.meshes == baseline.meshes && released.shaders == baseline.shaders,
          "Swipe review releases renderer resources");
}
} // namespace blobber::test

#pragma once
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
namespace blobber::test {
// Optional visual evidence: isolated preset previews over a real prepared dungeon.
// Production event wiring is verified separately with owned CPU fixtures.
inline void screenShakeReview(const std::filesystem::path& assets, const std::filesystem::path& output) {
    std::filesystem::create_directories(output);
    const auto definitions = loadNpcDefinitions(assets / "npcs/level.json");
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Shake review window remains hidden");
    GpuResources resources;
    Presenter presenter(resources, assets);
    const auto baseline = resources.counts();
    {
        const auto fixture = combatCatalog(1000, -100, {1, 4, -100}, 1);
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
        animator.setTime(game.timeMs());
        view.update(game, animator.snapshot(), {});
        view.activate();
        commands.refresh(view.picking()->targets(), false, false, true);
        auto hud = commands.snapshot();
        const auto world = game.actors().snapshot().world;
        const auto image = renderer.image();
        auto capture = [&](const char* name, WorldViewTransform transform) {
            hud.worldTransform = transform;
            presenter.presentWorld(image.target, image.revision, game.movement(), map, &hud, view.picking());
            check(presenter.displayedWorldTransform().x == transform.x &&
                      presenter.displayedWorldTransform().zoom == transform.zoom,
                  "Presenter retains the actual displayed transform for input");
            presenter.capture(output / name);
        };
        capture("rest.png", {});
        for (unsigned preset = 0; preset < 3; ++preset) {
            ScreenShakePlayer player;
            CombatOutcome out;
            out.event = preset == 2 ? CombatEvent::Cast : CombatEvent::Attack;
            out.spell = preset == 2 ? rules::SpellKind::Shatter : rules::SpellKind::Count;
            out.enemyAttack = preset == 0;
            out.critical = preset == 1;
            out.hit = true;
            out.damage = 5;
            check(player.record(out, 0, world), "Preset review uses production event classifier");
            capture(preset == 0   ? "little-25ms.png"
                    : preset == 1 ? "medium-25ms.png"
                                  : "big-25ms.png",
                    player.sample(25, world));
            if (preset == 2) {
                capture("big-80ms.png", player.sample(80, world));
                capture("expired-360ms.png", player.sample(360, world));
            }
        }
    }
    const auto released = resources.counts();
    check(released.textures == baseline.textures && released.targets == baseline.targets &&
              released.meshes == baseline.meshes && released.shaders == baseline.shaders,
          "Shake review releases view GPU resources");
}
} // namespace blobber::test

#pragma once
#include "app/loading.hpp"
#include "assets/rules_loader.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/allocations.hpp"
#include "support/check.hpp"
namespace blobber::test {
// Explicit static presentation review, outside routine tests and balance studies.
inline void pc072Review(const std::filesystem::path& source, const std::filesystem::path& output) {
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Review output must be a new directory");
    const auto catalog = loadRules(source / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(source / "npcs/level.json");
    Window window(1920, 1080, true);
    PreparationWorker worker;
    GpuResources resources;
    Presenter presenter(resources, source, &worker);
    WorldRenderer renderer(resources, world_assets::load(source / "prepared/world"));
    const auto type = [&](const char* name) {
        for (unsigned i = 0; i < definitions.types.size(); ++i)
            if (definitions.types[i] == name)
                return NpcTypeId{std::uint16_t(i)};
        throw std::runtime_error("Missing review species");
    };
    const Map map({"#########", "#.......#", "#.......#", "#....####", "#....####", "#....####", "#.......#",
                   "#.......#", "#########"});
    const Player spawn{{4, 6}, Heading::North};
    NpcWorld placements(map, spawn.cell, definitions.types,
                        {{type("goblin"), {4, 4}, {0, 1}, Heading::South},
                         {type("bugbear"), {4, 4}, {1, 1}, Heading::South},
                         {type("goblin_shaman"), {4, 4}, {0, 0}, Heading::South}});
    GameSession game(map, catalog, placements, 5, spawn, false, definitions.timings);
    NpcAnimator animation(game.actors(), definitions.timings);
    animation.setTime(0);
    GameView view(renderer);
    view.update(game, animation.snapshot(), {});
    CommandController commands(game, definitions.timings);
    commands.refresh(view.picking()->targets(), false, false, true);
    auto hud = commands.snapshot();
    // Fixed presentation-only notices model the crowded screenshot without
    // inventing combat decisions, RNG outcomes or a new expedition.
    hud.combat.phase = CombatPhase::Turns;
    hud.combat.upcomingCount = 6;
    for (unsigned i = 0; i < 6; ++i)
        hud.combat.upcoming[i].character = rules::CharacterId{std::uint16_t(i % 4)};
    hud.conditions[2] = 1u << unsigned(EffectKind::Bless);
    view.present(presenter, game, map, false, true, &hud);
    presenter.capture(output / "guardian-wall-bless.png");
    hud.magic.armed = true;
    hud.magic.area = true;
    hud.magic.kind = rules::SpellKind::BurningHands;
    hud.magic.affectedCount = 3;
    for (unsigned i = 0; i < 3; ++i)
        hud.magic.affected[i] = game.actors().id(i);
    view.present(presenter, game, map, false, true, &hud);
    presenter.capture(output / "shaman-targets.png");
    for (unsigned i = 0; i < 3; ++i) {
        const auto id = game.actors().id(i);
        check(game.actors().move(id, {4, 5}, game.actors().actor(id)->npc.subcell, spawn.cell).changed,
              "close review preserves three distinct subcells");
    }
    animation.setTime(0);
    view.update(game, animation.snapshot(), {});
    commands.refresh(view.picking()->targets(), false, false, true);
    hud.targets = commands.snapshot().targets;
    view.present(presenter, game, map, false, true, &hud);
    presenter.capture(output / "close-wall-targets.png");
    hud.magic.armed = false;
    view.present(presenter, game, map, false, true, &hud);
    presenter.capture(output / "close-wall-bless.png");
    presenter.presentSaving();
    presenter.capture(output / "saving.png");
    presenter.capture(output / "saving-1280.png", 1280, 720);
    const auto before = resources.counts();
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 8; ++i)
        presenter.presentSaving();
    watching = false;
    const auto after = resources.counts();
    check(!allocations && before.textures == after.textures && before.targets == after.targets,
          "cached saving frames allocate no engine C++ or GPU resources");
    {
        SavingScreen saving(&worker, presenter, [] { return true; });
        const auto completed = prepareCpu([] {
            preparationPoint("Validating staged save");
            return 42;
        });
        check(saving.closing() && completed == 42 && !worker.cancelled(),
              "native-close intent is deferred until save work joins without cancellation");
    }
    using GlError = unsigned (*)();
    check(reinterpret_cast<GlError>(rlGetProcAddress("glGetError"))() == 0,
          "review composition produces no OpenGL errors");
}
} // namespace blobber::test

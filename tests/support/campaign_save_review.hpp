#pragma once
#include "app/loading.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/allocations.hpp"
#include "support/automation_fixtures.hpp"
namespace blobber::test {
// Explicit production-presentation integration; simulation data belongs to this
// fixture. No OS input, focus changes, user saves or visible windows.
inline void campaignSaveReview(const std::filesystem::path& assets, const std::filesystem::path& output) {
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Review output must be new");
    const auto catalog = automation::test::catalog();
    const auto definitions = automation::test::definitions();
    const Map map({"#########", "#.......#", "#.......#", "#.......#", "#.......#", "#########"});
    const Player pose{{4, 4}, Heading::North};
    NpcWorld none(map, pose.cell, definitions.types, {});
    GameSession game(map, catalog, none, 5, pose, false, definitions.timings);
    Window window(1920, 1080, true);
    PreparationWorker worker;
    GpuResources resources;
    Presenter presenter(resources, assets, &worker);
    WorldRenderer renderer(resources, world_assets::load(assets / "prepared/world"));
    NpcAnimator animator(game.actors(), definitions.timings);
    animator.setTime(0);
    GameView view(renderer);
    view.update(game, animator.snapshot(), {});
    CommandController commands(game, definitions.timings);
    commands.refresh({}, false, false, true);
    auto hud = commands.snapshot();
    view.present(presenter, game, map, false, true, &hud);
    {
        SavingScreen saving(&worker, presenter, [] { return false; });
        prepareCpu([] { std::this_thread::sleep_for(std::chrono::milliseconds(70)); });
        check(saving.presentations() > 1, "Saving pumps presentation during delayed worker I/O");
        presenter.capture(output / "saving.png");
    }
    hud.campaign = hud.campaignTerminal = hud.campaignVictory = true;
    hud.combat.phase = CombatPhase::Victory;
    view.present(presenter, game, map, false, true, &hud); // Result before terminal work.
    {
        SavingScreen terminal(&worker, presenter, [] { return true; }, true);
        prepareCpu([] { std::this_thread::sleep_for(std::chrono::milliseconds(70)); });
        check(terminal.presentations() > 1 && terminal.closing() && !worker.cancelled(),
              "Terminal result pumps and native close waits without cancelling publication");
        presenter.capture(output / "victory-during-write.png");
    }
    const auto counts = resources.counts();
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 4; ++i)
        presenter.presentCached(0);
    watching = false;
    check(!allocations && resources.counts().textures == counts.textures &&
              resources.counts().targets == counts.targets,
          "Cached terminal frames allocate no engine C++ or GPU resources");
    using GlError = unsigned (*)();
    check(reinterpret_cast<GlError>(rlGetProcAddress("glGetError"))() == 0,
          "Saving/result presentation produces no OpenGL errors");
}
} // namespace blobber::test

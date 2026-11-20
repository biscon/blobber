#pragma once
#include "assets/blueprint_loader.hpp"
#include "assets/rules_loader.hpp"
#include "damp_campaign_route.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "ui/menu_screen.hpp"

namespace blobber::test {
// Explicit native review with shipped artwork. All persistence belongs to output.
inline void dampCampaignReview(const std::filesystem::path& assets, const std::filesystem::path& prepared,
                               const std::filesystem::path& output) {
    using namespace campaign_test;
    check(std::filesystem::create_directory(output), "Choose a new cave review directory");
    const auto rules = loadRules(assets / "rules/catalog.json");
    const auto defs = loadNpcDefinitions(assets / "npcs/level.json");
    const auto rooms = blueprints::load(assets / "blueprints/underkeep.json");
    CharacterLibrary library(rules, output / "characters.json");
    check(bool(library.initialize()), "Isolated review character library");
    auto draft = rules::recommendedDraft(rules, {0}, {0});
    std::snprintf(draft.name.data(), draft.name.size(), "Ida");
    const auto saved = library.create(draft, library.snapshot().revision);
    check(bool(saved), "Saved review character");
    MenuView menu(rules, library);
    menu.model.defaults();
    menu.model.activeSlot = 0;
    menu.model.assign({saved.id, -1});
    const auto roster = menu.model.resolve(rules, library.snapshot());
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Review window must be hidden");
    GpuResources resources;
    Presenter presenter(resources, assets, nullptr, prepared);
    presenter.prepareMenu();
    menu.development = true;
    menu.model.screen = MenuScreen::FloorPicker;
    menu.focus = 3;
    presenter.presentMenu(drawMenu, &menu);
    presenter.capture(output / "starting-floor.png");
    auto run = std::make_unique<CampaignRun>(rooms, rules, roster, defs.types, defs.timings, 5, 6);
    check(run->party().character({0})->build.name == "Ida" && run->party().character({0})->experience == 1162,
          "Production selected library character launches at Vault entry XP");
    WorldRenderer renderer(resources, world_assets::load(prepared / "world"));
    auto current = std::make_unique<GameView>(renderer);
    auto refresh = [&](GameView& view, GameSession& game) {
        NpcAnimator animator(game.actors(), defs.timings);
        animator.setTime(game.timeMs(), &game.effects());
        view.update(game, animator.snapshot(), {});
    };
    refresh(*current, run->game());
    current->activate();
    std::size_t peakCpu = 0, peakGpu = 0;
    auto transit = [&](TravelRequest request, bool cancel = false) {
        const auto revision = run->revision();
        auto ticket = run->prepareTravel(request);
        auto next = std::make_unique<GameView>(renderer);
        refresh(*next, ticket.game());
        peakCpu = std::max(peakCpu, renderer.cpuBytes());
        peakGpu = std::max(peakGpu, renderer.gpuBytes());
        if (cancel) {
            next.reset();
            check(!renderer.staged() && run->revision() == revision,
                  "Cancelled staged view releases destination");
            return;
        }
        check(run->commitTravel(ticket), "Cave view travel commits");
        next->activate();
        current.swap(next);
    };
    transit(run->recallRequest());
    faceStair(*run, FixtureKind::Downstairs);
    auto capture = [&](const char* name, bool waypoints = false) {
        auto& game = run->game();
        refresh(*current, game);
        CommandController commands(game, defs.timings);
        commands.refresh(current->picking()->targets(), false, false, true);
        if (waypoints)
            commands.openTravel();
        current->present(presenter, game, run->map(), false, true, &commands.snapshot());
        presenter.capture(output / name);
    };
    capture("waypoints.png", true);
    transit(run->waypointRequest(3));
    auto down = faceStair(*run, FixtureKind::Downstairs);
    transit(down, true);
    transit(down);
    transit(faceStair(*run, FixtureKind::Downstairs));
    transit(faceStair(*run, FixtureKind::Downstairs));
    auto& vault = run->game();
    vault.restorePose({{47, 12}, Heading::South}, false);
    vault.actors().damage(vault.combat().vaultMother(), 10000, vault.timeMs());
    run->update(0, false);
    check(run->status() == CampaignStatus::Victory, "Production Vault completion reaches victory");
    capture("vault-victory.png");
    check(peakCpu <= world_assets::worldCpuBudget && peakGpu <= world_assets::worldGpuBudget,
          "Peak staged residency within the renderer budgets");
    std::cout << "DC07 staged_world_cpu_bytes=" << peakCpu << " staged_world_gpu_bytes=" << peakGpu << '\n';
}
} // namespace blobber::test

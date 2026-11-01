#pragma once
#include "app/gameplay.hpp"
#include "assets/fingerprint.hpp"
#include "assets/rules_loader.hpp"
#include "persistence/campaign_store.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/check.hpp"
#include "ui/menu_screen.hpp"
#include <iostream>
namespace blobber::test {
// Explicit diagnostic mode only; routine tests own generated save fixtures.
inline void campaignStartupReview(const std::filesystem::path& assets, const std::filesystem::path& output,
                                  const std::filesystem::path& copiedSave) {
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Choose a new startup-review output directory");
    const auto before = fingerprintFile(copiedSave);
    const auto catalog = loadRules(assets / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(assets / "npcs/level.json");
    CampaignDisk disk(copiedSave);
    CampaignStore store(catalog, definitions,
                        {fingerprintFile(assets / "rules/catalog.json"), definitions.definitionFingerprint},
                        disk);
    auto campaign = store.load();
    const auto& location = campaign->location(campaign->activeLocation());
    const auto pose = campaign->game().movement().player;
    check(!gameplayPlacements(campaign->map(), *location.placements, pose.cell, campaign.get()),
          "Restored campaign bypasses the standalone placement rebuild");
    auto state = std::make_unique<FloorState>();
    campaign->game().exportFloor(*state);
    const auto digest = state->digest();
    Window window(1920, 1080, true);
    GpuResources resources;
    {
        Presenter presenter(resources, assets);
        const auto baseline = resources.counts();
        {
            WorldRenderer renderer(resources, world_assets::load(assets / "prepared/world"));
            NpcAnimator animator(campaign->game().actors(), definitions.timings);
            animator.setTime(campaign->game().timeMs(), &campaign->game().effects());
            GameView view(renderer);
            view.update(campaign->game(), animator.snapshot(), {});
            CommandController commands(campaign->game(), definitions.timings);
            commands.refresh(view.picking()->targets(), false, false, true);
            view.present(presenter, campaign->game(), campaign->map(), false, true, &commands.snapshot());
            presenter.capture(output / "continued-campaign.png");
        }
        const auto released = resources.counts();
        check(released.textures == baseline.textures && released.targets == baseline.targets &&
                  released.meshes == baseline.meshes && released.shaders == baseline.shaders,
              "Continued view releases world resources");
        CharacterLibrary library(catalog, output / "unused-library.json");
        MenuView menu(catalog, library);
        menu.canContinue = true;
        menu.message("NPC placement overlaps wall, boundary or player spawn");
        presenter.prepareMenu();
        presenter.presentMenu(drawMenu, &menu);
        presenter.capture(output / "readable-menu-error.png");
        using Error = unsigned (*)();
        check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
              "Continued campaign and error panel draw without GL errors");
    }
    campaign->game().exportFloor(*state);
    check(state->digest() == digest && fingerprintFile(copiedSave) == before,
          "Review preserves restored simulation and copied save bytes");
    std::cout << "Campaign startup review passed at depth " << campaign->activeLocation() << " cell "
              << pose.cell.x << ',' << pose.cell.y << "; save unchanged\n";
}
} // namespace blobber::test

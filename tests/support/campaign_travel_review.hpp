#pragma once
#include "assets/fingerprint.hpp"
#include "assets/rules_loader.hpp"
#include "persistence/campaign_store.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/check.hpp"
#include <iostream>
namespace blobber::test {
// Explicit production diagnostic; ordinary residency tests own tiny publications.
inline void campaignTravelReview(const std::filesystem::path& root, const std::filesystem::path& output,
                                 const std::filesystem::path& copiedSave) {
    check(std::filesystem::create_directory(output), "Choose a new travel-review output directory");
    const auto savedHash = fingerprintFile(copiedSave);
    const auto catalog = loadRules(root / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    const CampaignContent content{fingerprintFile(root / "rules/catalog.json"),
                                  definitions.definitionFingerprint};
    CampaignDisk disk(copiedSave);
    CampaignStore store(catalog, definitions, content, disk);
    auto campaign = store.load();
    check(campaign->activeLocation() == 0, "Review starts in Sanctuary at the dungeon entrance");
    const auto entrance = campaign->game().movement().player;
    campaign->game().move(Action::Forward);
    check(campaign->game().movement().blocked && campaign->game().movement().player == entrance &&
              !campaign->game().movement().transition.active(),
          "Extra forward input stays outside the entrance without a transition");
    auto assets =
        std::make_shared<const world_assets::CpuAssets>(world_assets::load(root / "prepared/world"));
    // Check the shipped appearance envelope without uploading a stress workload.
    ActorSnapshot all;
    all.typeCount = definitions.types.size();
    all.count = definitions.types.size();
    check(all.count <= all.actors.size(), "Appearance envelope fits actor fixture storage");
    for (unsigned i = 0; i < all.count; ++i)
        all.actors[i].npc.type.index = std::uint16_t(i);
    auto envelope = assets->requirements("underkeep", &all, true).textures;
    for (unsigned i = 0; i < campaignLocationCount; ++i)
        envelope |= assets->requirements(campaign->location(i).map().definition().assetPack).textures;
    std::size_t textureBytes = 0, alphaBytes = 0;
    for (unsigned i = 0; i < assets->publication.textures.size(); ++i)
        if (envelope[i]) {
            textureBytes += assets->publication.textures[i].bytes;
            alphaBytes += assets->publication.textures[i].alphaBytes;
        }
    check(textureBytes <= world_assets::textureBudget && alphaBytes <= world_assets::alphaBudget &&
              envelope.count() <= world_assets::textureCapacity,
          "Shipped location/appearance envelope fits residency admission");
    std::cout << "Envelope texture_bytes=" << textureBytes << " alpha_bytes=" << alphaBytes
              << " texture_count=" << envelope.count() << '\n';
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Travel review must remain hidden");
    GpuResources resources;
    {
        Presenter presenter(resources, root);
        presenter.prepareMenu();
        const auto baseline = resources.counts();
        {
            WorldRenderer renderer(resources, assets);
            auto current = std::make_unique<GameView>(renderer);
            const auto refresh = [&](GameView& view, GameSession& game) {
                NpcAnimator animator(game.actors(), definitions.timings);
                animator.setTime(game.timeMs(), &game.effects());
                view.update(game, animator.snapshot(), {});
            };
            const auto report = [&](const char* label) {
                const auto m = renderer.memory();
                std::cout << label << " resident_textures=" << m.residentTextures
                          << " alpha=" << m.residentAlpha << " world_gpu=" << m.worldGpu
                          << " app_gpu=" << m.applicationGpu << " app_peak=" << m.peakApplicationGpu << '\n';
            };
            refresh(*current, campaign->game());
            current->activate();
            const auto sourceBytes = renderer.memory().applicationGpu;
            const auto sourceTarget = renderer.image().target;
            const auto sourceState = encodeCampaign(catalog, definitions, content, *campaign->checkpoint());
            {
                auto ticket = campaign->prepareTravel(campaign->request(campaign->game().nearbyFixture()));
                GameView cancelled(renderer);
                refresh(cancelled, ticket.game());
                report("cancel-staged");
            }
            check(!renderer.staged() && renderer.memory().applicationGpu == sourceBytes &&
                      renderer.image().target.slot == sourceTarget.slot &&
                      renderer.image().target.generation == sourceTarget.generation &&
                      encodeCampaign(catalog, definitions, content, *campaign->checkpoint()) == sourceState,
                  "Cancelled destination preserves source resources and campaign state");
            for (unsigned destination : {1u, 0u}) {
                auto& source = campaign->game();
                if (destination == 0) {
                    source.setAnimated(false);
                    source.move(Action::TurnRight);
                    source.move(Action::TurnRight);
                }
                auto ticket = campaign->prepareTravel(campaign->request(source.nearbyFixture()));
                auto next = std::make_unique<GameView>(renderer);
                refresh(*next, ticket.game());
                report(destination ? "enter-staged" : "return-staged");
                check(campaign->commitTravel(ticket) && campaign->activeLocation() == destination,
                      "Reviewed travel commits to the requested location");
                next->activate();
                current.swap(next);
                next.reset(); // Old view dies before the old session held by ticket.
                auto& game = campaign->game();
                CommandController commands(game, definitions.timings);
                commands.refresh(current->picking()->targets(), false, false, true);
                current->present(presenter, game, campaign->map(), false, true, &commands.snapshot());
                report(destination ? "entered" : "returned");
                presenter.capture(output / (destination ? "entered-dungeon.png" : "returned-city.png"));
            }
            using Error = unsigned (*)();
            check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
                  "Travel draws without OpenGL errors");
        }
        const auto released = resources.counts();
        check(released.textures == baseline.textures && released.targets == baseline.targets &&
                  released.meshes == baseline.meshes && released.shaders == baseline.shaders,
              "Travel releases all world GPU resources");
    }
    check(fingerprintFile(copiedSave) == savedHash, "Review leaves the save byte-identical");
}
} // namespace blobber::test

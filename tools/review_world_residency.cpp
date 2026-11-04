#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "core/preparation.hpp"
#include "game/campaign.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
using namespace blobber;
namespace {
void require(bool b, const char* why) {
    if (!b)
        throw std::runtime_error(why);
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc != 3)
            throw std::runtime_error("Usage: blobber_review_world_residency ASSETS NEW_OUTPUT");
        const std::filesystem::path root(argv[1]), output(argv[2]);
        require(std::filesystem::create_directory(output), "Use a new output directory");
        const auto catalog = loadRules(root / "rules/catalog.json");
        const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
        require(catalog.preset(), "Review requires the built-in party preset");
        CampaignRun campaign(catalog, *catalog.preset(), definitions.types, definitions.timings, 5);
        auto assets =
            std::make_shared<const world_assets::CpuAssets>(world_assets::load(root / "prepared/world"));
        Window window(1920, 1080, true);
        require(IsWindowState(FLAG_WINDOW_HIDDEN), "Review window must stay hidden");
        GpuResources resources;
        Presenter presenter(resources, root);
        presenter.prepareMenu(); // Include the retained normal-menu allocation in the budget.
        WorldRenderer renderer(resources, assets);
        std::ofstream report(output / "memory.txt");
        report << "seed=5 supersampling=on logical=500x282 presentation=1920x1080\n"
               << "Byte accounting includes UI, fonts, render targets and geometry; driver allocations "
                  "excluded.\n"
               << "384 MiB steady / 448 MiB peak are advisory reference lines, not quality gates.\n";
        const auto log = [&](const char* label) {
            const auto m = renderer.memory();
            report << label << ": textures=" << m.textureCount << " installed=" << m.installedTextures
                   << " resident=" << m.residentTextures << " alpha=" << m.residentAlpha
                   << " models=" << m.residentMeshes << " world_gpu=" << m.worldGpu
                   << " app_gpu=" << m.applicationGpu << " app_peak=" << m.peakApplicationGpu << '\n';
        };
        const auto fixed = resources.counts().bytes;
        std::array<world_assets::AssetRequirements, 4> requirements;
        std::array<std::size_t, 4> geometryBytes{};
        for (unsigned i = 0; i < 4; ++i) {
            const auto& location = campaign.location(i);
            GameSession sample(location.map(), catalog, *location.placements, location.seed, location.entry,
                               false, definitions.timings, location.recovery, location.fixtures,
                               location.objective);
            requirements[i] = assets->requirements(location.map().definition().assetPack,
                                                   &sample.actors().snapshot(), true);
            const auto geometry = world_render::prepare(sample.geometry(), {}, {}, {}, assets.get());
            geometryBytes[i] = geometry.meshBytes();
        }
        for (unsigned a = 0; a < 4; ++a)
            for (unsigned b = a; b < 4; ++b) {
                const auto ids = requirements[a].textures | requirements[b].textures;
                std::size_t bytes = fixed + geometryBytes[a] + viewWidth * viewHeight * 8;
                if (a != b)
                    bytes += geometryBytes[b] + viewWidth * viewHeight * 8;
                for (unsigned id = 0; id < assets->publication.textures.size(); ++id)
                    if (ids[id])
                        bytes += assets->publication.textures[id].bytes;
                report << "projected_locations=" << a << ',' << b << " app_gpu=" << bytes
                       << " textures=" << ids.count() << '\n';
            }
        std::unique_ptr<GameView> current;
        const auto refresh = [&](GameView& view, GameSession& game) {
            NpcAnimator animator(game.actors(), definitions.timings);
            animator.setTime(game.timeMs(), &game.effects());
            view.update(game, animator.snapshot(), {});
        };
        const auto capture = [&](const char* label) {
            auto& game = campaign.game();
            refresh(*current, game);
            CommandController commands(game, definitions.timings);
            commands.refresh(current->picking()->targets(), false, false, true);
            current->present(presenter, game, campaign.map(), false, true, &commands.snapshot());
            log(label); // Normal gameplay total precedes the separate capture target.
            presenter.capture(output / (std::string(label) + ".png"));
        };
        current = std::make_unique<GameView>(renderer);
        refresh(*current, campaign.game());
        current->activate();
        capture("hub");
        const auto travel = [&](const char* label) {
            auto ticket = campaign.prepareTravel(campaign.request(campaign.game().nearbyFixture()));
            auto destination = std::make_unique<GameView>(renderer);
            refresh(*destination, ticket.game());
            log(label);
            require(campaign.commitTravel(ticket), "Review travel commit failed");
            destination->activate();
            current.swap(destination);
            destination.reset();
        };
        travel("hub-to-floor-staged");
        capture("floor-one");
        // In-process diagnostic pose change only: face the upstairs from its landing.
        const auto& f = campaign.game().fixtures().definition();
        bool found = false;
        for (unsigned i = 0; i < f.count; ++i)
            if (f.entries[i].kind == FixtureKind::Upstairs) {
                const auto& stair = f.entries[i];
                campaign.game().restorePose({stairLanding(stair), stair.facing}, true);
                campaign.game().discover();
                found = true;
                break;
            }
        require(found, "Missing upstairs");
        travel("floor-to-hub-staged");
        capture("hub-returned");
        current.reset();
        log("world-cleared");
        require(!renderer.memory().textureCount && !assets->residency().alphaBytes,
                "Location payloads retained after leaving world");
        report << "Application peak includes temporary diagnostic capture targets.\n";
        std::cout << "Residency review exported to " << output << '\n';
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

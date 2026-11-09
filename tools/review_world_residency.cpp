#include "assets/blueprint_loader.hpp"
#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "core/preparation.hpp"
#include "game/campaign.hpp"
#include "persistence/campaign_store.hpp"
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
        if (argc != 3 && (argc != 4 || std::string_view(argv[3]) != "--entrance"))
            throw std::runtime_error("Usage: blobber_review_world_residency ASSETS NEW_OUTPUT [--entrance]");
        const std::filesystem::path root(argv[1]), output(argv[2]);
        require(std::filesystem::create_directory(output), "Use a new output directory");
        const auto roomBlueprints = blueprints::load(root / "blueprints/underkeep.json");
        const auto catalog = loadRules(root / "rules/catalog.json");
        const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
        require(catalog.preset(), "Review requires the built-in party preset");
        auto campaign = std::make_unique<CampaignRun>(roomBlueprints, catalog, *catalog.preset(),
                                                      definitions.types, definitions.timings, 5);
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
                   << " models=" << m.residentMeshes << " world_cpu=" << renderer.cpuBytes()
                   << " world_gpu=" << m.worldGpu << " app_gpu=" << m.applicationGpu
                   << " app_peak=" << m.peakApplicationGpu << '\n';
        };
        const auto fixed = resources.counts().bytes;
        std::array<world_assets::AssetRequirements, 4> requirements;
        std::array<std::size_t, 4> geometryBytes{};
        for (unsigned i = 0; i < 4; ++i) {
            const auto& location = campaign->location(i);
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
                std::size_t textureBytes = 0, alphaBytes = 0;
                for (unsigned id = 0; id < assets->publication.textures.size(); ++id)
                    if (ids[id]) {
                        textureBytes += assets->publication.textures[id].bytes;
                        alphaBytes += assets->publication.textures[id].alphaBytes;
                    }
                bytes += textureBytes;
                require(textureBytes <= world_assets::textureBudget &&
                            alphaBytes <= world_assets::alphaBudget,
                        "Legal location pair exceeds resident asset budget");
                report << "projected_locations=" << a << ',' << b << " app_gpu=" << bytes
                       << " textures=" << ids.count() << " mip_bytes=" << textureBytes
                       << " alpha_bytes=" << alphaBytes << '\n';
            }
        std::unique_ptr<GameView> current;
        const auto refresh = [&](GameView& view, GameSession& game) {
            NpcAnimator animator(game.actors(), definitions.timings);
            animator.setTime(game.timeMs(), &game.effects());
            view.update(game, animator.snapshot(), {});
        };
        const auto capture = [&](const char* label, bool mapOpen = false) {
            auto& game = campaign->game();
            refresh(*current, game);
            CommandController commands(game, definitions.timings);
            commands.refresh(current->picking()->targets(), false, false, true);
            current->present(presenter, game, campaign->map(), mapOpen, true, &commands.snapshot());
            log(label); // Normal gameplay total precedes the separate capture target.
            presenter.capture(output / (std::string(label) + ".png"));
        };
        current = std::make_unique<GameView>(renderer);
        refresh(*current, campaign->game());
        current->activate();
        capture("hub");
        capture("hub-map-initial", true);
        auto move = [&](Action action) {
            campaign->game().setAnimated(false);
            campaign->game().move(action);
        };
        move(Action::Right);
        move(Action::Right);
        move(Action::Forward);
        capture("dungeon-entrance");
        if (argc == 4) {
            for (const auto& pose : std::array<std::pair<const char*, Player>, 3>{
                     {{"entrance-street", {{13, 8}, Heading::North}},
                      {"entrance-left", {{12, 7}, Heading::North}},
                      {"entrance-right", {{14, 7}, Heading::North}}}}) {
                campaign->game().restorePose(pose.second, false);
                campaign->game().discover();
                capture(pose.first);
            }
            return 0;
        }
        {
            auto ticket = campaign->prepareTravel(campaign->request(campaign->game().nearbyFixture()));
            auto cancelled = std::make_unique<GameView>(renderer);
            refresh(*cancelled, ticket.game());
            log("city-floor-cancel-staged");
            cancelled.reset();
            require(campaign->activeLocation() == 0 && !renderer.staged(), "Cancelled staging retained city");
            log("city-floor-cancelled");
        }
        const auto travel = [&](const char* label) {
            auto ticket = campaign->prepareTravel(campaign->request(campaign->game().nearbyFixture()));
            auto destination = std::make_unique<GameView>(renderer);
            refresh(*destination, ticket.game());
            log(label);
            require(campaign->commitTravel(ticket), "Review travel commit failed");
            destination->activate();
            current.swap(destination);
            destination.reset();
        };
        travel("hub-to-floor-staged");
        capture("floor-one");
        // In-process diagnostic pose change only: face the upstairs from its landing.
        const auto& f = campaign->game().fixtures().definition();
        bool found = false;
        for (unsigned i = 0; i < f.count; ++i)
            if (f.entries[i].kind == FixtureKind::Upstairs) {
                const auto& stair = f.entries[i];
                campaign->game().restorePose({stairLanding(stair), stair.facing}, true);
                campaign->game().discover();
                found = true;
                break;
            }
        require(found, "Missing upstairs");
        travel("floor-to-hub-staged");
        capture("hub-returned");
        move(Action::TurnRight);
        move(Action::TurnRight);
        travel("second-descent-staged");
        {
            const auto kits = campaign->party().campSupplies().kits;
            campaign->party().damage({0}, 1);
            const auto hp = campaign->party().character({0})->hitPoints;
            auto ticket = campaign->prepareTravel(campaign->recallRequest());
            auto destination = std::make_unique<GameView>(renderer);
            refresh(*destination, ticket.game());
            log("recall-staged");
            require(campaign->commitTravel(ticket), "Recall commit failed");
            destination->activate();
            current.swap(destination);
            destination.reset();
            require(campaign->party().campSupplies().kits == kits - 1 &&
                        campaign->party().character({0})->hitPoints == hp,
                    "Recall charges once without healing");
        }
        capture("recall-sanctuary");
        RestRequest rest;
        rest.kind = RestKind::Long;
        rest.world = campaign->game().actors().snapshot().world;
        rest.token = campaign->game().restToken();
        require(bool(campaign->game().rest(rest)), "Explicit sanctuary rest failed");
        capture("sanctuary-rested");
        // Continue exercises the real disk adapter with an isolated review-owned slot.
        CampaignDisk disk(output / "review-save.json");
        CampaignStore store(catalog, definitions, {"sc04-review", 0, &roomBlueprints}, disk);
        const auto slot = store.inspect();
        require(store.save(*campaign, slot.revision).durable, "Review save failed");
        auto loaded = store.load();
        auto continued = std::make_unique<GameView>(renderer);
        refresh(*continued, loaded->game());
        continued->activate();
        current.swap(continued);
        continued.reset();
        campaign.swap(loaded);
        loaded.reset();
        capture("continued-city");
        capture("continued-map", true);
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

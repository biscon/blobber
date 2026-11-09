#include "assets/blueprint_loader.hpp"
#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "game/campaign.hpp"
#include "game/emporium_interior.hpp"
#include "persistence/campaign_codec.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "review_shadowtart.hpp"
#include "world/sanctuary_city.hpp"
#include <fstream>
#include <iostream>
using namespace blobber;
int main(int argc, char** argv) try {
    if (argc != 3 && argc != 4)
        throw std::runtime_error(
            "Usage: blobber_review_emporium ASSETS NEW_OUTPUT [sign|alchemy|shadowtart]");
    if (argc == 4 && std::string_view(argv[3]) != "sign" && std::string_view(argv[3]) != "alchemy" &&
        std::string_view(argv[3]) != "shadowtart")
        throw std::runtime_error("Unknown emporium review selection");
    const std::filesystem::path root(argv[1]), output(argv[2]);
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Use a new output directory");
    if (argc == 4 && std::string_view(argv[3]) == "shadowtart") {
        reviewShadowtart(root, output);
        return 0;
    }
    const auto roomBlueprints = blueprints::load(root / "blueprints/underkeep.json");
    const auto catalog = loadRules(root / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    auto run = std::make_unique<CampaignRun>(roomBlueprints, catalog, *catalog.preset(), definitions.types,
                                             definitions.timings, 5);
    Window window(1920, 1080, true);
    if (!IsWindowState(FLAG_WINDOW_HIDDEN))
        throw std::runtime_error("Capture window must stay hidden");
    GpuResources resources;
    Presenter presenter(resources, root);
    WorldRenderer renderer(resources, world_assets::load(root / "prepared/world"));
    auto view = std::make_unique<GameView>(renderer);
    std::ofstream report(output / "report.txt");
    const auto memory = [&](const char* name) {
        const auto m = renderer.memory();
        report << name << " textures=" << m.residentTextures << " alpha=" << m.residentAlpha
               << " world_gpu=" << m.worldGpu << " world_cpu=" << renderer.cpuBytes()
               << " app_peak=" << m.peakApplicationGpu << '\n';
    };
    const auto update = [&] {
        auto& game = run->game();
        game.discover();
        NpcAnimator animation(game.actors(), definitions.timings);
        animation.setTime(game.timeMs(), &game.effects());
        view->update(game, animation.snapshot(), {});
        view->activate();
    };
    const auto capture = [&](const char* name) {
        update();
        auto& game = run->game();
        CommandController commands(game, definitions.timings);
        commands.refresh(view->picking()->targets(), false, false, true);
        view->present(presenter, game, run->map(), false, true, &commands.snapshot());
        presenter.capture(output / (std::string(name) + ".png"));
        report << name << " location=" << run->name() << " cell=" << game.movement().player.cell.x << ','
               << game.movement().player.cell.y << " heading=" << headingName(game.movement().player.heading)
               << " prompt=" << commands.snapshot().fixturePrompt.data() << '\n';
    };
    const auto travel = [&](bool cancel) {
        auto ticket = run->prepareTravel(run->doorwayRequest());
        auto next = std::make_unique<GameView>(renderer);
        NpcAnimator animation(ticket.game().actors(), definitions.timings);
        next->update(ticket.game(), animation.snapshot(), {});
        memory(cancel ? "staged-cancel" : "staged-commit");
        if (!cancel) {
            if (!run->commitTravel(ticket))
                throw std::runtime_error("Travel commit failed");
            next->activate();
            view.swap(next);
        }
        next.reset(); // Source view dies before the ticket's source session.
    };
    if (argc == 4 && std::string_view(argv[3]) == "alchemy") {
        run->game().restorePose(sanctuaryCity().doorway("emporium").approach, false);
        travel(false);
        capture("entry");
        run->game().restorePose({{7, 5}, Heading::East}, false);
        capture("alchemy-wide");
        run->game().restorePose({{8, 5}, Heading::East}, false);
        capture("alchemy");
        view.reset();
        if (renderer.memory().residentTextures || renderer.memory().residentAlpha)
            throw std::runtime_error("World leases remain after alchemy review");
        return 0;
    }
    run->game().restorePose({{16, 6}, Heading::South}, false);
    capture("exterior-sign");
    if (argc == 4) {
        view.reset();
        if (renderer.memory().residentTextures || renderer.memory().residentAlpha)
            throw std::runtime_error("World leases remain after sign review");
        return 0;
    }
    run->game().restorePose(sanctuaryCity().doorway("emporium").approach, false);
    update();
    memory("city-only");
    const auto sourceMemory = renderer.memory();
    resources.failAfter(0);
    bool failed = false;
    try {
        travel(true);
    } catch (const std::exception&) {
        failed = true;
    }
    resources.failAfter(-1);
    if (!failed || run->activeLocationId() != CampaignLocationId::City ||
        renderer.memory().residentTextures != sourceMemory.residentTextures ||
        renderer.memory().applicationGpu != sourceMemory.applicationGpu)
        throw std::runtime_error("GPU staging failure changed source or retained destination");
    travel(true);
    if (renderer.memory().applicationGpu != sourceMemory.applicationGpu)
        throw std::runtime_error("Canceled staging retained destination assets");
    report << "Failed/canceled GPU preparation preserves city and exact residency\n";
    travel(false);
    if (run->game().actors().snapshot().count != 1 || run->game().atSanctuary())
        throw std::runtime_error("Emporium requires its resident and ordinary rest");
    capture("entry");
    memory("emporium-only");
    const std::pair<const char*, Player> shots[]{
        {"counter", emporiumCustomer},
        {"scrolls", {{4, 7}, Heading::West}},
        {"alchemy", {{7, 5}, Heading::East}},
    };
    for (const auto& shot : shots) {
        run->game().restorePose(shot.second, false);
        capture(shot.first);
    }
    run->game().restorePose(run->location(unsigned(CampaignLocationId::Emporium)).exit, false);
    const auto checkpoint = run->checkpoint();
    const CampaignContent content{"emporium-review", definitions.definitionFingerprint, &roomBlueprints};
    const auto text = encodeCampaign(catalog, definitions, content, *checkpoint);
    report << "Seven-location snapshot bytes=" << run->snapshotBytes() << " active save bytes=" << text.size()
           << '\n';
    auto decoded = decodeCampaign(catalog, definitions, content, text);
    view.reset();
    run = std::make_unique<CampaignRun>(roomBlueprints, catalog, std::move(*decoded), definitions.timings);
    view = std::make_unique<GameView>(renderer);
    capture("exit-continue");
    travel(false);
    if (!(run->game().movement().player ==
          Player{sanctuaryCity().doorway("emporium").approach.cell, Heading::West}))
        throw std::runtime_error("Exit did not return to the correct street pose");
    update();
    view.reset();
    memory("released");
    if (renderer.memory().residentTextures || renderer.memory().residentAlpha)
        throw std::runtime_error("World leases remain after review");
    report << "Production Continue, return doorway and final lease release pass\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}

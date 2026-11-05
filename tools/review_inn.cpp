#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "game/campaign.hpp"
#include "persistence/campaign_codec.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "world/sanctuary_city.hpp"
#include <fstream>
#include <iostream>
using namespace blobber;
int main(int argc, char** argv) try {
    if (argc != 3 && argc != 4)
        throw std::runtime_error("Usage: blobber_review_inn ASSETS NEW_OUTPUT [details]");
    const std::filesystem::path root(argv[1]), output(argv[2]);
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Use a new output directory");
    const auto catalog = loadRules(root / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    auto run =
        std::make_unique<CampaignRun>(catalog, *catalog.preset(), definitions.types, definitions.timings, 5);
    Window window(1920, 1080, true);
    if (!IsWindowState(FLAG_WINDOW_HIDDEN))
        throw std::runtime_error("Capture window must stay hidden");
    GpuResources resources;
    Presenter presenter(resources, root);
    WorldRenderer renderer(resources, world_assets::load(root / "prepared/world"));
    auto view = std::make_unique<GameView>(renderer);
    std::ofstream report(output / "report.txt");
    auto memory = [&](const char* name) {
        const auto m = renderer.memory();
        report << name << " textures=" << m.residentTextures << " alpha=" << m.residentAlpha
               << " world_gpu=" << m.worldGpu << " world_cpu=" << renderer.cpuBytes()
               << " app_peak=" << m.peakApplicationGpu << '\n';
    };
    auto update = [&] {
        auto& game = run->game();
        game.discover();
        NpcAnimator animation(game.actors(), definitions.timings);
        animation.setTime(game.timeMs(), &game.effects());
        view->update(game, animation.snapshot(), {});
        view->activate();
    };
    auto capture = [&](const char* name, bool map = false, bool rest = false) {
        update();
        auto& game = run->game();
        CommandController commands(game, definitions.timings);
        commands.refresh(view->picking() ? view->picking()->targets() : TargetSnapshot{}, false, map, true);
        if (rest) {
            commands.openCamp();
            commands.campKind(RestKind::Long);
        }
        view->present(presenter, game, run->map(), map, true, &commands.snapshot());
        presenter.capture(output / (std::string(name) + ".png"));
        const auto p = game.movement().player;
        report << name << " location=" << run->name() << " cell=" << p.cell.x << ',' << p.cell.y
               << " heading=" << headingName(p.heading) << " clock=" << game.timeMs()
               << " prompt=" << commands.snapshot().fixturePrompt.data() << '\n';
    };
    auto travel = [&](bool cancel) {
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
        next.reset(); // Source view must die before ticket's source session.
    };
    run->game().restorePose(sanctuaryCity().doorway("inn").approach, false);
    capture("city-entry");
    memory("city-only");
    travel(true);
    memory("city-after-cancel");
    travel(false);
    capture("entry");
    memory("inn-only");
    if (argc == 4) {
        if (std::string_view(argv[3]) != "details")
            throw std::runtime_error("Unknown capture selection");
        run->game().restorePose({{3, 9}, Heading::West}, false);
        capture("plant-close");
        run->game().restorePose({{8, 3}, Heading::North}, false);
        capture("ale-barrels");
        view.reset();
        memory("released");
        return 0;
    }
    const std::pair<const char*, Player> shots[]{
        {"common-room", {{5, 8}, Heading::North}}, {"bar", {{6, 5}, Heading::North}},
        {"hearth", {{4, 4}, Heading::West}},       {"seating", {{5, 6}, Heading::West}},
        {"stairs", {{7, 5}, Heading::East}},       {"plants-exit", {{5, 7}, Heading::South}},
        {"staff-aisle", {{4, 3}, Heading::East}},
        {"plant-close", {{3, 9}, Heading::West}},  {"ale-barrels", {{8, 3}, Heading::North}},
    };
    for (const auto& shot : shots) {
        run->game().restorePose(shot.second, false);
        capture(shot.first);
    }
    run->game().restorePose({{4, 4}, Heading::West}, false);
    capture("fire-0");
    const auto pickingRevision = renderer.visibility()->frame().revision;
    run->update(.2, false);
    capture("fire-1");
    if (renderer.visibility()->frame().revision != pickingRevision)
        throw std::runtime_error("Decorative fire invalidated picking geometry");
    const auto clock = run->game().timeMs();
    const auto image = renderer.image().revision;
    run->update(.2, true);
    update();
    if (run->game().timeMs() != clock || renderer.image().revision != image)
        throw std::runtime_error("Pause did not freeze fire/image cache");
    report << "Fire animation preserves picking revision and paused image cache\n";
    run->party().damage({0}, 1);
    capture("free-rest", false, true);
    capture("inn-map", true);
    auto checkpoint = run->checkpoint();
    auto text =
        encodeCampaign(catalog, definitions, {"inn-review", definitions.definitionFingerprint}, *checkpoint);
    report << "Active inn serialized bytes=" << text.size() << '\n';
    auto saved =
        decodeCampaign(catalog, definitions, {"inn-review", definitions.definitionFingerprint}, text);
    view.reset();
    run = std::make_unique<CampaignRun>(catalog, std::move(*saved), definitions.timings);
    view = std::make_unique<GameView>(renderer);
    capture("continue-inn");
    run->game().restorePose(run->location(4).exit, false);
    capture("exit");
    travel(false);
    capture("city-return");
    memory("city-returned");
    view.reset();
    memory("released");
    if (renderer.memory().residentTextures || renderer.memory().residentAlpha)
        throw std::runtime_error("World assets retained after view release");
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}

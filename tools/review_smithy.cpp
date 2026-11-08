#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "game/campaign.hpp"
#include "game/smithy_interior.hpp"
#include "persistence/campaign_codec.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "review_blacksmith.hpp"
#include "world/sanctuary_city.hpp"
#include <fstream>
#include <iostream>
using namespace blobber;
int main(int argc, char** argv) try {
    if (argc != 3 && argc != 4)
        throw std::runtime_error("Usage: blobber_review_smithy ASSETS NEW_OUTPUT [equipment|blacksmith]");
    if (argc == 4 && std::string_view(argv[3]) != "equipment" && std::string_view(argv[3]) != "blacksmith")
        throw std::runtime_error("Unknown smithy review selection");
    const std::filesystem::path root(argv[1]), output(argv[2]);
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Use a new output directory");
    if (argc == 4 && std::string_view(argv[3]) == "blacksmith") {
        reviewBlacksmith(root, output);
        return 0;
    }
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
    auto capture = [&](const char* name, bool map = false) {
        update();
        auto& game = run->game();
        CommandController commands(game, definitions.timings);
        commands.refresh(view->picking() ? view->picking()->targets() : TargetSnapshot{}, false, map, true);
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
    if (argc == 4) {
        run->game().restorePose(sanctuaryCity().doorway("smithy").approach, false);
        travel(false);
        capture("entry");
        run->game().restorePose({{8, 5}, Heading::East}, false);
        capture("equipment");
        run->game().restorePose({{8, 4}, Heading::East}, false);
        capture("equipment-side");
        view.reset();
        if (renderer.memory().residentTextures || renderer.memory().residentAlpha)
            throw std::runtime_error("World assets retained after equipment review");
        return 0;
    }
    report << "Snapshot bytes=" << run->snapshotBytes() << '\n';
    run->game().restorePose(sanctuaryCity().doorway("smithy").approach, false);
    capture("city-entry");
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
        throw std::runtime_error("Failed GPU staging did not preserve source and release destination");
    capture("city-after-failure");
    report << "Injected GPU preparation failure preserves city and residency\n";
    travel(true);
    if (renderer.memory().applicationGpu != sourceMemory.applicationGpu)
        throw std::runtime_error("Canceled GPU staging retained destination resources");
    memory("city-after-cancel");
    travel(false);
    capture("entry");
    memory("smithy-only");
    const std::pair<const char*, Player> shots[]{
        {"counter", smithyCustomer},
        {"forge", {{4, 3}, Heading::West}},
        {"equipment", {{8, 5}, Heading::East}},
        {"staff-passage", {{8, 3}, Heading::West}},
    };
    for (const auto& shot : shots) {
        run->game().restorePose(shot.second, false);
        capture(shot.first);
    }
    run->game().restorePose({{4, 3}, Heading::West}, false);
    capture("fire-before");
    const auto pickingRevision = renderer.visibility()->frame().revision;
    run->update(.2, false);
    capture("fire-after");
    if (renderer.visibility()->frame().revision != pickingRevision)
        throw std::runtime_error("Decorative fire invalidated picking geometry");
    const auto clock = run->game().timeMs();
    const auto image = renderer.image().revision;
    run->update(.2, true);
    update();
    if (run->game().timeMs() != clock || renderer.image().revision != image)
        throw std::runtime_error("Pause did not freeze fire/image cache");
    report << "Fire animation preserves picking revision and paused image cache\n";
    capture("smithy-map", true);
    run->game().restorePose(run->location(unsigned(CampaignLocationId::Smithy)).exit, false);
    auto checkpoint = run->checkpoint();
    const auto text = encodeCampaign(catalog, definitions,
                                     {"smithy-review", definitions.definitionFingerprint}, *checkpoint);
    report << "Active smithy serialized bytes=" << text.size() << '\n';
    auto saved =
        decodeCampaign(catalog, definitions, {"smithy-review", definitions.definitionFingerprint}, text);
    view.reset();
    run = std::make_unique<CampaignRun>(catalog, std::move(*saved), definitions.timings);
    view = std::make_unique<GameView>(renderer);
    capture("exit-continue");
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

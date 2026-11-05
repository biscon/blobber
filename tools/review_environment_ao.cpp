#include "assets/image.hpp"
#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "world/sanctuary_city.hpp"
#include <fstream>
#include <iostream>

using namespace blobber;
int main(int argc, char** argv) try {
    if (argc != 3)
        throw std::runtime_error("Usage: blobber_review_environment_ao ASSETS NEW_OUTPUT");
    const std::filesystem::path root(argv[1]), output(argv[2]);
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Use a new capture directory");
    const auto rules = loadRules(root / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    Window window(1920, 1080, true, true);
    if (!IsWindowState(FLAG_WINDOW_HIDDEN))
        throw std::runtime_error("AO captures require a hidden window");
    GpuResources resources;
    Presenter presenter(resources, root);
    WorldRenderer renderer(resources, world_assets::load(root / "prepared/world"));
    std::ofstream report(output / "report.txt");
    auto capture = [&](GameSession& game, GameView& view, const std::string& name) {
        NpcAnimator animator(game.actors(), definitions.timings);
        animator.setTime(game.timeMs());
        CommandController commands(game, definitions.timings);
        for (bool ao : {false, true}) {
            renderer.ambientOcclusion(ao);
            view.update(game, animator.snapshot(), {});
            view.activate();
            commands.refresh(view.picking() ? view.picking()->targets() : TargetSnapshot{}, game.actionBusy(),
                             false, true);
            view.present(presenter, game, game.geometry().base(), false, true, &commands.snapshot());
            presenter.capture(output / (name + (ao ? "-on.png" : "-off.png")));
        }
        OwnedImage image(LoadImageFromTexture(resources.target(renderer.ambientOcclusionTarget()).texture));
        ImageFlipVertical(&image.value);
        if (!ExportImage(image.value, (output / (name + "-ao.png")).c_str()))
            throw std::runtime_error("AO capture export failed");
        report << name << " world_gpu_bytes=" << renderer.gpuBytes()
               << " targets=" << resources.counts().targets << " shaders=" << resources.counts().shaders
               << " draws=" << renderer.stats.draws << '\n';
    };
    {
        const auto city = sanctuaryCity();
        const NpcWorld empty(city.map, city.spawn.cell, definitions.types, {});
        GameSession game(city.map, rules, *rules.preset(), empty, 5, city.spawn, false, definitions.timings);
        GameView view(renderer);
        game.restorePose({{11, 10}, Heading::South}, true);
        capture(game, view, "city-fountain");
        game.restorePose({{14, 6}, Heading::East}, true);
        capture(game, view, "city-facade");
        game.setAnimated(true);
        game.move(Action::TurnRight);
        game.update(.08f, false);
        capture(game, view, "city-facade-turn");
    }
    {
        WorldDefinition world;
        world.assetPack = "underkeep";
        Decoration chest{{3, 2}, Heading::South, DecorationKind::Chest, 0, .5f, 0, .76f, .36f, .58f};
        chest.container = 0;
        world.decorations = {chest};
        const Map map({"#########", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#",
                       "#.......#", "#.......#", "#########"},
                      world);
        const auto goblin = std::find(definitions.types.begin(), definitions.types.end(), "goblin");
        if (goblin == definitions.types.end())
            throw std::runtime_error("Review requires a goblin appearance");
        const NpcTypeId type{std::uint16_t(goblin - definitions.types.begin())};
        const Player spawn{{4, 4}, Heading::North};
        const NpcWorld npcs(map, spawn.cell, definitions.types, {{type, {4, 2}, {1, 1}, Heading::South}});
        FloorFixtures fixtures;
        fixtures.count = 1;
        fixtures.entries[0] = {{4, 3}, Heading::North, FixtureKind::Door};
        GameSession game(map, rules, *rules.preset(), npcs, 5, spawn, false, definitions.timings, {},
                         fixtures);
        GameView view(renderer);
        game.setAnimated(false);
        capture(game, view, "dungeon-door-closed");
        if (!game.interact(game.nearbyFixture()))
            throw std::runtime_error("Review cannot open door");
        capture(game, view, "dungeon-door-creature");
        game.restorePose({{3, 3}, Heading::North}, true);
        capture(game, view, "dungeon-chest");
        game.setAnimated(true);
        game.move(Action::TurnLeft);
        game.update(.08f, false);
        capture(game, view, "dungeon-chest-turn");
        game.restorePose({{1, 2}, Heading::North}, true);
        capture(game, view, "dungeon-corner");
    }
    resources.finishPreparation();
    std::cout << "AO on/off/diagnostic captures complete: " << output << '\n';
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}

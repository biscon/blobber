#include "assets/image.hpp"
#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "game/inn_interior.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "world/sanctuary_city.hpp"
#include <fstream>
#include <iostream>

using namespace blobber;
using world_render::DungeonLighting;
namespace {
constexpr DungeonLighting presets[]{DungeonLighting::Current, DungeonLighting::Gentle,
                                    DungeonLighting::Strong, DungeonLighting::Early};
constexpr const char* names[]{"current", "a-gentle", "b-strong", "c-early"};
std::vector<unsigned char> pixels(GpuResources& r, TargetHandle target) {
    OwnedImage image(LoadImageFromTexture(r.target(target).texture));
    ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const auto* p = static_cast<const unsigned char*>(image.value.data);
    return {p, p + image.value.width * image.value.height * 4};
}
} // namespace
int main(int argc, char** argv) try {
    if (argc != 3)
        throw std::runtime_error("Usage: blobber_review_dungeon_lighting ASSETS NEW_OUTPUT");
    const std::filesystem::path root(argv[1]), output(argv[2]);
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Use a new capture directory");
    const auto rules = loadRules(root / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    const auto goblin = std::find(definitions.types.begin(), definitions.types.end(), "goblin");
    if (goblin == definitions.types.end())
        throw std::runtime_error("Review requires goblin appearance");
    const NpcTypeId type{std::uint16_t(goblin - definitions.types.begin())};
    Window window(1920, 1080, true, true);
    if (!IsWindowState(FLAG_WINDOW_HIDDEN))
        throw std::runtime_error("Lighting captures require a hidden window");
    GpuResources resources;
    Presenter presenter(resources, root);
    WorldRenderer renderer(resources, world_assets::load(root / "prepared/world"));
    std::ofstream report(output / "report.txt");
    auto compare = [&](GameSession& game, const char* name, bool exportImages) {
        GameView view(renderer);
        NpcAnimator animator(game.actors(), definitions.timings);
        animator.setTime(game.timeMs());
        CommandController commands(game, definitions.timings);
        std::vector<unsigned char> reference;
        for (unsigned i = 0; i < 4; ++i) {
            renderer.dungeonLighting(presets[i]);
            view.update(game, animator.snapshot(), {});
            view.activate();
            if (exportImages) {
                commands.refresh(view.picking() ? view.picking()->targets() : TargetSnapshot{},
                                 game.actionBusy(), false, true);
                view.present(presenter, game, game.geometry().base(), false, true, &commands.snapshot());
                presenter.capture(output / (std::string(name) + "-" + names[i] + ".png"));
            } else {
                auto image = pixels(resources, renderer.image().target);
                if (!i)
                    reference = std::move(image);
                else if (image != reference)
                    throw std::runtime_error(std::string(name) + " changed with dungeon lighting");
            }
            report << name << ' ' << names[i] << " clock=" << game.timeMs()
                   << " gpu_bytes=" << renderer.gpuBytes() << '\n';
        }
    };
    {
        WorldDefinition world;
        world.assetPack = "underkeep";
        // Narrow enough to read wall falloff, long enough to reach the existing fog.
        std::vector<std::string> rows(16, "##...##");
        rows.front() = rows.back() = "#######";
        const Map map(rows, world);
        const Player spawn{{3, 13}, Heading::North};
        const NpcWorld empty(map, spawn.cell, definitions.types, {});
        GameSession game(map, rules, *rules.preset(), empty, 5, spawn, false, definitions.timings);
        compare(game, "corridor", true);
    }
    {
        WorldDefinition world;
        world.assetPack = "underkeep";
        Decoration chest{{2, 5}, Heading::South, DecorationKind::Chest, 0, .5f, 0, .76f, .36f, .58f};
        chest.container = 0;
        world.decorations = {chest,
                             {{6, 3}, Heading::North, DecorationKind::Cage, 0, .5f, 0, .84f, .84f, .72f}};
        const Map map({"#########", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#",
                       "#.......#", "#.......#", "#.......#", "#########"},
                      world);
        const Player spawn{{4, 7}, Heading::North};
        const NpcWorld npcs(map, spawn.cell, definitions.types, {{type, {4, 3}, {1, 1}, Heading::South}});
        GameSession game(map, rules, *rules.preset(), npcs, 5, spawn, false, definitions.timings);
        compare(game, "room", true);
    }
    for (bool inn : {false, true}) {
        const auto map = inn ? innInterior().map : sanctuaryCity().map;
        const Player pose = inn ? Player{{5, 8}, Heading::North} : Player{{11, 10}, Heading::South};
        const NpcWorld empty(map, pose.cell, definitions.types, {});
        GameSession game(map, rules, *rules.preset(), empty, 5, pose, false, definitions.timings);
        compare(game, inn ? "inn" : "city", false);
    }
    resources.finishPreparation();
    report << "Production city and inn outputs identical across all presets\n";
    std::cout << "Eight lighting captures exported; production city/inn comparisons unchanged\n";
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}

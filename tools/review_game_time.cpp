#include "assets/image.hpp"
#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "game/inn_interior.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include <fstream>
#include <iostream>
using namespace blobber;
namespace {
std::vector<unsigned char> pixels(GpuResources& resources, TargetHandle target) {
    OwnedImage image(LoadImageFromTexture(resources.target(target).texture));
    if (!image.value.data)
        throw std::runtime_error("Cannot read review output");
    ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const auto* p = static_cast<const unsigned char*>(image.value.data);
    return {p, p + image.value.width * image.value.height * 4};
}
} // namespace
int main(int argc, char** argv) try {
    if (argc != 3)
        throw std::runtime_error("Usage: blobber_review_game_time ASSETS NEW_OUTPUT");
    const std::filesystem::path root(argv[1]), output(argv[2]);
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Choose a new review output directory");
    const auto rules = loadRules(root / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    Window window(1920, 1080, true, true);
    if (!IsWindowState(FLAG_WINDOW_HIDDEN))
        throw std::runtime_error("Review requires a hidden window");
    GpuResources resources;
    Presenter presenter(resources, root);
    const auto largest = GameTime{maxCheckpointClock}.text();
    if (presenter.fonts().width(largest.data(), UiTextStyle::Small) > 102)
        throw std::runtime_error("Maximum supported day does not fit map sidebar");
    WorldRenderer renderer(resources, world_assets::load(root / "prepared/world"));
    std::ofstream report(output / "review.txt");
    auto capture = [&](GameView& view, GameSession& game, NpcAnimator& animator, CommandController& commands,
                       GameTime time, const char* filename, bool map = false) {
        view.update(game, animator.snapshot(), {}, {}, false, time);
        view.activate();
        commands.refresh(view.picking() ? view.picking()->targets() : TargetSnapshot{}, false, map, true);
        auto hud = commands.snapshot();
        hud.gameTime = time;
        view.present(presenter, game, game.geometry().base(), map, true, &hud);
        presenter.capture(output / filename);
        report << filename << ' ' << time.text().data() << '\n';
    };
    {
        auto city = sanctuaryHub();
        const Player pose{{11, 8}, Heading::West};
        const NpcWorld residents(city.map, pose.cell, definitions.types, sanctuaryResidents(definitions.types));
        GameSession game(city.map, rules, residents, 5, pose, false, definitions.timings, city.recovery,
                         city.fixtures);
        NpcAnimator animator(game.actors(), definitions.timings);
        animator.setTime(0);
        CommandController commands(game, definitions.timings);
        GameView view(renderer);
        capture(view, game, animator, commands, {4 * gameDuration::hour}, "city-day.png");
        capture(view, game, animator, commands, {10 * gameDuration::hour}, "city-dusk.png");
        capture(view, game, animator, commands, {14 * gameDuration::hour}, "city-night.png");
        // A representative multi-day date, with both key rows and development control visible.
        view.update(game, animator.snapshot(), {}, {}, false, GameTime{30 * gameDuration::hour});
        commands.refresh({}, false, true, true);
        auto hud = commands.snapshot();
        hud.gameTime = {30 * gameDuration::hour};
        hud.campaign = true;
        hud.locationName = "Sanctuary";
        hud.questKeys = (1u << 2) | (1u << 3);
        hud.development = true;
        view.present(presenter, game, city.map, true, true, &hud);
        presenter.capture(output / "map-clock.png");
        report << "map-clock.png Day 2 14:00; longest supported date width="
               << presenter.fonts().width(largest.data(), UiTextStyle::Small) << " / 102\n";
    }
    for (bool inn : {true, false}) {
        auto interior = innInterior();
        WorldDefinition stone;
        stone.assetPack = "underkeep";
        const Map dungeon({"#########", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#",
                           "#.......#", "#.......#", "#.......#", "#########"}, stone);
        const auto& map = inn ? interior.map : dungeon;
        const Player pose = inn ? Player{{5, 8}, Heading::North} : Player{{4, 7}, Heading::North};
        const NpcWorld residents(map, pose.cell, definitions.types,
                                 inn ? std::vector<NpcInstance>{innkeeperPlacement(definitions.types)}
                                     : std::vector<NpcInstance>{});
        GameSession game(map, rules, residents, 5, pose, false, definitions.timings);
        NpcAnimator animator(game.actors(), definitions.timings);
        animator.setTime(0);
        CommandController commands(game, definitions.timings);
        GameView view(renderer);
        view.update(game, animator.snapshot(), {}, {}, false, GameTime{});
        view.activate();
        const auto day = pixels(resources, renderer.image().target);
        capture(view, game, animator, commands, {14 * gameDuration::hour},
                inn ? "inn-night.png" : "dungeon-night.png");
        if (pixels(resources, renderer.image().target) != day)
            throw std::runtime_error("Production interior changed at night");
        report << "Interior world pixels identical to day\n";
    }
    std::cout << "Six game-time captures exported; production interiors unchanged\n";
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}

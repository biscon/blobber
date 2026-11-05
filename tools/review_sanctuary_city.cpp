#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "world/sanctuary_city.hpp"
#include <fstream>
#include <iostream>
using namespace blobber;
int main(int argc, char** argv) try {
    if (argc != 3 && argc != 4)
        throw std::runtime_error("Usage: blobber_review_sanctuary_city ASSETS NEW_OUTPUT [SHOT]");
    const std::string selected = argc == 4 ? argv[3] : "";
    const std::filesystem::path root(argv[1]), output(argv[2]);
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Use a new output directory");
    const auto rules = loadRules(root / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    if (!rules.preset())
        throw std::runtime_error("City review requires the preset party");
    auto publication = world_assets::load(root / "prepared/world");
    if (publication.definitions != definitions.definitionFingerprint)
        throw std::runtime_error("City review definitions differ; rebuild world assets");
    const auto city = sanctuaryCityReview();
    const NpcWorld empty(city.map, city.spawn.cell, definitions.types, {});
    GameSession game(city.map, rules, *rules.preset(), empty, 5, city.spawn, false, definitions.timings);
    NpcAnimator animator(game.actors(), definitions.timings);
    Window window(1920, 1080, true);
    if (!IsWindowState(FLAG_WINDOW_HIDDEN))
        throw std::runtime_error("Capture must stay hidden");
    GpuResources resources;
    Presenter presenter(resources, root);
    WorldRenderer renderer(resources, std::move(publication));
    std::ofstream report(output / "report.txt");
    report << "SC01.2 residential corner; seed=5 supersampling=on native=1920x1080\n";
    {
        GameView view(renderer);
        CommandController commands(game, definitions.timings);
        unsigned captured = 0;
        const auto capture = [&](const std::string& name) {
            if (!selected.empty() && name != selected)
                return;
            ++captured;
            game.discover();
            animator.setTime(game.timeMs(), &game.effects());
            view.update(game, animator.snapshot(), {});
            view.activate();
            const auto* picking = view.picking();
            commands.refresh(picking ? picking->targets() : TargetSnapshot{}, game.actionBusy(), false, true);
            view.present(presenter, game, city.map, false, true, &commands.snapshot());
            presenter.capture(output / (name + ".png"));
            const auto& m = game.movement();
            const auto camera = world_render::camera(m, game.geometry());
            report << name << " cell=" << m.player.cell.x << ',' << m.player.cell.y
                   << " heading=" << headingName(m.player.heading) << " camera=" << camera.position.x << ','
                   << camera.position.y << ',' << camera.position.z << " yaw=" << camera.yaw
                   << " progress=" << m.transition.progress() << " draws=" << renderer.stats.draws << '\n';
        };
        struct Pose {
            const char* name;
            Player player;
        };
        const Pose poses[] = {{"01-corner", city.spawn},
                              {"02-low-door", city.doors[0].approach},
                              {"03-tall-door", city.doors[1].approach},
                              {"04-low-roof", {{5, 10}, Heading::North}},
                              {"05-tall-roof", {{2, 8}, Heading::East}},
                              {"06-low-back", {{5, 2}, Heading::South}},
                              {"07-tall-back", {{14, 8}, Heading::West}},
                              {"08-side", {{10, 12}, Heading::North}}};
        for (const auto& pose : poses) {
            game.restorePose(pose.player, true);
            capture(pose.name);
        }
        game.restorePose({{2, 13}, Heading::North}, true);
        game.move(Action::TurnRight);
        game.update(.08f, false);
        capture("09-roofline-diagonal");
        game.update(.08f, false);
        game.restorePose({{1, 14}, Heading::North}, true);
        game.move(Action::TurnRight);
        game.update(.08f, false);
        capture("10-roof-materials");
        game.update(.08f, false);
        game.restorePose(city.spawn, true);
        const Action actions[] = {Action::Forward, Action::Forward, Action::TurnLeft, Action::Forward,
                                  Action::Right,   Action::Back,    Action::TurnRight};
        for (unsigned i = 0; i < std::size(actions); ++i) {
            game.move(actions[i]);
            if (game.movement().blocked)
                throw std::runtime_error("Scripted city route is blocked");
            for (unsigned step = 1; step <= 4; ++step) {
                game.update(.04f, false);
                capture("motion-" + std::to_string(i) + "-" + std::to_string(step));
            }
        }
        if (!captured)
            throw std::runtime_error("Unknown review shot");
        const auto m = renderer.memory();
        const auto* scene = renderer.scene();
        report << "resident_textures=" << m.textureCount << " texture_bytes=" << m.residentTextures
               << " alpha_bytes=" << m.residentAlpha << " model_bytes=" << m.residentMeshes
               << " scene_cpu=" << scene->bytes() << " scene_gpu=" << scene->meshBytes()
               << " app_gpu=" << m.applicationGpu << " app_peak=" << m.peakApplicationGpu << '\n';
        report << "Driver allocations excluded. Peak includes temporary capture target.\n";
    }
    if (renderer.memory().textureCount || renderer.assets().residency().meshes)
        throw std::runtime_error("Review retained location assets after view release");
    report << "released_world_textures=0 released_models=0\n";
    std::cout << "City review exported to " << output << '\n';
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}

#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "game/inn_interior.hpp"
#include "generation/floor.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "render/world_projection_parameters.hpp"
#include "rlgl.h"
#include "world/sanctuary_city.hpp"
#include "world/view.hpp"
#include <fstream>
#include <iostream>

using namespace blobber;
namespace {
// Select a real T/cross junction, rather than the middle of an open room.
Player intersection(const generation::GeneratedFloor& floor) {
    Player best{};
    int bestScore = -1;
    for (int y = 1; y < floor.map.height() - 1; ++y)
        for (int x = 1; x < floor.map.width() - 1; ++x) {
            const Cell cell{x, y};
            if (floor.map.movementBlocked(cell))
                continue;
            bool fixture = false;
            for (unsigned i = 0; i < floor.layout.fixtures.count; ++i)
                fixture |= floor.layout.fixtures.entries[i].cell == cell;
            if (fixture)
                continue;
            int corners = 0;
            for (int dy : {-1, 1})
                for (int dx : {-1, 1})
                    corners += floor.map.solid(cell + Cell{dx, dy});
            if (corners < 2)
                continue;
            for (unsigned h = 0; h < 4; ++h) {
                const auto heading = Heading(h);
                const auto f = forward(heading), r = right(heading);
                if (floor.map.movementBlocked(cell + f) || floor.map.movementBlocked(cell + r) ||
                    floor.map.movementBlocked(cell + r * -1))
                    continue;
                int reach = 1;
                while (reach < 8 && !floor.map.movementBlocked(cell + f * (reach + 1)))
                    ++reach;
                const int score = corners * 10 + reach;
                if (score > bestScore) {
                    bestScore = score;
                    best = {cell, heading};
                }
            }
        }
    if (bestScore < 0)
        throw std::runtime_error("Review seed has no T/cross intersection");
    return best;
}
} // namespace

int main(int argc, char** argv) try {
    if (argc != 3)
        throw std::runtime_error("Usage: blobber_review_camera_framing ASSETS NEW_OUTPUT");
    const std::filesystem::path root(argv[1]), output(argv[2]);
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Use a new capture directory");
    const auto rules = loadRules(root / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    if (!rules.preset())
        throw std::runtime_error("Review requires the preset party");
    SetTraceLogLevel(LOG_WARNING);
    Window window(1920, 1080, true, true);
    if (!IsWindowState(FLAG_WINDOW_HIDDEN))
        throw std::runtime_error("Capture must stay hidden");
    GpuResources resources;
    Presenter presenter(resources, root);
    WorldRenderer renderer(resources, world_assets::load(root / "prepared/world"));
    std::ofstream report(output / "report.txt");
    report << "horizon=" << world_render::viewHorizon << " setback=" << partyCameraSetback
           << " eye=.5 horizontal_fov=84 supersampling=on size=1920x1080\n";
    auto capture = [&](const Map& map, Player pose, const char* name, FloorFixtures fixtures = {}) {
        if (map.movementBlocked(pose.cell))
            throw std::runtime_error("Blocked review pose");
        const NpcWorld empty(map, pose.cell, definitions.types, {});
        GameSession game(map, rules, *rules.preset(), empty, 5, pose, false, definitions.timings, {},
                         fixtures);
        game.discover();
        NpcAnimator animator(game.actors(), definitions.timings);
        animator.setTime(game.timeMs(), &game.effects());
        GameView view(renderer);
        CommandController commands(game, definitions.timings);
        view.update(game, animator.snapshot(), {});
        view.activate();
        commands.refresh(view.picking() ? view.picking()->targets() : TargetSnapshot{}, false, false, true);
        view.present(presenter, game, map, false, true, &commands.snapshot());
        presenter.capture(output / (std::string(name) + ".png"));
        using GlError = unsigned (*)();
        if (reinterpret_cast<GlError>(rlGetProcAddress("glGetError"))() != 0)
            throw std::runtime_error("Camera capture OpenGL error");
        const auto camera = world_render::camera(game.movement(), game.geometry());
        report << name << " cell=" << pose.cell.x << ',' << pose.cell.y
               << " heading=" << headingName(pose.heading) << " clock=" << game.timeMs()
               << " camera=" << camera.position.x << ',' << camera.position.y << ',' << camera.position.z
               << '\n';
    };
    const auto generated = generation::generate({5, 1});
    if (!generated)
        throw std::runtime_error("Review floor generation failed");
    const auto& floor = *generated.floor;
    report << "dungeon_seed=5 depth=1 generator=" << generation::generatorVersion
           << " templates=" << generation::templateVersion << " fingerprint=" << floor.fingerprint
           << " encounters_and_door_fixtures=omitted\n";
    capture(floor.map, intersection(floor), "dungeon");
    const auto city = sanctuaryCity();
    capture(city.map, {{11, 10}, Heading::South}, "sanctuary");
    const auto inn = innInterior();
    capture(inn.map, {{5, 8}, Heading::North}, "inn", inn.fixtures);
    if (renderer.memory().textureCount || renderer.assets().residency().meshes)
        throw std::runtime_error("Review retained world assets after view release");
    std::cout << "Three matched hidden camera captures exported to " << output << '\n';
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}

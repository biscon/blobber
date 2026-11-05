#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "sanctuary_city_layout.hpp"
#include "world/sanctuary_city.hpp"
#include <fstream>
#include <iostream>
using namespace blobber;
int main(int argc, char** argv) try {
    if (argc != 3 && argc != 4)
        throw std::runtime_error("Usage: blobber_review_sanctuary_city ASSETS NEW_OUTPUT "
                                 "[SHOT|materials|layout|stills|motion|gate-motion]");
    const std::string selected = argc == 4 ? argv[3] : "";
    const std::filesystem::path root(argv[1]), output(argv[2]);
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Use a new output directory");
    const auto city = sanctuaryCityReview();
    std::ofstream layout(output / "layout.svg");
    writeCityLayout(layout, city);
    if (selected == "layout")
        return 0;
    struct Pose {
        const char* name;
        Player player;
    };
    const Pose poses[] = {{"arrival", city.spawn},
                          {"square", {{12, 8}, Heading::South}},
                          {"gate-street", {{12, 16}, Heading::South}},
                          {"gate-roofline", {{12, 12}, Heading::South}},
                          {"gate-west-join", {{10, 19}, Heading::East}},
                          {"gate-east-join", {{14, 19}, Heading::West}},
                          {"alley", {{3, 10}, Heading::East}},
                          {"courtyard", {{7, 3}, Heading::West}},
                          {"market", {{21, 10}, Heading::South}},
                          {"roofline", {{12, 14}, Heading::North}},
                          {"south-lane", {{11, 18}, Heading::West}}};
    for (const auto& pose : poses)
        if (city.map.movementBlocked(pose.player.cell))
            throw std::runtime_error(std::string("Blocked capture pose: ") + pose.name);
    const auto rules = loadRules(root / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    if (!rules.preset())
        throw std::runtime_error("City review requires the preset party");
    auto publication = world_assets::load(root / "prepared/world");
    if (publication.definitions != definitions.definitionFingerprint)
        throw std::runtime_error("City review definitions differ; rebuild world assets");
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
    report << "SC02 complete town; seed=5 supersampling=on native=1920x1080\n";
    {
        GameView view(renderer);
        CommandController commands(game, definitions.timings);
        unsigned captured = 0;
        const auto capture = [&](const std::string& name) {
            const bool moving = name.rfind("motion-", 0) == 0;
            const bool materialShot = name == "door-sanctuary" || name == "door-west-home" ||
                                      name == "door-city-gate" || name == "alley" || name == "roofline";
            if (!selected.empty() && name != selected && !(selected == "materials" && materialShot) &&
                !(selected == "stills" && !moving) && !(selected == "motion" && moving) &&
                !(selected == "gate-motion" && name.rfind("motion-gate-", 0) == 0))
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
        for (const auto& pose : poses) {
            game.restorePose(pose.player, true);
            capture(pose.name);
        }
        for (unsigned i = 0; i < city.doorCount; ++i) {
            game.restorePose(city.doors[i].approach, true);
            capture(std::string("door-") + city.doors[i].name);
        }
        // Walk both complete loops through the production movement system.
        // Export just selected translation/turn samples, keeping review work bounded.
        for (unsigned route = 0; route < city.loops.size(); ++route) {
            const auto& loop = city.loops[route];
            const auto start = loop.corners[0], next = loop.corners[1];
            const Heading initial = next.x > start.x   ? Heading::East
                                    : next.x < start.x ? Heading::West
                                    : next.y > start.y ? Heading::South
                                                       : Heading::North;
            game.restorePose({start, initial}, true);
            for (unsigned leg = 1; leg < loop.corners.size(); ++leg) {
                const auto end = loop.corners[leg];
                while (!(game.movement().player.cell == end)) {
                    const auto from = game.movement().player.cell;
                    const Heading heading = end.x > from.x   ? Heading::East
                                            : end.x < from.x ? Heading::West
                                            : end.y > from.y ? Heading::South
                                                             : Heading::North;
                    const auto current = game.movement().player.heading;
                    const Action action = current == heading ? Action::Forward
                                          : Heading((unsigned(current) + 1) % 4) == heading
                                              ? Action::TurnRight
                                              : Action::TurnLeft;
                    const bool first = from == loop.corners[leg - 1];
                    game.move(action);
                    if (game.movement().blocked)
                        throw std::runtime_error("Scripted city loop is blocked");
                    for (unsigned step = 1; step <= 4; ++step) {
                        game.update(.04f, false);
                        if (first && leg == 2)
                            capture("motion-loop-" + std::to_string(route) +
                                    (action == Action::Forward ? "-walk-" : "-turn-") + std::to_string(step));
                    }
                    if (game.movement().transition.active())
                        throw std::runtime_error("City route did not settle at expected movement duration");
                }
                capture("loop-" + std::to_string(route) + "-corner-" + std::to_string(leg));
            }
        }
        game.restorePose({{3, 10}, Heading::East}, true);
        for (unsigned i = 0; i < 5; ++i) {
            const Action action = i < 4 ? Action::Forward : Action::Back;
            game.move(action);
            if (game.movement().blocked)
                throw std::runtime_error("Alley movement is blocked");
            for (unsigned step = 1; step <= 4; ++step) {
                game.update(.04f, false);
                if (i == 1 || i == 4)
                    capture("motion-alley-" + std::to_string(i) + "-" + std::to_string(step));
            }
        }
        game.restorePose({{12, 19}, Heading::South}, true);
        for (unsigned i = 0; i < 2; ++i) {
            game.move(i ? Action::Right : Action::Left);
            if (game.movement().blocked)
                throw std::runtime_error("Gate approach strafe is blocked");
            for (unsigned step = 1; step <= 4; ++step) {
                game.update(.04f, false);
                capture("motion-gate-" + std::to_string(i) + "-" + std::to_string(step));
            }
        }
        if (!captured)
            throw std::runtime_error("Unknown review shot");
        const auto m = renderer.memory();
        const auto& pack = renderer.assets().pack("sanctuary-city");
        report << "placements=" << city.map.definition().buildings.size()
               << "/96 models=" << pack.buildings.size() << "/16 materials=" << pack.materialCount
               << "/32 doorways=" << city.doorCount << "/16 reservations=" << city.reservationCount
               << "/16\n";
        const auto* scene = renderer.scene();
        report << "captured_frames=" << captured << " scene_triangles=" << scene->indices.size() / 3
               << " world_cpu=" << renderer.cpuBytes() << " world_gpu=" << renderer.gpuBytes()
               << " visibility_bytes=" << renderer.visibility()->bytes() << '\n';
        report << "resident_textures=" << m.textureCount << " texture_bytes=" << m.residentTextures
               << " alpha_bytes=" << m.residentAlpha << " model_bytes=" << m.residentMeshes
               << " scene_cpu=" << scene->bytes() << " scene_gpu=" << scene->meshBytes()
               << " app_gpu=" << m.applicationGpu << " app_peak=" << m.peakApplicationGpu << '\n';
        report << "Driver allocations excluded. Peak includes temporary capture target.\n";
        const auto active = renderer.image();
        const auto resident = renderer.memory();
        {
            GameView staged(renderer);
            staged.update(game, animator.snapshot(), {});
            report << "active_plus_staged_cpu=" << renderer.cpuBytes()
                   << " active_plus_staged_gpu=" << renderer.gpuBytes() << '\n';
        }
        if (renderer.staged() || renderer.image().revision != active.revision ||
            renderer.memory().residentTextures != resident.residentTextures ||
            renderer.memory().residentMeshes != resident.residentMeshes)
            throw std::runtime_error("City staged cancellation changed the active view or residency");
        report << "city_stage_cancel_preserved_active=1\n";
    }
    if (renderer.memory().textureCount || renderer.assets().residency().meshes)
        throw std::runtime_error("Review retained location assets after view release");
    report << "released_world_textures=0 released_models=0\n";
    std::cout << "City review exported to " << output << '\n';
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}

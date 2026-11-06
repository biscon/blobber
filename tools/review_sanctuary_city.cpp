#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "review_dialogue.hpp"
#include "review_residents.hpp"
#include "sanctuary_city_layout.hpp"
#include "world/sanctuary_city.hpp"
#include <fstream>
#include <iostream>
using namespace blobber;
int main(int argc, char** argv) try {
    if (argc != 3 && argc != 4)
        throw std::runtime_error(
            "Usage: blobber_review_sanctuary_city ASSETS NEW_OUTPUT "
            "[SHOT|materials|layout|stills|motion|gate-motion|tree|dressing|water|"
            "corrections|fountain-orbit|distance-tone|inn-sign|residents|residents-idle]");
    const std::string selected = argc == 4 ? argv[3] : "";
    const std::filesystem::path root(argv[1]), output(argv[2]);
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Use a new output directory");
    if (selected == "dialogue-beats") {
        reviewDialogueBeats(root, output);
        return 0;
    }
    if (selected == "residents" || selected == "residents-idle") {
        reviewResidents(root, output, selected == "residents-idle");
        return 0;
    }
    const auto city = sanctuaryCity();
    std::ofstream layout(output / "layout.svg");
    writeCityLayout(layout, city);
    if (selected == "layout")
        return 0;
    struct Pose {
        const char* name;
        Player player;
    };
    const Pose poses[] = {{"fountain-close", {{11, 10}, Heading::South}},
                          {"fountain-west", {{9, 11}, Heading::East}},
                          {"fountain-north-facing", {{10, 13}, Heading::North}},
                          {"distance-south", {{12, 19}, Heading::North}},
                          {"distance-step", {{12, 18}, Heading::North}},
                          {"distance-corner", {{21, 21}, Heading::West}},
                          {"market-front", {{8, 14}, Heading::South}},
                          {"market-grain", {{9, 14}, Heading::South}},
                          {"east-garden", {{18, 20}, Heading::East}},
                          {"lane-garden", {{5, 14}, Heading::South}},
                          {"ivy-alley", {{9, 12}, Heading::West}},
                          {"ivy-courtyard", {{7, 16}, Heading::West}},
                          {"sign-inn", {{14, 6}, Heading::East}},
                          {"sign-smithy", {{13, 13}, Heading::East}},
                          {"sign-provisioner", {{14, 10}, Heading::East}},
                          {"sign-inn-near", {{16, 7}, Heading::North}},
                          {"sign-smithy-near", {{15, 14}, Heading::North}},
                          {"sign-provisioner-near", {{16, 7}, Heading::South}},
                          {"tree-square", {{12, 8}, Heading::West}},
                          {"tree-east", {{7, 4}, Heading::West}},
                          {"tree-west", {{2, 4}, Heading::East}},
                          {"tree-north", {{4, 2}, Heading::South}},
                          {"tree-south", {{4, 5}, Heading::North}},
                          {"tree-street", {{7, 5}, Heading::West}},
                          {"arrival", city.spawn},
                          {"square", {{12, 8}, Heading::South}},
                          {"gate-street", {{12, 16}, Heading::South}},
                          {"gate-roofline", {{12, 12}, Heading::South}},
                          {"gate-west-join", {{10, 19}, Heading::East}},
                          {"gate-east-join", {{14, 19}, Heading::West}},
                          {"alley", {{3, 10}, Heading::East}},
                          {"courtyard", {{7, 3}, Heading::West}},
                          {"market", {{9, 12}, Heading::South}},
                          {"former-market", {{21, 10}, Heading::South}},
                          {"inn-spacing", {{14, 8}, Heading::North}},
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
    report << "SC03 dressed city; seed=5 supersampling=on native=1920x1080\n";
    {
        GameView view(renderer);
        CommandController commands(game, definitions.timings);
        unsigned captured = 0;
        const auto capture = [&](const std::string& name) {
            const bool moving = name.rfind("motion-", 0) == 0;
            const bool materialShot = name == "door-sanctuary" || name == "door-west-home" ||
                                      name == "door-city-gate" || name == "alley" || name == "roofline";
            const bool dressingShot = name == "arrival" || name == "square" || name == "market" ||
                                      name == "tree-east" || name == "tree-south" ||
                                      name.rfind("fountain-", 0) == 0 || name.rfind("market-", 0) == 0 ||
                                      name.rfind("ivy-", 0) == 0 || name.rfind("sign-", 0) == 0 ||
                                      name == "east-garden" || name == "lane-garden" || name == "door-inn" ||
                                      name == "door-smithy" || name == "door-provisioner";
            const bool correctionShot =
                name == "square" || name == "market" || name == "market-front" || name == "market-grain" ||
                name == "former-market" || name == "inn-spacing" || name == "ivy-alley" ||
                name == "ivy-courtyard" || name == "lane-garden" || name == "east-garden" ||
                name == "tree-east" || name == "fountain-close" || name == "fountain-west";
            if (!selected.empty() && name != selected && !(selected == "dressing" && dressingShot) &&
                !(selected == "corrections" && correctionShot) &&
                !(selected == "inn-sign" &&
                  (name == "sign-inn" || name == "sign-inn-near" || name == "inn-spacing")) &&
                !(selected == "distance-tone" &&
                  (name.rfind("distance-", 0) == 0 || name == "fountain-north-facing" || name == "square")) &&
                !(selected == "fountain-orbit" && name.rfind("orbit-", 0) == 0) &&
                !(selected == "water" && name.rfind("water-", 0) == 0) &&
                !(selected == "materials" && materialShot) && !(selected == "stills" && !moving) &&
                !(selected == "motion" && moving) &&
                !(selected == "gate-motion" && name.rfind("motion-gate-", 0) == 0) &&
                !(selected == "tree" && (name.rfind("tree-", 0) == 0 || name.rfind("motion-tree-", 0) == 0)))
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
            report << name << " far_clip=" << renderer.assets().pack("sanctuary-city").viewDistance
                   << " cell=" << m.player.cell.x << ',' << m.player.cell.y
                   << " heading=" << headingName(m.player.heading) << " camera=" << camera.position.x << ','
                   << camera.position.y << ',' << camera.position.z << " yaw=" << camera.yaw
                   << " progress=" << m.transition.progress() << " draws=" << renderer.stats.draws << '\n';
        };
        for (const auto& pose : poses) {
            game.restorePose(pose.player, true);
            capture(pose.name);
        }
        if (selected == "distance-tone") {
            game.restorePose({{21, 21}, Heading::North}, true);
            game.move(Action::TurnLeft);
            game.update(.08f, false);
            capture("distance-oblique");
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
        game.restorePose({{7, 4}, Heading::West}, true);
        for (const auto action : {Action::Forward, Action::Back, Action::TurnRight}) {
            game.move(action);
            if (game.movement().blocked)
                throw std::runtime_error("Tree capture approach blocked");
            for (unsigned step = 1; step <= 4; ++step) {
                game.update(.04f, false);
                capture("motion-tree-" + std::to_string(unsigned(action)) + "-" + std::to_string(step));
            }
        }
        if (selected == "dressing") {
            game.restorePose({{10, 13}, Heading::South}, true);
            game.move(Action::TurnRight);
            game.update(.08f, false);
            capture("market-oblique");
            game.update(.08f, false);
            game.restorePose({{9, 10}, Heading::East}, true);
            game.move(Action::TurnRight);
            game.update(.08f, false);
            capture("fountain-turn");
            game.update(.08f, false);
        }
        if (selected == "fountain-orbit") {
            const Player orbit[] = {{{11, 9}, Heading::South},  {{13, 11}, Heading::West},
                                    {{11, 14}, Heading::North}, {{8, 11}, Heading::East},
                                    {{9, 9}, Heading::East},    {{13, 9}, Heading::South},
                                    {{13, 14}, Heading::West},  {{9, 14}, Heading::North}};
            for (unsigned i = 0; i < 8; ++i) {
                game.restorePose(orbit[i], true);
                if (i >= 4) {
                    game.move(Action::TurnRight);
                    game.update(.08f, false);
                }
                capture("orbit-" + std::to_string(i));
            }
        }
        if (selected == "water") {
            game.restorePose({{11, 10}, Heading::South}, true);
            for (unsigned phase = 0; phase < 8; ++phase) {
                capture("water-" + std::to_string(phase));
                report << "water_game_time=" << game.timeMs() << " phase=" << ((game.timeMs() / 125) % 8)
                       << " color_revision=" << renderer.image().revision
                       << " picking_revision=" << renderer.visibility()->frame().revision << '\n';
                if (phase < 7)
                    game.update(.125f, false);
            }
            const auto beforePause = game.timeMs();
            const auto pausedImage = renderer.image().revision;
            game.update(.5, true);
            capture("water-paused");
            if (game.timeMs() != beforePause || renderer.image().revision != pausedImage)
                throw std::runtime_error("Paused water changed clock or color image");
            report << "water_pause_clock_and_color_unchanged=1\n";
        }
        if (!captured)
            throw std::runtime_error("Unknown review shot");
        const auto m = renderer.memory();
        const auto& pack = renderer.assets().pack("sanctuary-city");
        report << "placements=" << city.map.definition().buildings.size()
               << "/96 scenery=" << city.map.definition().scenery.size()
               << "/32 models=" << pack.buildings.size() << "/24 materials=" << pack.materialCount
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

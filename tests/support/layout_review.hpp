#pragma once
#include "assets/rules_loader.hpp"
#include "generation/population.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/check.hpp"
#include <iostream>
namespace blobber::test {
// Explicit production-art review only. Routine correctness fixtures do not depend on game content.
inline void layoutReview(const std::filesystem::path& source, const std::filesystem::path& output) {
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Review output must be new");
    const auto catalog = loadRules(source / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(source / "npcs/level.json");
    Window window(1920, 1080, true);
    GpuResources resources;
    {
        Presenter presenter(resources, source);
        const auto baseline = resources.counts();
        {
            WorldRenderer renderer(resources, world_assets::load(source / "prepared/world"));
            auto show = [&](GameSession& game, Player pose, const std::string& name) {
                check(!game.geometry().solid(pose.cell) && !game.actors().occupied(pose.cell),
                      "Review pose has legal clearance");
                game.restorePose(pose, false);
                NpcAnimator animation(game.actors(), definitions.timings);
                animation.setTime(game.timeMs());
                GameView view(renderer);
                view.update(game, animation.snapshot(), {});
                CommandController commands(game, definitions.timings);
                commands.refresh(view.picking() ? view.picking()->targets() : TargetSnapshot{}, false, false,
                                 true);
                view.present(presenter, game, game.geometry().base(), false, true, &commands.snapshot());
                presenter.capture(output / (name + ".png"));
                using Error = unsigned (*)();
                check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
                      "Generated world draws without GL errors");
            };
            for (unsigned seed : {0u, 5u}) {
                const auto floor = generation::generate({seed, 3});
                check(bool(floor), "Review floor generated");
                const auto& f = floor.floor->layout;
                const auto population =
                    generation::preparePopulation(*floor.floor, catalog, definitions.types);
                GameSession game(floor.floor->map, catalog, population.placements, floor.floor->runtimeSeed,
                                 f.spawn, false, definitions.timings, population.recovery, f.fixtures,
                                 population.objective);
                const std::string prefix = std::string(generation::profileName(f.profile)) + "-";
                show(game, f.spawn, prefix + "landing");
                const auto& hall = f.rooms[0];
                Cell pose = hall.anchor;
                for (int y = hall.origin.y; y < hall.origin.y + hall.size.y; ++y)
                    for (int x = hall.origin.x; x < hall.origin.x + hall.size.x; ++x)
                        if (generation::regionAt(f, {x, y}) == 0 && !game.actors().occupied({x, y}) &&
                            x > hall.origin.x && y > hall.origin.y)
                            pose = {x, y};
                show(game, {pose, hall.size.x > hall.size.y ? Heading::West : Heading::North},
                     prefix + "hall");
                bool shared = false, bent = false;
                for (unsigned i = 0; i < f.connectionCount; ++i) {
                    const auto& e = f.connections[i];
                    if (!shared && e.count == 1) {
                        const auto p = e.socketA + forward(e.facingA) * -1;
                        if (!game.actors().occupied(p)) {
                            show(game, {p, e.facingA}, prefix + "shared-wall");
                            shared = true;
                        }
                    }
                    for (unsigned j = 2; !bent && j < e.count; ++j) {
                        const auto a = generation::pathCell(f, e, j - 2),
                                   b = generation::pathCell(f, e, j - 1), c = generation::pathCell(f, e, j);
                        if (b + a * -1 == c + b * -1 || game.geometry().solid(a) || game.actors().occupied(a))
                            continue;
                        for (unsigned h = 0; h < 4; ++h)
                            if (a + forward(Heading(h)) == b) {
                                show(game, {a, Heading(h)}, prefix + "bend");
                                bent = true;
                            }
                    }
                }
                check(bent, "Review includes a routed bend");
                const auto& door = f.fixtures.entries[f.approachDoor];
                show(game, {door.cell + forward(door.facing) * -1, door.facing},
                     prefix + "guardian-threshold");
            }
            std::cout << "Generated layout hidden captures: CPU bytes=" << renderer.cpuBytes()
                      << " GPU bytes=" << renderer.gpuBytes() << '\n';
        }
        const auto counts = resources.counts();
        check(counts.textures == baseline.textures && counts.targets == baseline.targets &&
                  counts.meshes == baseline.meshes && counts.shaders == baseline.shaders,
              "Review world leases return to the persistent presenter baseline");
    }
    // Presenter/UI resources belong to the registry; its destructor runs before Window's.
    std::cout << "Generated layout review passed; world resources released.\n";
}
} // namespace blobber::test

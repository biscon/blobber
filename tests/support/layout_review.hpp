#pragma once
#include "assets/image.hpp"
#include "assets/rules_loader.hpp"
#include "generation/architecture.hpp"
#include "generation/population.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/check.hpp"
#include <iostream>
namespace blobber::test {
// Explicit production-art review only. Routine correctness fixtures do not depend on game content.
inline void layoutReview(const std::filesystem::path& source, const std::filesystem::path& output,
                         bool ceilings = false) {
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
            GpuLease<TargetHandle> ids(resources, resources.createTarget(viewWidth, viewHeight));
            auto show = [&](GameSession& game, Player pose, const std::string& name) {
                check(!game.geometry().solid(pose.cell) && !game.actors().occupied(pose.cell),
                      "Review pose has legal clearance");
                game.restorePose(pose, false);
                if (ceilings)
                    game.discover();
                NpcAnimator animation(game.actors(), definitions.timings);
                animation.setTime(game.timeMs());
                GameView view(renderer);
                view.update(game, animation.snapshot(), {});
                CommandController commands(game, definitions.timings);
                commands.refresh(view.picking() ? view.picking()->targets() : TargetSnapshot{}, false, false,
                                 true);
                view.present(presenter, game, game.geometry().base(), false, true, &commands.snapshot());
                presenter.capture(output / (name + ".png"));
                if (ceilings) {
                    renderer.diagnosticIds(ids.get());
                    OwnedImage raster(LoadImageFromTexture(resources.target(ids.get()).texture));
                    check(raster.value.data, "Ceiling review GPU ID readback");
                    ImageFlipVertical(&raster.value);
                    ImageFormat(&raster.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                    const auto* pixels = static_cast<const unsigned char*>(raster.value.data);
                    const auto& picking = renderer.visibility()->frame().picking;
                    unsigned interior = 0;
                    for (int y = 1; y < viewHeight - 1; ++y)
                        for (int x = 1; x < viewWidth - 1; ++x) {
                            const auto at = (y * viewWidth + x) * 4;
                            const auto actual = std::uint16_t(pixels[at] + 256 * pixels[at + 1]);
                            const auto expected = picking.slot(x, y);
                            if (actual == expected)
                                continue;
                            bool edge = false;
                            for (int dy = -1; dy <= 1; ++dy)
                                for (int dx = -1; dx <= 1; ++dx)
                                    edge |= picking.slot(x + dx, y + dy) != expected;
                            interior += !edge;
                        }
                    check(interior < 100, "Ceiling review CPU/GPU interior picking agreement");
                }
                using Error = unsigned (*)();
                check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
                      "Generated world draws without GL errors");
            };
            std::array<bool, 9> capturedTiers{};
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
                if (ceilings) {
                    // Isolated architectural review: retain dressing, remove actors from this owned session.
                    for (unsigned i = 0; i < game.actors().snapshot().count; ++i)
                        game.actors().remove(game.actors().id(i));
                    game.setAnimated(false);
                    show(game, f.spawn, prefix + "stairs");
                    const auto heights = generation::roomCeilings(f);
                    for (unsigned room = 0; room < f.roomCount; ++room) {
                        const auto height = unsigned(heights[room]);
                        if (height < 5 || capturedTiers[height])
                            continue;
                        const auto& r = f.rooms[room];
                        // Stand near a room boundary looking along its longer dimension.
                        bool found = false;
                        for (int y = r.origin.y; y < r.origin.y + r.size.y && !found; ++y)
                            for (int x = r.origin.x; x < r.origin.x + r.size.x && !found; ++x) {
                                const Cell c{x, y};
                                const auto h = r.size.y >= r.size.x ? Heading::South : Heading::East;
                                if (generation::regionAt(f, c) != room ||
                                    game.geometry().movementBlocked(c) ||
                                    generation::regionAt(f, c + forward(h)) != room ||
                                    game.geometry().movementBlocked(c + forward(h)))
                                    continue;
                                show(game, {c, h}, prefix + "ceiling-" + std::to_string(height));
                                capturedTiers[height] = found = true;
                            }
                    }
                    const auto& door = f.fixtures.entries[f.approachDoor];
                    const Player pose{door.cell + forward(door.facing) * -1, door.facing};
                    show(game, pose, prefix + "door-closed");
                    check(bool(game.interact(game.nearbyFixture())),
                          "Architectural threshold opens normally");
                    show(game, pose, prefix + "door-open");
                    show(game, {door.cell, door.facing}, prefix + "threshold");
                    continue;
                }
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
            if (ceilings)
                check(capturedTiers[5] && capturedTiers[6] && capturedTiers[7] && capturedTiers[8],
                      "Review covers every room ceiling tier");
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

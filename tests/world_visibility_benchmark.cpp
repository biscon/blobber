#include "generation/floor.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include "support/world_visibility_fixtures.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
using namespace blobber;
int main(int argc, char**) {
    try {
        test::VisibilityFixture fixture;
        world_assets::CpuAssets assets(fixture.publication());
        const auto catalog = creationCatalog(true);
        const auto generated = generation::generate({5, 1});
        check(bool(generated), "Benchmark generation");
        const auto& map = generated.floor->map;
        const auto spawn = generated.floor->layout.spawn;
        std::vector<NpcInstance> placements;
        const unsigned capacity = argc > 1 ? 256 : 32;
        for (int z = 1; z < map.height() - 1 && placements.size() < capacity; ++z)
            for (int x = 1; x < map.width() - 1 && placements.size() < capacity; ++x)
                if (!map.solid({x, z}) && !(Cell{x, z} == spawn.cell))
                    placements.push_back({{0}, {x, z}, {0, 0}, Heading::South});
        const NpcWorld npcs(map, spawn.cell, {"goblin"}, placements);
        ActorWorld actors(map, catalog, npcs);
        auto scene = world_render::prepare(map);
        world_render::VisibilityPipeline pipeline(scene, assets);
        NpcAnimationSnapshot animation;
        animation.count = actors.snapshot().count;
        animation.world = actors.snapshot().world;
        animation.actorRevision = actors.snapshot().revision;
        MovementState movement;
        movement.player = spawn;
        using Clock = std::chrono::steady_clock;
        std::array<double, 600> times{};
        std::cout << "{\"actors\":" << placements.size() << ",\"scratch_bytes\":" << pipeline.bytes()
                  << ",\"trials\":[";
        for (unsigned trial = 0; trial < 3; ++trial) {
            for (unsigned i = 0; i < 720; ++i) {
                movement.transition = {spawn,
                                       {spawn.cell, Heading((unsigned(spawn.heading) + 1) % 4)},
                                       Action::TurnRight,
                                       float(i % 16) * .01f,
                                       .16f};
                animation.revision++;
                const auto begin = Clock::now();
                pipeline.update({map, movement, &actors, &animation});
                if (i >= 120)
                    times[i - 120] = std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
            }
            std::sort(times.begin(), times.end());
            if (trial)
                std::cout << ',';
            std::cout << "{\"p50_ms\":" << times[299] << ",\"p95_ms\":" << times[569]
                      << ",\"p99_ms\":" << times[593] << '}';
        }
        std::cout << "]}\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

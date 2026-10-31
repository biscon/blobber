#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include "support/probe_allocations.hpp"
#include "support/world_visibility_fixtures.hpp"
#include <iostream>
using namespace blobber;
int main() {
    try {
        test::VisibilityFixture fixture;
        world_assets::CpuAssets assets(fixture.publication());
        const auto catalog = creationCatalog(true);
        const auto map = test::worldRoom(256, 32);
        const Player spawn{{128, 28}, Heading::North};
        std::vector<NpcInstance> instances;
        for (unsigned i = 0; i < NpcWorld::maxInstances; ++i)
            instances.push_back({{0}, {int(1 + i % 128), int(20 + i / 128)}, {0, 0}, Heading::South});
        const NpcWorld placements(map, spawn.cell, {"goblin"}, instances);
        ActorWorld actors(map, catalog, placements);
        NpcAnimationSnapshot animation;
        animation.world = actors.snapshot().world;
        animation.actorRevision = actors.snapshot().revision;
        animation.count = instances.size();
        LootSnapshot loot;
        loot.world = 8;
        for (unsigned i = 0; i < lootCapacity; ++i)
            loot.stacks[i] = {{int(1 + i % 254), int(13 + i / 254)}, {0}, 1, true, 1};
        FloorFixtures fixtures;
        fixtures.count = fixtureCapacity;
        for (unsigned i = 0; i < fixtureCapacity; ++i)
            fixtures.entries[i] = {{int(200 + i), 24}, Heading::South, FixtureKind::Endpoint, 0};
        FloorGeometry geometry(map, fixtures);
        auto scene = world_render::prepare(map);
        world_render::VisibilityPipeline pipeline(scene, assets);
        MovementState movement;
        movement.player = spawn;
        pipeline.update({geometry.view(), movement, &actors, &animation, &loot});
        check(pipeline.frame().count == world_render::billboardCapacity,
              "All maximum-capacity candidates fit without truncation");
        watching = true;
        allocations = 0;
        for (unsigned i = 0; i < 4; ++i) {
            animation.revision++;
            pipeline.update({geometry.view(), movement, &actors, &animation, &loot});
        }
        watching = false;
        check(allocations == 0, "Full-capacity presentation has no frame C++ allocations");
        check(pipeline.bytes() < world_render::viewScratchBudget, "Full-capacity storage remains bounded");
        std::cout << "Visibility capacity stress passed: " << pipeline.frame().count << " candidates; "
                  << pipeline.bytes() << " scratch bytes\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}

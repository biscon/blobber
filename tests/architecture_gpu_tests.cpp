#include "assets/image.hpp"
#include "platform/window.hpp"
#include "platform/world_renderer.hpp"
#include "support/allocations.hpp"
#include "support/architecture_fixtures.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include <iostream>
using namespace blobber;
unsigned compare(GpuResources& resources, WorldRenderer& renderer, TargetHandle target) {
    renderer.diagnosticIds(target);
    OwnedImage image(LoadImageFromTexture(resources.target(target).texture));
    check(image.value.data, "GPU readback available");
    ImageFlipVertical(&image.value);
    ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const auto* pixels = static_cast<const unsigned char*>(image.value.data);
    const auto& cpu = renderer.visibility()->frame().picking;
    unsigned differences = 0, interior = 0;
    for (int y = 0; y < viewHeight; ++y)
        for (int x = 0; x < viewWidth; ++x) {
            const auto at = 4 * (y * viewWidth + x);
            const auto gpu = std::uint16_t(pixels[at] + 256 * pixels[at + 1]);
            if (gpu == cpu.slot(x, y))
                continue;
            ++differences;
            bool edge = false;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    edge |= cpu.slot(x + dx, y + dy) == gpu;
            interior += !edge;
        }
    check(interior == 0, "Architecture CPU/GPU interiors agree within one-pixel edge tolerance");
    return differences;
}
int main() try {
    test::ArchitectureFixture fixture;
    auto assets = std::make_shared<const world_assets::CpuAssets>(fixture.publication());
    const auto catalog = creationCatalog(true);
    const Map map(test::architectureRows(), test::architectureWorld());
    const NpcWorld placements(map, {7, 4}, {"goblin"},
                              {{{0}, {7, 8}, {0, 0}, Heading::North}, {{0}, {9, 7}, {0, 0}, Heading::North}});
    ActorWorld actors(map, catalog, placements);
    std::vector<NpcTimings> timings(1);
    timings[0][0] = {{{100, 0, 0, 0}}, 1};
    NpcAnimator animator(actors, timings);
    MovementState movement;
    movement.player = {{7, 4}, Heading::South};
    auto input = [&] {
        animator.setTime(0);
        return world_render::FrameInput{map, movement, &actors, &animator.snapshot()};
    };
    Window window(640, 360, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "GPU window stays hidden");
    GpuResources resources;
    {
        WorldRenderer renderer(resources, assets);
        GpuLease<TargetHandle> ids(resources, resources.createTarget(viewWidth, viewHeight));
        renderer.stage(input());
        renderer.commit();
        renderer.render(input());
        check(!renderer.picking()->exposed(0) && renderer.picking()->exposed(1),
              "Shell hides rear actor and exposes street actor");
        unsigned differences = compare(resources, renderer, ids.get());
        const auto image = renderer.image();
        const auto resident = assets->residency();
        world_render::Limits tiny;
        tiny.quads = 1;
        rejects([&] { renderer.stage(map, movement, {}, {}, tiny); }, "Failed architecture staging rejects");
        check(renderer.image().revision == image.revision && assets->residency().meshes == resident.meshes,
              "Failed stage preserves active image and dependencies");
        for (unsigned i = 0; i < 3; ++i) {
            renderer.stage(input());
            renderer.cancel();
            check(assets->residency().meshes == 2, "Cancel retains shared active parts");
        }
        for (const Player p :
             {Player{{7, 3}, Heading::South}, Player{{5, 7}, Heading::East}, Player{{9, 7}, Heading::West}}) {
            movement.player = p;
            renderer.render(input());
            differences += compare(resources, renderer, ids.get());
        }
        movement.player = {{7, 4}, Heading::South};
        renderer.render(input());
        const auto reads = assets->residency().reads;
        // Warm moving and settled paths before guarding engine-owned CPU allocation.
        movement.transition = {movement.player, {{6, 4}, Heading::South}, Action::Right, .08f, .16f};
        renderer.render(input());
        movement.transition = {};
        renderer.render(input());
        const auto uploads = resources.transfers().uploads;
        allocations = 0;
        watching = true;
        for (unsigned i = 0; i < 6; ++i) {
            movement.transition = {
                movement.player, {{6, 4}, Heading::South}, Action::Right, .04f + .01f * i, .16f};
            renderer.render(input());
            movement.transition = {};
            renderer.render(input());
        }
        watching = false;
        check(allocations == 0 && assets->residency().reads == reads &&
                  resources.transfers().uploads == uploads,
              "Warmed architecture frames allocate/load/upload nothing");
        renderer.clear();
        check(!assets->residency().meshes && !assets->residency().alphaBytes &&
                  !renderer.memory().textureCount,
              "Clearing view releases architecture assets");
        const auto baseline = resources.counts();
        renderer.stage(input());
        renderer.commit();
        renderer.clear();
        const auto again = resources.counts();
        check(again.textures == baseline.textures && again.meshes == baseline.meshes &&
                  again.targets == baseline.targets,
              "Revisit returns to retained renderer resource baseline");
        std::cout << "Architecture GPU checks passed; boundary pixel differences=" << differences << '\n';
    }
    const auto end = resources.counts();
    check(!end.textures && !end.targets && !end.meshes && !end.shaders, "GPU owners release before window");
} catch (const std::exception& e) {
    watching = false;
    std::cerr << e.what() << '\n';
    return 1;
}

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
    for (unsigned mode = 0; mode < 4; ++mode) {
        const bool gate = mode == 1, foliage = mode == 2, water = mode == 3;
        test::ArchitectureFixture fixture(foliage, water);
        auto publication = gate ? test::gatePublication(fixture.publication()) : fixture.publication();
        auto assets = std::make_shared<const world_assets::CpuAssets>(std::move(publication));
        const auto catalog = creationCatalog(true);
        auto scenery = test::sceneryWorld();
        if (water)
            scenery.scenery[0].model = "water-screen";
        const Map map(foliage || water ? test::sceneryRows()
                      : gate           ? test::gateRows()
                                       : test::architectureRows(),
                      foliage || water ? scenery
                      : gate           ? test::gateWorld()
                                       : test::architectureWorld());
        const NpcWorld placements(map, {7, 4}, {"goblin"},
                                  {{{0}, gate ? Cell{7, 10} : Cell{7, 8}, {0, 0}, Heading::North},
                                   {{0}, gate ? Cell{10, 6} : Cell{9, 7}, {0, 0}, Heading::North}});
        ActorWorld actors(map, catalog, placements);
        std::vector<NpcTimings> timings(1);
        timings[0][0] = {{{100, 0, 0, 0}}, 1};
        NpcAnimator animator(actors, timings);
        MovementState movement;
        movement.player = {gate ? Cell{7, 2} : Cell{7, 4}, Heading::South};
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
            check(renderer.picking()->exposed(0) == foliage && renderer.picking()->exposed(1),
                  "Shell blocks rear actor; foliage opening exposes it; street actor remains visible");
            unsigned differences = compare(resources, renderer, ids.get());
            const auto image = renderer.image();
            const auto resident = assets->residency();
            world_render::Limits tiny;
            tiny.quads = 1;
            rejects([&] { renderer.stage(map, movement, {}, {}, tiny); },
                    "Failed architecture staging rejects");
            check(renderer.image().revision == image.revision &&
                      assets->residency().meshes == resident.meshes,
                  "Failed stage preserves active image and dependencies");
            for (unsigned i = 0; i < 3; ++i) {
                renderer.stage(input());
                renderer.cancel();
                check(assets->residency().meshes == (foliage || water ? 3u : 2u),
                      "Cancel retains shared active parts");
            }
            for (const Player p : {Player{{7, 3}, Heading::South}, Player{{4, 5}, Heading::East},
                                   Player{{10, 5}, Heading::West}, Player{{7, 9}, Heading::North}}) {
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
            if (water) {
                auto frame = input();
                frame.now = 0;
                renderer.render(frame);
                const auto pickingRevision = renderer.visibility()->frame().revision;
                const auto first = renderer.image().revision;
                auto sample = [&] {
                    OwnedImage image(LoadImageFromTexture(resources.target(renderer.image().target).texture));
                    check(image.value.data, "Water color readback available");
                    ImageFlipVertical(&image.value);
                    ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                    const auto p =
                        world_render::project(renderer.visibility()->frame().camera, {7.5f, .8f, 6.5f});
                    const auto* rgba = static_cast<const unsigned char*>(image.value.data);
                    const auto at = 4 * (int(p.y) * viewWidth + int(p.x));
                    return std::array<unsigned char, 3>{rgba[at], rgba[at + 1], rgba[at + 2]};
                };
                const auto firstColor = sample();
                frame.now = 124;
                renderer.render(frame);
                check(renderer.stats.cached && renderer.image().revision == first,
                      "Water keeps color cache within its 125ms phase");
                frame.now = 125;
                renderer.render(frame);
                check(!renderer.stats.cached && renderer.image().revision != first && sample() != firstColor,
                      "Visible water advances painted color at the phase boundary");
                check(renderer.visibility()->frame().revision == pickingRevision,
                      "Water color does not invalidate CPU visibility or picking");
                const auto phaseOne = renderer.image().revision;
                renderer.render(frame);
                check(renderer.stats.cached && renderer.image().revision == phaseOne,
                      "Paused gameplay clock retains water image");
                frame.now = 999;
                renderer.render(frame);
                check(sample() != firstColor, "Last water frame is selected before wrap");
                frame.now = 1000;
                renderer.render(frame);
                check(sample() == firstColor, "Eight water phases wrap exactly after one second");
                differences += compare(resources, renderer, ids.get());
                allocations = 0;
                watching = true;
                for (unsigned phase = 0; phase < 8; ++phase) {
                    frame.now = phase * 125;
                    renderer.render(frame);
                }
                watching = false;
                check(!allocations && assets->residency().reads == reads &&
                          resources.transfers().uploads == uploads,
                      "Animated water allocates, reads and uploads nothing in warmed frames");
                movement.player = {{7, 9}, Heading::South};
                frame.now = 0;
                renderer.render(frame);
                const auto away = renderer.image().revision;
                frame.now = 125;
                renderer.render(frame);
                check(renderer.stats.cached && renderer.image().revision == away,
                      "Water outside culled ranges does not invalidate color");
            }
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
            std::cout << (water     ? "Animated water"
                          : foliage ? "Cutout scenery"
                          : gate    ? "Wide gate"
                                    : "Architecture")
                      << " GPU checks passed; boundary pixel differences=" << differences << '\n';
        }
        const auto end = resources.counts();
        check(!end.textures && !end.targets && !end.meshes && !end.shaders,
              "GPU owners release before window");
    }
} catch (const std::exception& e) {
    watching = false;
    std::cerr << e.what() << '\n';
    return 1;
}

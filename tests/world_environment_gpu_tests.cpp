#include "assets/image.hpp"
#include "platform/window.hpp"
#include "platform/world_renderer.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include "support/environment_fixtures.hpp"
#include "support/world_visibility_fixtures.hpp"
#include <iostream>
using namespace blobber;
namespace {
unsigned compare(GpuResources& r, WorldRenderer& renderer, TargetHandle ids) {
    renderer.diagnosticIds(ids);
    OwnedImage image(LoadImageFromTexture(r.target(ids).texture));
    check(image.value.data, "Diagnostic ID readback");
    ImageFlipVertical(&image.value);
    ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const auto* pixels = static_cast<const unsigned char*>(image.value.data);
    const auto& picking = renderer.visibility()->frame().picking;
    std::array<bool, PickingBuffer::fixtureBase + fixtureCapacity> cpu{}, gpu{};
    unsigned mismatch = 0, interior = 0;
    for (int y = 0; y < viewHeight; ++y)
        for (int x = 0; x < viewWidth; ++x) {
            const auto p = (y * viewWidth + x) * 4;
            const auto actual = std::uint16_t(pixels[p] + 256 * pixels[p + 1]), expected = picking.slot(x, y);
            if (actual < gpu.size())
                gpu[actual] = true;
            if (expected < cpu.size())
                cpu[expected] = true;
            if (actual == expected)
                continue;
            ++mismatch;
            bool edge = false;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    edge |= picking.slot(x + dx, y + dy) == actual;
            if (!edge)
                ++interior;
        }
    if (interior)
        std::cerr << "Interior mismatches " << interior << " / total " << mismatch << '\n';
    check(interior == 0, "CPU/GPU ID interiors agree within one-pixel geometric boundary");
    check(cpu == gpu, "CPU/GPU exact visible object membership");
    return mismatch;
}
} // namespace

int main(int argc, char** argv) {
    try {
        test::VisibilityFixture fixture;
        auto assets = std::make_shared<const world_assets::CpuAssets>(fixture.publication());
        const auto catalog = creationCatalog(true);
        const auto map = test::rendererHeightMap();
        const auto hub = sanctuaryMap();
        const NpcWorld placements(map, {3, 5}, {"goblin"}, {{{0}, {3, 2}, {0, 0}, Heading::South}});
        ActorWorld actors(map, catalog, placements);
        NpcAnimator animator(actors, fixture.timings);
        MovementState movement;
        movement.player = {{3, 4}, Heading::North};
        Window window(640, 360, true);
        check(IsWindowState(FLAG_WINDOW_HIDDEN), "GPU fixture remains hidden");
        GpuResources resources;
        {
            WorldRenderer renderer(resources, assets);
            GpuLease<TargetHandle> ids(resources, resources.createTarget(viewWidth, viewHeight));
            SpellArea area;
            area.ground.revision = 1;
            area.ground.areas[0].active = true;
            area.ground.areas[0].cell = {3, 3};
            area.ground.weapons[0] = {{3, 3}, true};
            auto input = [&] {
                animator.setTime(0);
                return world_render::FrameInput{map,     movement, &actors, &animator.snapshot(),
                                                nullptr, nullptr,  nullptr, &area};
            };
            renderer.stage(input());
            renderer.commit();
            unsigned differences = 0;
            for (bool quality : {false, true}) {
                renderer.supersampling(quality);
                for (unsigned h = 0; h < 4; ++h) {
                    movement.player.heading = Heading(h);
                    renderer.render(input());
                    if (argc == 2 && quality && h < 2) {
                        const std::filesystem::path output(argv[1]);
                        std::filesystem::create_directories(output);
                        OwnedImage capture(LoadImageFromTexture(resources.target(renderer.image().target).texture));
                        ImageFlipVertical(&capture.value);
                        check(ExportImage(capture.value, (output / ("height-fixture-" + std::to_string(h) + ".png")).string().c_str()),
                              "Export optional test-owned height review");
                    }
                    differences += compare(resources, renderer, ids.get());
                }
                movement.player.heading = Heading::North;
                movement.transition = {
                    movement.player, {{3, 3}, Heading::North}, Action::Forward, .08f, .16f};
                renderer.render(input());
                differences += compare(resources, renderer, ids.get());
                movement.transition.elapsed = .16f;
            }
            const auto image = renderer.image();
            const auto* picking = renderer.picking();
            resources.failAfter(0);
            rejects([&] { renderer.stage(input()); }, "Height scene staged failure");
            resources.failAfter(-1);
            check(renderer.image().revision == image.revision && renderer.picking() == picking,
                  "Failed height scene retains image and IDs");
            const auto before = resources.transfers();
            watching = true;
            allocations = 0;
            for (unsigned i = 0; i < 60; ++i) {
                movement.player.heading = Heading(i % 4);
                renderer.supersampling(i % 2);
                renderer.render(input());
            }
            watching = false;
            check(allocations == 0, "Warmed height GPU frames allocate no tracked C++ storage");
            check(resources.transfers().uploads == before.uploads, "No height frame resource uploads");
            renderer.clear();
            MovementState outdoor;
            outdoor.player = {{3, 4}, Heading::North};
            renderer.stage(world_render::FrameInput{hub, outdoor});
            renderer.commit();
            renderer.render(world_render::FrameInput{hub, outdoor});
            check(renderer.cpuBytes() < 352u * world_assets::MiB &&
                      renderer.gpuBytes() < 352u * world_assets::MiB,
                  "Extended scene budgets retained");
            std::cout << "Height CPU/GPU edge differences: " << differences
                      << "; allocations: " << allocations << '\n';
        }
        const auto counts = resources.counts();
        check(counts.textures == 0 && counts.targets == 0 && counts.meshes == 0 && counts.shaders == 0,
              "Height resources released before context");
        return 0;
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}

#include "assets/image.hpp"
#include "platform/window.hpp"
#include "platform/world_renderer.hpp"
#include "rlgl.h"
#include "support/check.hpp"
#include "support/probe_allocations.hpp"
#include "support/world_fixtures.hpp"
#include <cmath>
#include <iostream>
using namespace blobber;
namespace {
std::vector<unsigned char> pixels(GpuResources& r, WorldImage image) {
    OwnedImage im(LoadImageFromTexture(r.target(image.target).texture));
    check(im.value.data, "Read logical output");
    ImageFlipVertical(&im.value);
    ImageFormat(&im.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    auto* p = static_cast<unsigned char*>(im.value.data);
    return {p, p + 500 * 282 * 4};
}
bool same(GpuCounts a, GpuCounts b) {
    return a.textures == b.textures && a.targets == b.targets && a.meshes == b.meshes &&
           a.shaders == b.shaders && a.bytes == b.bytes;
}
} // namespace
int main() {
    try {
        test::WorldFixture fixture;
        const auto map = test::worldRoom();
        Window window(640, 360, true);
        SetTargetFPS(0);
        ClearWindowState(FLAG_VSYNC_HINT);
        check(IsWindowState(FLAG_WINDOW_HIDDEN), "GPU tests remain hidden");
        GpuResources resources;
        const auto baseline = resources.counts();
        for (int failure = 0; failure < 7; ++failure) {
            resources.failAfter(failure);
            rejects([&] { WorldRenderer renderer(resources, world_assets::load(fixture.directory)); },
                    "Shared upload fault rejects");
            check(same(resources.counts(), baseline), "Shared upload rollback releases resources");
        }
        resources.failAfter(-1);
        std::vector<unsigned char> reference;
        for (unsigned cycle = 0; cycle < 2; ++cycle) {
            WorldRenderer renderer(resources, world_assets::load(fixture.directory));
            MovementState state;
            state.player = {{1, 1}, Heading::West};
            renderer.stage(map, state);
            check(!renderer.image().target, "Staging does not publish");
            renderer.commit();
            auto image = renderer.render(state);
            check(renderer.stats.cached, "Prepared first image reused");
            auto result = pixels(resources, image);
            const double d = .9, light = 1 / (1 + .115 * d + .015 * d * d);
            const auto i = (100 * 500 + 250) * 4;
            check(std::abs(int(result[i]) - 200 * light) <= 2 &&
                      std::abs(int(result[i + 1]) - 100 * light) <= 2,
                  "Shader matches independent adjacent-wall depth oracle");
            {
                OwnedImage high(
                    LoadImageFromTexture(resources.target(renderer.supersampledTarget()).texture));
                ImageFlipVertical(&high.value);
                ImageFormat(&high.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                const auto* source = static_cast<const unsigned char*>(high.value.data);
                unsigned varied = 0;
                for (unsigned y = 0; y < 282; ++y)
                    for (unsigned x = 0; x < 500; ++x)
                        for (unsigned c = 0; c < 4; ++c) {
                            const auto p = (y * 2 * 1000 + x * 2) * 4 + c;
                            const unsigned sum =
                                source[p] + source[p + 4] + source[p + 4000] + source[p + 4004];
                            check(std::abs(int(result[(y * 500 + x) * 4 + c]) - int((sum + 2) / 4)) <= 1,
                                  "Resolved pixels match four source samples including nonuniform edges");
                            varied += std::abs(int(source[p]) - int(source[p + 4004])) > 2;
                        }
                check(varied > 100, "Resolve oracle exercises nonuniform samples");
            }
            if (cycle)
                check(reference == result, "Independent lifetimes produce identical output");
            else
                reference = result;
            const auto resident = resources.counts();
            for (int failure = 0; failure < 5; ++failure) {
                resources.failAfter(failure);
                rejects([&] { renderer.stage(map, state); }, "Staged acquisition rejects");
                resources.failAfter(-1);
                check(!renderer.staged() && same(resident, resources.counts()),
                      "Staged rollback releases only candidate resources");
                check(pixels(resources, renderer.image()) == result,
                      "Failed staging preserves active pixels");
            }
            unsigned cpuFailures = 0;
            for (long point = 0; point < 10; ++point) {
                allocationsUntilFailure = point;
                try {
                    renderer.stage(map, state);
                } catch (const std::bad_alloc&) {
                    ++cpuFailures;
                }
                allocationsUntilFailure = -1;
                renderer.cancel();
                check(same(resident, resources.counts()) && pixels(resources, renderer.image()) == result,
                      "CPU preparation failure retains active view and releases candidate");
            }
            check(cpuFailures >= 5, "Injection reaches scene owner and all contiguous geometry allocations");
            MovementState destination;
            destination.player = {{12, 12}, Heading::East};
            renderer.stage(map, destination);
            check(pixels(resources, renderer.image()) == result,
                  "Successful staging leaves active logical image intact");
            rejects([&] { renderer.stage(map, state); }, "Third scene rejected");
            renderer.cancel();
            check(same(resident, resources.counts()), "Cancellation restores scene budget");
            const auto old = renderer.image().target;
            renderer.stage(map, destination);
            renderer.commit();
            rejects([&] { resources.target(old); }, "Retired scene target is stale");
            const auto beforeSwitch = resources.counts();
            renderer.supersampling(false);
            auto native = renderer.render(destination);
            check(!renderer.stats.cached, "Quality invalidates settled image");
            renderer.render(destination);
            check(renderer.stats.cached, "Quality change invalidates once");
            renderer.supersampling(true);
            renderer.render(destination);
            check(same(beforeSwitch, resources.counts()), "Quality switch creates no resources");
            // First/subsequent movement, idle and quality paths are allocation-free after preparation.
            const auto transfersBefore = resources.transfers();
            Controller controller(destination.player);
            watching = true;
            for (unsigned pass = 0; pass < 8; ++pass) {
                renderer.supersampling(pass % 2);
                controller.submit(map, pass % 2 ? Action::TurnRight : Action::TurnLeft);
                for (unsigned tick = 0; tick < 17; ++tick) {
                    renderer.render(controller.state());
                    controller.update(map, .01f);
                }
            }
            watching = false;
            check(allocations == 0, "World GPU orchestration allocates no C++ storage");
            check(resources.transfers().uploads == transfersBefore.uploads &&
                      resources.transfers().bytes == transfersBefore.bytes,
                  "Frame processing performs no mesh or texture uploads");
            const auto ui = resources.createTarget(32, 32);
            BeginTextureMode(resources.target(ui));
            ClearBackground(BLACK);
            DrawRectangle(0, 0, 16, 32, RED);
            DrawRectangle(16, 0, 16, 32, GREEN);
            EndTextureMode();
            OwnedImage restored(LoadImageFromTexture(resources.target(ui).texture));
            check(GetImageColor(restored.value, 8, 8).r == RED.r &&
                      GetImageColor(restored.value, 24, 8).g == GREEN.g,
                  "World restores 2D shader/viewport/depth state");
            resources.destroy(ui);
            (void)native;
        }
        check(same(resources.counts(), baseline), "All scene/shared resources released");
        {
            world_assets::Writer writer(fixture.directory);
            std::vector<unsigned char> pattern(8 * 8 * 4);
            for (unsigned y = 0; y < 8; ++y)
                for (unsigned x = 0; x < 8; ++x) {
                    const auto i = (y * 8 + x) * 4;
                    pattern[i] = y < 4 ? 255 : 0;
                    pattern[i + 2] = y < 4 ? 0 : 255;
                    pattern[i + 3] = 255;
                }
            for (const auto* name : {"wall", "floor", "ceiling"})
                writer.add(name, 8, 8, false, pattern);
            writer.finish({});
            WorldRenderer renderer(resources, world_assets::load(fixture.directory));
            MovementState pose;
            pose.player = {{1, 1}, Heading::West};
            renderer.stage(map, pose);
            renderer.commit();
            for (bool quality : {false, true}) {
                renderer.supersampling(quality);
                const auto image = pixels(resources, renderer.render(pose));
                check(image[(50 * 500 + 250) * 4] > 200 && image[(50 * 500 + 250) * 4 + 2] < 2 &&
                          image[(200 * 500 + 250) * 4] < 2 && image[(200 * 500 + 250) * 4 + 2] > 200,
                      "Asymmetric source orientation survives native and resolved target paths");
            }
        }
        {
            const auto cpu = world_render::prepare(map);
            const auto handle = resources.createMesh(cpu);
            GpuResources foreign;
            rejects([&] { foreign.mesh(handle); }, "Foreign mesh handles reject");
            check(resources.destroy(handle) && !resources.destroy(handle), "Mesh release is exactly once");
            const auto next = resources.createMesh(cpu);
            rejects([&] { resources.mesh(handle); }, "Reused mesh slots reject stale generations");
            resources.destroy(next);
            const auto shader = resources.createShader(
                nullptr, "#version 330\nout vec4 finalColor;void main(){finalColor=vec4(1);}");
            resources.destroy(shader);
            rejects([&] { resources.shader(shader); }, "Shader release invalidates handle");
        }
        check(same(resources.counts(), baseline), "Diagnostic resource lifetimes balance");

        using Error = unsigned (*)();
        const auto error = reinterpret_cast<Error>(rlGetProcAddress("glGetError"));
        check(error && error() == 0, "No OpenGL errors");
        std::cout << "World hidden GPU checks passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}

#include "assets/image.hpp"
#include "platform/window.hpp"
#include "platform/world_renderer.hpp"
#include "support/check.hpp"
#include "support/world_fixtures.hpp"
#include <chrono>
#include <iostream>
using namespace blobber;
int main() {
    try {
        test::WorldFixture fixture;
        const auto map = test::worldRoom(363, 363);
        const auto tooLarge = test::worldRoom(364, 364);
        const auto begin = std::chrono::steady_clock::now();
        auto scene = world_render::prepare(map);
        check(scene.indices.size() / 6 == 262086, "Stress scene approaches quad cap");
        check(scene.vertices.size() > 65535, "Stress requires uint32 vertex indices");
        rejects([&] { world_render::prepare(tooLarge); }, "Above-cap valid map rejects without truncation");
        std::vector<std::string> sparseRows(1024, std::string(1024, '#'));
        for (int x = 1; x < 1023; ++x)
            sparseRows[1][x] = '.';
        const Map sparseMap(sparseRows);
        auto sparse = world_render::prepare(sparseMap);
        check(sparse.columns == 128 && sparse.rows == 128,
              "Maximum map dimensions supported within geometry budget");
        scene = {};
        sparse = {};
        Window window(640, 360, true);
        SetTargetFPS(0);
        ClearWindowState(FLAG_VSYNC_HINT);
        GpuResources resources;
        WorldRenderer renderer(resources, world_assets::load(fixture.directory));
        MovementState pose;
        pose.player = {{361, 361}, Heading::South};
        renderer.stage(map, pose);
        renderer.commit();
        pose.player.heading = Heading::East;
        renderer.stage(map, pose);
        const auto cpu = renderer.cpuBytes(), gpu = renderer.gpuBytes();
        check(cpu < 352u * 1024 * 1024 && gpu < 352u * 1024 * 1024, "Two large scenes fit tracked budgets");
        const auto initialDraws = renderer.stats.draws;
        renderer.commit();
        renderer.render(pose);
        const auto image = renderer.image();
        OwnedImage im(LoadImageFromTexture(resources.target(image.target).texture));
        check(im.value.data, "High-index world renders");
        ImageFlipVertical(&im.value);
        const auto pixel = GetImageColor(im.value, 250, 100);
        check(pixel.r > 170 && pixel.g > 80, "High uint32 ranges reach the GPU correctly");
        std::cout
            << "World stress passed: quads=262086 active_staged_cpu=" << cpu << " gpu_estimate=" << gpu
            << " draws=" << initialDraws << " elapsed_ms="
            << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count()
            << '\n';
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

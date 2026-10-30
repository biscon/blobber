#include "assets/image.hpp"
#include "platform/window.hpp"
#include "probe/gpu.hpp"
#include "probe/legacy.hpp"
#include "rlgl.h"
#include "support/check.hpp"
#include "support/probe_allocations.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
using namespace blobber;
using namespace blobber::probe;
namespace {
std::vector<Color> pixels(Texture2D texture) {
    OwnedImage im(LoadImageFromTexture(texture));
    check(im.value.data, "Readback succeeds");
    ImageFlipVertical(&im.value);
    ImageFormat(&im.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const auto* p = static_cast<Color*>(im.value.data);
    return {p, p + width * height};
}
bool same(const std::vector<Color>& a, const std::vector<Color>& b) {
    return std::equal(a.begin(), a.end(), b.begin(),
                      [](Color x, Color y) { return x.r == y.r && x.g == y.g && x.b == y.b && x.a == y.a; });
}
} // namespace
int main() {
    try {
        Window window(1920, 1080, true);
        SetTargetFPS(0);
        ClearWindowState(FLAG_VSYNC_HINT);
        auto scene = makeScene(false);
        auto art = loadArt(false, {}, scene);
        // Single-color surfaces provide an independent projection/fog oracle.
        for (unsigned i = 0; i < 3; ++i)
            std::fill(art.surfaces[i].colors.begin(), art.surfaces[i].colors.end(), Color{200, 100, 50, 255});
        auto invalid = makeScene(false);
        invalid.faces.resize(maxQuads + 1, invalid.faces.front());
        rejects([&] { Gpu rejected(invalid, art); },
                "Oversize mesh rejects after partial resource preparation");
        std::vector<Color> reference;
        for (int pass = 0; pass < 2; ++pass) {
            Gpu gpu(scene, art);
            MovementState state;
            state.player = {{2, 2}, Heading::West};
            auto image = pixels(gpu.render(scene, art, state, 0, 0, 90));
            const auto actual = image[100 * width + 250];
            const auto expected = shade({200 / 255.f, 100 / 255.f, 50 / 255.f}, .9f, 1);
            check(std::abs(int(actual.r) - expected[0] * 255) < 2 &&
                      std::abs(int(actual.g) - expected[1] * 255) < 2,
                  "GPU front wall matches CPU fog oracle");
            check(image[280 * width + 250].r < actual.r, "Floor receives its distinct face shading");
            if (pass)
                check(same(reference, image), "Resource recreation preserves output");
            else
                reference = image;
            gpu.render(scene, art, state, 0, 0, 90);
            check(gpu.stats.cached, "Unchanged view reused");
            auto super = pixels(gpu.render(scene, art, state, 0, 2, 90));
            check(std::abs(int(super[100 * width + 250].r) - actual.r) <= 1,
                  "Four-sample resolve retains uniform region and orientation");
            auto nearState = scripted(scene, 6 * shotMs);
            auto withBody = pixels(gpu.render(scene, art, nearState, 0, 0, 90));
            // Render an otherwise identical fixture with the actor hidden behind the enclosing wall.
            auto hidden = makeScene(false);
            hidden.actor = {0, 0};
            Gpu hiddenGpu(hidden, art);
            auto withoutBody = pixels(hiddenGpu.render(hidden, art, nearState, 0, 0, 90));
            unsigned changed = 0;
            for (std::size_t i = 0; i < withBody.size(); ++i)
                if (withBody[i].g != withoutBody[i].g)
                    ++changed;
            check(changed > 500, "Billboard is visible over room background");
            const auto cam = camera(nearState);
            const auto& im = art.npc.animations[0].views[0][0];
            // A known transparent hole at source (13,26) reveals the same world pixel.
            Vec hole{scene.actor.x + .25f + (13.5f / im.width - im.anchorX) * art.npc.width,
                     (im.anchorY - 26.5f / im.height) * art.npc.height, scene.actor.y + .25f};
            auto hp = project(cam, hole);
            int ix = int(hp.x), iy = int(hp.y);
            check(ix >= 0 && ix < width && iy >= 0 && iy < height, "Hole projects inside viewport");
            check(withBody[iy * width + ix].r == withoutBody[iy * width + ix].r &&
                      withBody[iy * width + ix].g == withoutBody[iy * width + ix].g,
                  "Transparent sprite hole preserves world depth/color");
            auto rear = makeScene(false);
            rear.actor = {1, 2};
            Gpu rearGpu(rear, art);
            check(same(reference, pixels(rearGpu.render(rear, art, state, 0, 0, 90))),
                  "Opaque wall fully hides actor behind it");
            for (bool back : {false, true}) {
                MovementState facing;
                facing.player = back ? Player{{5, 8}, Heading::South} : Player{{5, 10}, Heading::North};
                const auto view = camera(facing);
                const auto sample = project(
                    view, {scene.actor.x + .25f, art.npc.height * (1 - 10.5f / 48), scene.actor.y + .25f});
                auto facingPixels = pixels(gpu.render(scene, art, facing, 0, 0, 90));
                const auto body = facingPixels[int(sample.y) * width + int(sample.x)];
                const auto expectedColor =
                    shade(back ? std::array<float, 3>{91 / 255.f, 117 / 255.f, 47 / 255.f}
                               : std::array<float, 3>{116 / 255.f, 174 / 255.f, 62 / 255.f},
                          sample.depth, 1);
                check(std::abs(body.g - expectedColor[1] * 255) < 2,
                      "Directional billboard selects front/back source art");
            }
            // Warm all profiles and directions before guarding engine-owned frame calls.
            for (int cycle = 0; cycle < 2; ++cycle) {
                if (cycle == 1) {
                    allocations = 0;
                    watching = true;
                }
                for (int mode = 0; mode < 3; ++mode)
                    for (unsigned tick = 0; tick < 96; ++tick) {
                        const auto ms = (tick / 16) * shotMs + 200 + (tick % 16) * 10;
                        gpu.render(scene, art, scripted(scene, ms), ms, mode, 90);
                    }
                watching = false;
            }
            check(allocations == 0, "Warmed GPU frame orchestration allocates no tracked C++ storage");
            // Projection/blend/depth state must not leak into the following 2D pass.
            auto target = LoadRenderTexture(8, 8);
            check(IsRenderTextureValid(target), "State test target");
            BeginTextureMode(target);
            ClearBackground(BLANK);
            DrawRectangle(0, 0, 8, 8, RED);
            EndTextureMode();
            {
                OwnedImage im2(LoadImageFromTexture(target.texture));
                auto* p = static_cast<Color*>(im2.value.data);
                check(p[0].r == 230 && p[0].a == 255, "2D draw works after 3D pass");
            }
            UnloadRenderTexture(target);
        }
        auto specimen = makeScene(true);
        Gpu outside(specimen, art);
        auto state = scripted(specimen, 10 * shotMs);
        auto closed = pixels(outside.render(specimen, art, state, 0, 1, 0));
        auto opened = pixels(outside.render(specimen, art, state, 0, 1, 90));
        check(!same(closed, opened), "Door hinge changes depth-tested image");
        auto skyView = scripted(specimen, 11 * shotMs + 790);
        auto skyPixels = pixels(outside.render(specimen, art, skyView, 0, 1, 90));
        check(skyPixels[10 * width + 250].b > skyPixels[10 * width + 250].r, "Outdoor opening shows sky");
        using ErrorFn = unsigned (*)();
        auto error = reinterpret_cast<ErrorFn>(rlGetProcAddress("glGetError"));
        check(error && error() == 0, "No OpenGL integration error");
        std::cout << "Probe generated GPU depth, coverage, resolve and lifecycle checks pass\n";
        return 0;
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}

#include "assets/image.hpp"
#include "platform/gpu_resources.hpp"
#include "platform/window.hpp"
#include "support/check.hpp"
#include <iostream>
using namespace blobber;
int main() {
    try {
        const Window window(320, 240);
        GpuResources resources;
        rejects([&] { resources.createTarget(0, 32); }, "Invalid target size rejected");
        rejects([&] { resources.createTexture({}); }, "Invalid texture image rejected");
        OwnedImage image(GenImageColor(8, 8, RED));
        const auto texture = resources.createTexture(image.value);
        UpdateTexture(resources.texture(texture), image.value.data);
        check(resources.destroy(texture) && !resources.destroy(texture), "Texture destruction is once-only");
        const auto replacement = resources.createTexture(image.value);
        rejects([&] { resources.texture(texture); }, "Stale GPU texture rejected after reuse");
        const auto target = resources.createTarget(32, 32);
        BeginTextureMode(resources.target(target));
        ClearBackground(BLACK);
        DrawTexture(resources.texture(replacement), 0, 0, WHITE);
        EndTextureMode();
        {
            OwnedImage readback(LoadImageFromTexture(resources.target(target).texture));
            check(readback.value.data != nullptr, "GPU readback succeeds");
        }
        check(resources.destroy(target), "Target released");
        resources.createTarget(32, 32);
        rejects([&] { resources.target(target); }, "Stale GPU target rejected after reuse");
        for (int i = 0; i < 3; ++i)
            resources.createTarget(32, 32);
        rejects([&] { resources.createTarget(32, 32); }, "Target capacity fails before allocation");
        for (int i = 0; i < 7; ++i)
            resources.createTexture(image.value);
        rejects([&] { resources.createTexture(image.value); }, "Texture capacity fails before allocation");
        BeginDrawing();
        ClearBackground(BLACK);
        EndDrawing();
        std::cout << "GPU lifecycle checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

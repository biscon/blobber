#pragma once
#include "platform/presenter.hpp"
#include "support/check.hpp"
#include "ui/hud.hpp"
#include "ui/world_view_draw.hpp"
namespace blobber::test {
// Generated/owned world publication supplied by the integration fixture. No game art dependency.
inline void screenShakeGpu(GpuResources& resources, WorldRenderer& renderer,
                           const world_render::FrameInput& input) {
    GpuLease<TargetHandle> target(resources, resources.createTarget(gameWidth, gameHeight));
    const auto worldImage = renderer.image();
    const auto pickingRevision = renderer.picking()->targets().revision;
    const auto transfers = resources.transfers();
    HudSnapshot hud;
    hud.cards[0].id = {0};
    hud.feedback[0].active = hud.feedback[0].hit = true;
    hud.feedback[0].anchor = {250, 150, true};
    hud.feedback[1].active = hud.feedback[1].hit = hud.feedback[1].party = true;
    hud.feedback[1].character = {0};
    auto draw = [&](WorldViewTransform transform) {
        hud.worldTransform = transform;
        BeginTextureMode(resources.target(target.get()));
        ClearBackground(MAGENTA);
        drawWorldViewport(resources.target(worldImage.target).texture, true, transform);
        drawHudOverlay(&hud, renderer.picking());
        {
            const WorldOverlayScope overlay(transform);
            overlay.clip({100, 70, 10, 10});
            // Deliberately oversized drawing must respect the transformed depth clip.
            DrawRectangle(0, 0, gameWidth, gameHeight, LIME);
            overlay.viewportClip();
            // Geometry extending beyond the left viewport must never escape the fixed scissor.
            DrawRectangle(-30, 300, 60, 60, SKYBLUE);
        }
        EndTextureMode();
    };
    auto read = [&] {
        auto result =
            std::make_unique<OwnedImage>(LoadImageFromTexture(resources.target(target.get()).texture));
        check(result->value.data, "Shake native target readback");
        ImageFlipVertical(&result->value);
        ImageFormat(&result->value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        return result;
    };
    draw({});
    auto idle = read();
    const WorldViewTransform shifted{5, -4, 1.05f};
    allocations = 0;
    watching = true;
    draw(shifted);
    const auto cached = renderer.render(input);
    watching = false;
    check(allocations == 0, "World composition and attached overlay transforms allocate no engine memory");
    auto shaken = read();
    const auto* a = static_cast<const Color*>(idle->value.data);
    const auto* b = static_cast<const Color*>(shaken->value.data);
    auto equal = [](Color x, Color y) { return x.r == y.r && x.g == y.g && x.b == y.b && x.a == y.a; };
    unsigned changed = 0, stationaryFeedback = 0;
    for (int y = 0; y < gameHeight; ++y)
        for (int x = 0; x < gameWidth; ++x) {
            const unsigned pixel = y * gameWidth + x;
            if (x >= viewX * 3 && x < (viewX + viewWidth) * 3 && y >= viewY * 3 &&
                y < (viewY + viewHeight) * 3) {
                changed += !equal(a[pixel], b[pixel]);
                check(!equal(b[pixel], MAGENTA), "Shake exposes no cleared pixels inside viewport");
            } else {
                check(equal(a[pixel], b[pixel]), "Pixels outside the world viewport remain stationary");
                stationaryFeedback += equal(a[pixel], Color{190, 24, 18, 255});
            }
        }
    check(changed > 100 && stationaryFeedback > 100, "Scene moves while party feedback stays fixed");
    const auto marker = shifted.forward({105, 75});
    check(equal(b[int(marker.y * 3) * gameWidth + int(marker.x * 3)], LIME),
          "Depth-clipped world decoration follows the image transform");
    const auto outside = shifted.forward({99, 75});
    check(!equal(b[int(outside.y * 3) * gameWidth + int(outside.x * 3)], LIME),
          "Transformed depth clip still excludes obscured geometry");
    const auto feedback = shifted.forward({254, 168});
    check(equal(b[int(feedback.y * 3) * gameWidth + int(feedback.x * 3)], Color{190, 24, 18, 255}),
          "Enemy damage splash follows world transform");
    check(cached.revision == worldImage.revision &&
              renderer.picking()->targets().revision == pickingRevision &&
              resources.transfers().uploads == transfers.uploads,
          "Shake alone reuses world publication and picking without uploads");
    draw({});
    auto restored = read();
    const auto* c = static_cast<const Color*>(restored->value.data);
    for (int pixel = 0; pixel < gameWidth * gameHeight; ++pixel)
        check(equal(a[pixel], c[pixel]), "Expiry restores the exact original composition");
    using Error = unsigned (*)();
    check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0, "Shake GL state valid");
}
} // namespace blobber::test

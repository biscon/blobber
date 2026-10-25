#include "platform/presenter.hpp"
#include "assets/image.hpp"
#include "ui/hud.hpp"
#include <stdexcept>
namespace blobber {
Presenter::Presenter() {
    art_ = resources_.createTarget(artWidth, artHeight);
    game_ = resources_.createTarget(gameWidth, gameHeight);
    OwnedImage image(GenImageColor(viewWidth, viewHeight, BLACK));
    viewport_ = resources_.createTexture(image.value);
}
void Presenter::drawTarget(TargetHandle handle, Rectangle destination) const {
    const auto& value = resources_.target(handle);
    DrawTexturePro(value.texture, {0, 0, float(value.texture.width), -float(value.texture.height)},
                   destination, {0, 0}, 0, WHITE);
}
void Presenter::present(const Frame& frame, const MovementState& state, const Map& map, bool debug,
                        bool changed, const HudSnapshot* hud, const PickingBuffer* picking) {
    if (changed)
        UpdateTexture(resources_.texture(viewport_), frame.data());
    BeginTextureMode(resources_.target(art_));
    drawUI(state, debug, map, hud);
    DrawTexture(resources_.texture(viewport_), viewX, viewY, WHITE);
    drawHudOverlay(hud, picking);
    EndTextureMode();
    BeginTextureMode(resources_.target(game_));
    ClearBackground(BLACK);
    drawTarget(art_, {0, 0, float(gameWidth), float(gameHeight)});
    EndTextureMode();
    BeginDrawing();
    ClearBackground(BLACK);
    // Fit physical framebuffer pixels, then convert to raylib's logical coordinates.
    const int rw = GetRenderWidth(), rh = GetRenderHeight();
    const FitRect fit = aspectFit(rw, rh);
    if (rw > 0 && rh > 0 && !IsWindowMinimized()) {
        const float sx = float(GetScreenWidth()) / rw, sy = float(GetScreenHeight()) / rh;
        drawTarget(game_, {fit.x * sx, fit.y * sy, fit.width * sx, fit.height * sy});
    }
    EndDrawing();
}
void Presenter::capture(const std::filesystem::path& path) const {
    OwnedImage image(LoadImageFromTexture(resources_.target(game_).texture));
    if (!image.value.data)
        throw std::runtime_error("Cannot read capture target");
    ImageFlipVertical(&image.value);
    if (!ExportImage(image.value, path.string().c_str()))
        throw std::runtime_error("Cannot export capture: " + path.string());
}
} // namespace blobber

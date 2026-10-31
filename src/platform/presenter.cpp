#include "platform/presenter.hpp"
#include "assets/image.hpp"
#include "assets/prepared_image.hpp"
#include "rlgl.h"
#include "ui/hud.hpp"
#include <cmath>
#include <cstdio>
#include <stdexcept>
namespace blobber {
Presenter::Presenter(GpuResources& resources, const std::filesystem::path& assets, PreparationWorker* worker)
    : worker_(worker), resources_(resources), assetsMenuPath_(assets / "ui/menu-background.png"),
      fonts_(assets / "fonts"), uiArt_(resources_, assets / "ui") {
    game_ = resources_.createTarget(gameWidth, gameHeight);
    ui_ = resources_.createTarget(gameWidth, gameHeight);
    SetTextureFilter(resources_.target(ui_).texture, TEXTURE_FILTER_BILINEAR);
}
void Presenter::drawTarget(TargetHandle handle, Rectangle destination) const {
    const auto& value = resources_.target(handle);
    DrawTexturePro(value.texture, {0, 0, float(value.texture.width), -float(value.texture.height)},
                   destination, {0, 0}, 0, WHITE);
}
void Presenter::drawGame(Rectangle destination) const {
    drawTarget(game_, destination);
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
    drawTarget(ui_, destination);
    EndBlendMode();
    if (fade_ > 0)
        DrawRectangleRec(destination, {0, 0, 0, static_cast<unsigned char>(std::round(fade_ * 255))});
    if (loadingLabel_[0]) {
        rlPushMatrix();
        rlTranslatef(destination.x, destination.y, 0);
        rlScalef(destination.width / gameWidth, destination.height / gameHeight, 1);
        fonts_.draw(loadingLabel_.data(),
                    (artWidth - fonts_.width(loadingLabel_.data(), UiTextStyle::Label)) * .5f,
                    artHeight * .5f, {202, 178, 118, 255}, UiTextStyle::Label);
        rlPopMatrix();
    }
}
void Presenter::presentCached(float fade, const char* status) {
    fade_ = std::clamp(fade, 0.f, 1.f);
    std::snprintf(loadingLabel_.data(), loadingLabel_.size(), "%s", status ? status : "");
    BeginDrawing();
    ClearBackground(BLACK);
    const int rw = GetRenderWidth(), rh = GetRenderHeight();
    if (rw > 0 && rh > 0 && !IsWindowMinimized()) {
        const auto fit = aspectFit(rw, rh);
        drawGame({fit.x * float(GetScreenWidth()) / rw, fit.y * float(GetScreenHeight()) / rh,
                  fit.width * float(GetScreenWidth()) / rw, fit.height * float(GetScreenHeight()) / rh});
    }
    EndDrawing();
}
void Presenter::presentWorld(TargetHandle logical, std::uint64_t revision, const MovementState& state,
                             const Map& map, const HudSnapshot* hud, const PickingBuffer* picking, bool debug,
                             const generation::DiagnosticView* diagnostic, bool reveal) {
    loadingLabel_[0] = 0;
    const auto& target = resources_.target(logical);
    if (!revision || target.texture.width != viewWidth || target.texture.height != viewHeight)
        throw std::invalid_argument("Invalid world image publication");
    compose(target.texture, true, state, map, debug, hud, picking, diagnostic, reveal);
}
void Presenter::compose(const Texture2D& texture, bool flipped, const MovementState& state, const Map& map,
                        bool debug, const HudSnapshot* hud, const PickingBuffer* picking,
                        const generation::DiagnosticView* generation, bool reveal) {
    SetTextureFilter(resources_.target(game_).texture, TEXTURE_FILTER_POINT);
    BeginTextureMode(resources_.target(game_));
    ClearBackground({12, 18, 18, 255});
    constexpr float scale = float(gameWidth) / artWidth;
    DrawTexturePro(texture, {0, 0, float(viewWidth), float(viewHeight) * (flipped ? -1 : 1)},
                   {viewX * scale, viewY * scale, viewWidth * scale, viewHeight * scale}, {0, 0}, 0, WHITE);
    EndTextureMode();
    BeginTextureMode(resources_.target(ui_));
    ClearBackground(BLANK);
    // Store premultiplied RGB with coverage alpha, not alpha squared.
    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA,
                              RL_FUNC_ADD, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);
    drawUI(state, debug, map, hud, &fonts_, &uiArt_);
    if (debug)
        drawMapOverlay(state, map, &fonts_, generation, hud, reveal);
    drawHudOverlay(hud, picking, &fonts_, &uiArt_);
    EndBlendMode();
    EndTextureMode();
    BeginDrawing();
    ClearBackground(BLACK);
    // Fit physical framebuffer pixels, then convert to raylib's logical coordinates.
    const int rw = GetRenderWidth(), rh = GetRenderHeight();
    const FitRect fit = aspectFit(rw, rh);
    if (rw > 0 && rh > 0 && !IsWindowMinimized()) {
        const float sx = float(GetScreenWidth()) / rw, sy = float(GetScreenHeight()) / rh;
        drawGame({fit.x * sx, fit.y * sy, fit.width * sx, fit.height * sy});
    }
    EndDrawing();
}
void Presenter::prepareMenu() {
    if (!menuBackground_) {
        OwnedImage background(preparedImage(assetsMenuPath_));
        if (!background.value.data)
            throw std::runtime_error("Missing menu background");
        menuBackground_ = resources_.createTexture(background.value);
        SetTextureFilter(resources_.texture(menuBackground_), TEXTURE_FILTER_BILINEAR);
    }
}
void Presenter::presentMenu(void (*draw)(void*, const UiFonts&, const UiArt&), void* context) {
    loadingLabel_[0] = 0;
    fade_ = 0;
    SetTextureFilter(resources_.target(game_).texture, TEXTURE_FILTER_BILINEAR);
    BeginTextureMode(resources_.target(game_));
    ClearBackground({12, 18, 18, 255});
    const auto& background = resources_.texture(menuBackground_);
    DrawTexturePro(background, {0, 0, float(background.width), float(background.height)},
                   {0, 0, float(gameWidth), float(gameHeight)}, {0, 0}, 0, WHITE);
    draw(context, fonts_, uiArt_);
    EndTextureMode();
    BeginTextureMode(resources_.target(ui_));
    ClearBackground(BLANK);
    EndTextureMode();
    BeginDrawing();
    ClearBackground(BLACK);
    const int rw = GetRenderWidth(), rh = GetRenderHeight();
    if (rw > 0 && rh > 0) {
        const auto fit = aspectFit(rw, rh);
        drawGame({fit.x * GetScreenWidth() / rw, fit.y * GetScreenHeight() / rh,
                  fit.width * GetScreenWidth() / rw, fit.height * GetScreenHeight() / rh});
    }
    EndDrawing();
}
void Presenter::capture(const std::filesystem::path& path, int width, int height) const {
    if (width <= 0 || height <= 0)
        throw std::invalid_argument("Invalid capture size");
    // Diagnostic-only allocation: the normal frame loop reuses its two targets.
    GpuResources captureResources;
    const auto composite = captureResources.createTarget(width, height);
    BeginTextureMode(captureResources.target(composite));
    ClearBackground(BLACK);
    const auto fit = aspectFit(width, height);
    drawGame({fit.x, fit.y, fit.width, fit.height});
    EndTextureMode();
    OwnedImage image(LoadImageFromTexture(captureResources.target(composite).texture));
    if (!image.value.data)
        throw std::runtime_error("Cannot read capture target");
    ImageFlipVertical(&image.value);
    if (!ExportImage(image.value, path.string().c_str()))
        throw std::runtime_error("Cannot export capture: " + path.string());
}
} // namespace blobber

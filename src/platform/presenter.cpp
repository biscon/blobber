#include "platform/presenter.hpp"
#include "assets/image.hpp"
#include "assets/prepared_image.hpp"
#include "platform/audio.hpp"
#include "rlgl.h"
#include "ui/hud.hpp"
#include "ui/menu_background.hpp"
#include "ui/music_banner.hpp"
#include <cmath>
#include <cstdio>
#include <stdexcept>
namespace blobber {
void drawWorldViewport(const Texture2D& texture, bool flipped, WorldViewTransform transform) {
    constexpr float scale = float(gameWidth) / artWidth;
    const auto origin = transform.forward({float(viewX), float(viewY)});
    BeginScissorMode(int(viewX * scale), int(viewY * scale), int(viewWidth * scale), int(viewHeight * scale));
    DrawTexturePro(texture, {0, 0, float(viewWidth), float(viewHeight) * (flipped ? -1 : 1)},
                   {origin.x * scale, origin.y * scale, viewWidth * scale * transform.zoom,
                    viewHeight * scale * transform.zoom},
                   {0, 0}, 0, WHITE);
    EndScissorMode();
}
Presenter::Presenter(GpuResources& resources, const std::filesystem::path& assets, PreparationWorker* worker,
                     const std::filesystem::path& preparedAssets)
    : worker_(worker), resources_(resources), assetsMenuPath_(assets / "ui/menu-background.png"),
      fonts_(assets / "fonts"), fontMemory_(resources_, fonts_.gpuBytes()),
      uiArt_(resources_, assets / "ui", preparedAssets) {
    game_ = resources_.createTarget(gameWidth, gameHeight);
    ui_ = resources_.createTarget(gameWidth, gameHeight);
    SetTextureFilter(resources_.target(ui_).texture, TEXTURE_FILTER_BILINEAR);
}
void Presenter::finishFrame() {
    if (IsWindowFocused() && IsKeyPressed(KEY_F1))
        showFps_ = !showFps_;
    if (showFps_) {
        // Window-space diagnostics stay above pause/loading overlays and outside cached images.
        DrawRectangle(8, 8, 112, 28, {0, 0, 0, 220});
        DrawFPS(12, 12);
    }
    EndDrawing();
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
    if (saving_) {
        rlPushMatrix();
        rlTranslatef(destination.x, destination.y, 0);
        rlScalef(destination.width / gameWidth, destination.height / gameHeight, 1);
        DrawRectangle(0, 0, gameWidth, gameHeight, {0, 0, 0, 160});
        constexpr Rectangle dialog{gameWidth * .5f - 210, gameHeight * .5f - 65, 420, 130};
        DrawRectangleRec(dialog, {15, 23, 23, 255});
        DrawRectangleLinesEx(dialog, 2, {202, 178, 118, 255});
        // Font coordinates are in the native HUD's 640x360 space.
        constexpr const char* label = "Saving...";
        fonts_.draw(label, (artWidth - fonts_.width(label, UiTextStyle::Heading)) * .5f,
                    fonts_.centeredY(label, artHeight * .5f - 20, 40, UiTextStyle::Heading),
                    {202, 178, 118, 255}, UiTextStyle::Heading);
        rlPopMatrix();
    }
}
void Presenter::presentCached(float fade, const char* status) {
    saving_ = false;
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
    finishFrame();
}
void Presenter::presentSaving() {
    saving_ = true;
    fade_ = 0;
    loadingLabel_[0] = 0;
    BeginDrawing();
    ClearBackground(BLACK);
    const int rw = GetRenderWidth(), rh = GetRenderHeight();
    if (rw > 0 && rh > 0 && !IsWindowMinimized()) {
        const auto fit = aspectFit(rw, rh);
        const Rectangle destination{
            fit.x * float(GetScreenWidth()) / rw, fit.y * float(GetScreenHeight()) / rh,
            fit.width * float(GetScreenWidth()) / rw, fit.height * float(GetScreenHeight()) / rh};
        drawGame(destination);
    }
    finishFrame();
}
void Presenter::presentWorld(TargetHandle logical, std::uint64_t revision, const MovementState& state,
                             const Map& map, const HudSnapshot* hud, const PickingBuffer* picking, bool debug,
                             const generation::DiagnosticView* diagnostic, bool reveal,
                             const MapOverlayModel* mapModel) {
    saving_ = false;
    loadingLabel_[0] = 0;
    const auto& target = resources_.target(logical);
    if (!revision || target.texture.width != viewWidth || target.texture.height != viewHeight)
        throw std::invalid_argument("Invalid world image publication");
    compose(target.texture, true, state, map, debug, hud, picking, diagnostic, reveal, mapModel);
}
void Presenter::compose(const Texture2D& texture, bool flipped, const MovementState& state, const Map& map,
                        bool debug, const HudSnapshot* hud, const PickingBuffer* picking,
                        const generation::DiagnosticView* generation, bool reveal,
                        const MapOverlayModel* mapModel) {
    SetTextureFilter(resources_.target(game_).texture, TEXTURE_FILTER_POINT);
    BeginTextureMode(resources_.target(game_));
    ClearBackground({12, 18, 18, 255});
    displayedWorld_ = hud ? hud->worldTransform : WorldViewTransform{};
    drawWorldViewport(texture, flipped, displayedWorld_);
    EndTextureMode();
    BeginTextureMode(resources_.target(ui_));
    ClearBackground(BLANK);
    // Store premultiplied RGB with coverage alpha, not alpha squared.
    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA,
                              RL_FUNC_ADD, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);
    drawUI(state, debug, map, hud, &fonts_, &uiArt_);
    if (debug)
        drawMapOverlay(state, map, &fonts_, generation, hud, reveal, mapModel, &uiArt_);
    if (audio && hud && !hud->modal() && !debug)
        drawMusicBanner(audio->musicBanner(), fonts_);
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
    finishFrame();
}
void Presenter::prepareMenu() {
    if (!menuBackground_) {
        OwnedImage background(preparedImage(assetsMenuPath_));
        if (!background.value.data)
            throw std::runtime_error("Missing menu background");
        menuBackground_ = resources_.createTexture(background.value);
        SetTextureFilter(resources_.texture(menuBackground_), TEXTURE_FILTER_BILINEAR);
        menuStarted_ = GetTime();
    }
    uiArt_.prepareMenuTitle(assetsMenuPath_.parent_path() / "menu-title.png");
}
void Presenter::presentMenu(void (*draw)(void*, const UiFonts&, const UiArt&), void* context,
                            std::optional<double> backgroundSeconds) {
    saving_ = false;
    loadingLabel_[0] = 0;
    displayedWorld_ = {};
    fade_ = 0;
    SetTextureFilter(resources_.target(game_).texture, TEXTURE_FILTER_BILINEAR);
    BeginTextureMode(resources_.target(game_));
    ClearBackground({12, 18, 18, 255});
    const auto& background = resources_.texture(menuBackground_);
    const auto crop = menuBackgroundCrop(backgroundSeconds.value_or(GetTime() - menuStarted_));
    DrawTexturePro(background,
                   {crop.x * background.width, crop.y * background.height, crop.width * background.width,
                    crop.height * background.height},
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
    finishFrame();
}
void Presenter::capture(const std::filesystem::path& path, int width, int height) const {
    if (width <= 0 || height <= 0)
        throw std::invalid_argument("Invalid capture size");
    // Diagnostic-only allocation: the normal frame loop reuses its two targets.
    GpuResources captureResources;
    const auto composite = captureResources.createTarget(width, height);
    GpuExternalLease captureMemory(resources_, captureResources.counts().bytes);
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

#pragma once
#include "core/preparation.hpp"
#include "game/commands.hpp"
#include "game/movement.hpp"
#include "generation/diagnostic_view.hpp"
#include "platform/gpu_resources.hpp"
#include "render/picking.hpp"
#include "ui/art.hpp"
#include "ui/fonts.hpp"
#include <algorithm>
#include <filesystem>
namespace blobber {
class AudioRuntime;
class MapOverlayModel;
// Composite a prepared image into the fixed native viewport; no target allocation.
void drawWorldViewport(const Texture2D&, bool flipped, WorldViewTransform);
class Presenter {
  public:
    AudioRuntime* audio = nullptr; // Application-owned; outlives presentation loops.
    Presenter(GpuResources& resources, const std::filesystem::path& assets,
              PreparationWorker* worker = nullptr);
    PreparationWorker* loadingWorker() const noexcept { return worker_; }
    void presentCached(float fade, const char* status = nullptr);
    void presentSaving(); // Cached composite only; no live simulation/UI readers.
    void fade(float alpha) noexcept { fade_ = std::clamp(alpha, 0.f, 1.f); }
    const UiFonts& fonts() const noexcept { return fonts_; }
    GpuResources& resources() noexcept { return resources_; }
    void presentWorld(TargetHandle logical, std::uint64_t revision, const MovementState&, const Map&,
                      const HudSnapshot* hud = nullptr, const PickingBuffer* picking = nullptr,
                      bool debug = false, const generation::DiagnosticView* diagnostic = nullptr,
                      bool reveal = false, const MapOverlayModel* mapModel = nullptr);
    const WorldViewTransform& displayedWorldTransform() const noexcept { return displayedWorld_; }
    void prepareDialogue(const std::vector<std::string>& files) { uiArt_.prepareDialogue(files); }
    void clearDialogue() noexcept { uiArt_.clearDialogue(); }
    void prepareMenu(); // Explicit allocation-allowed UI preparation, before menu frames.
    void presentMenu(void (*draw)(void*, const UiFonts&, const UiArt&), void* context);
    // Diagnostic operation: readback and export may allocate; never used during normal play.
    void capture(const std::filesystem::path& path, int width = gameWidth, int height = gameHeight) const;

  private:
    void finishFrame();
    void drawTarget(TargetHandle handle, Rectangle destination) const;
    void drawGame(Rectangle destination) const;
    void compose(const Texture2D&, bool flipped, const MovementState&, const Map&, bool debug,
                 const HudSnapshot*, const PickingBuffer*, const generation::DiagnosticView*, bool reveal,
                 const MapOverlayModel*);
    WorldViewTransform displayedWorld_;
    PreparationWorker* worker_ = nullptr;
    bool showFps_ = false, saving_ = false;
    float fade_ = 0;
    std::array<char, 96> loadingLabel_{};
    GpuResources& resources_;
    TargetHandle game_, ui_;
    TextureHandle menuBackground_;
    std::filesystem::path assetsMenuPath_;
    UiFonts fonts_;
    GpuExternalLease fontMemory_;
    UiArt uiArt_;
};
} // namespace blobber

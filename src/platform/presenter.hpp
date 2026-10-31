#pragma once
#include "game/commands.hpp"
#include "game/movement.hpp"
#include "generation/diagnostic_view.hpp"
#include "platform/gpu_resources.hpp"
#include "render/picking.hpp"
#include "ui/art.hpp"
#include "ui/fonts.hpp"
#include <filesystem>
namespace blobber {
class Presenter {
  public:
    Presenter(GpuResources& resources, const std::filesystem::path& assets);
    GpuResources& resources() noexcept { return resources_; }
    void presentWorld(TargetHandle logical, std::uint64_t revision, const MovementState&, const Map&,
                      const HudSnapshot* hud = nullptr, const PickingBuffer* picking = nullptr,
                      bool debug = false, const generation::DiagnosticView* diagnostic = nullptr,
                      bool reveal = false);
    void prepareMenu(); // Explicit allocation-allowed UI preparation, before menu frames.
    void presentMenu(void (*draw)(void*, const UiFonts&, const UiArt&), void* context);
    // Diagnostic operation: readback and export may allocate; never used during normal play.
    void capture(const std::filesystem::path& path, int width = gameWidth, int height = gameHeight) const;

  private:
    void drawTarget(TargetHandle handle, Rectangle destination) const;
    void drawGame(Rectangle destination) const;
    void compose(const Texture2D&, bool flipped, const MovementState&, const Map&, bool debug,
                 const HudSnapshot*, const PickingBuffer*, const generation::DiagnosticView*, bool reveal);
    GpuResources& resources_;
    TargetHandle game_, ui_;
    TextureHandle menuBackground_;
    std::filesystem::path assetsMenuPath_;
    UiFonts fonts_;
    UiArt uiArt_;
};
} // namespace blobber

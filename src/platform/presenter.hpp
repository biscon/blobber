#pragma once
#include "game/commands.hpp"
#include "game/movement.hpp"
#include "platform/gpu_resources.hpp"
#include "render/frame.hpp"
#include "render/picking.hpp"
#include "ui/art.hpp"
#include "ui/fonts.hpp"
#include <filesystem>
namespace blobber {
class Presenter {
  public:
    explicit Presenter(const std::filesystem::path& assets);
    void present(const Frame& frame, const MovementState& state, const Map& map, bool debug, bool changed,
                 const HudSnapshot* hud = nullptr, const PickingBuffer* picking = nullptr);
    // Diagnostic operation: readback and export may allocate; never used during normal play.
    void capture(const std::filesystem::path& path, int width = gameWidth, int height = gameHeight) const;

  private:
    void drawTarget(TargetHandle handle, Rectangle destination) const;
    void drawGame(Rectangle destination) const;
    GpuResources resources_;
    TargetHandle game_, ui_;
    TextureHandle viewport_;
    UiFonts fonts_;
    UiArt uiArt_;
};
} // namespace blobber

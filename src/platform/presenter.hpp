#pragma once
#include "game/movement.hpp"
#include "platform/gpu_resources.hpp"
#include "render/frame.hpp"
#include <filesystem>
namespace blobber {
class Presenter {
  public:
    Presenter();
    void present(const Frame& frame, const MovementState& state, const Map& map, bool debug, bool changed);
    // Diagnostic operation: readback and export may allocate; never used during normal play.
    void capture(const std::filesystem::path& path) const;

  private:
    void drawTarget(TargetHandle handle, Rectangle destination) const;
    GpuResources resources_;
    TargetHandle art_, game_;
    TextureHandle viewport_;
};
} // namespace blobber

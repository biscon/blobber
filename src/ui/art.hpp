#pragma once
#include "platform/gpu_resources.hpp"
#include "ui/layout.hpp"
#include <array>
#include <filesystem>
namespace blobber {
// One startup-prepared atlas, owned by Presenter's resource registry.
class UiArt {
  public:
    UiArt(GpuResources& resources, const std::filesystem::path& directory);
    void draw(unsigned index, UiRect destination, Color tint = WHITE) const;

  private:
    GpuResources& resources_;
    TextureHandle atlas_;
    std::array<Rectangle, 11> regions_{};
};
} // namespace blobber

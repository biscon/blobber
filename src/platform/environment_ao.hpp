#pragma once
#include "platform/gpu_resources.hpp"

namespace blobber {
// Shared scratch, never a published scene. All storage is acquired at preparation.
class EnvironmentAo {
  public:
    static constexpr int width = 1000, height = 564;
    static constexpr std::size_t bytes = std::size_t(width) * height * 8 * 3;
    explicit EnvironmentAo(GpuResources&);
    TargetHandle depth() const noexcept { return depth_.get(); }
    TargetHandle image() const noexcept { return output_.get(); }
    void render(Matrix projection);
    void bindSurface(Shader, bool enabled, int width, int height);

  private:
    GpuResources& resources_;
    GpuLease<TargetHandle> depth_, output_, scratch_;
    GpuLease<ShaderHandle> evaluate_, blur_;
    void pass(TargetHandle destination, Shader, TargetHandle source, Matrix inverseProjection,
              Vector2 direction);
};
} // namespace blobber

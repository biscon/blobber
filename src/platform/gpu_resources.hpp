#pragma once
#include "core/resource_pool.hpp"
#include "raylib.h"
namespace blobber {
struct TextureTag;
struct TargetTag;
using TextureHandle = Handle<TextureTag>;
using TargetHandle = Handle<TargetTag>;
class GpuResources {
  public:
    ~GpuResources();
    TextureHandle createTexture(const Image& image);
    TargetHandle createTarget(int width, int height);
    bool destroy(TextureHandle handle) noexcept;
    bool destroy(TargetHandle handle) noexcept;
    const Texture2D& texture(TextureHandle handle) const;
    const RenderTexture2D& target(TargetHandle handle) const;

  private:
    ResourcePool<Texture2D, TextureTag, 8> textures_;
    ResourcePool<RenderTexture2D, TargetTag, 4> targets_;
};
} // namespace blobber

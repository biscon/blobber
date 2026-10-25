#include "platform/gpu_resources.hpp"
#include <stdexcept>
namespace blobber {
GpuResources::~GpuResources() {
    targets_.clear([](RenderTexture2D value) { UnloadRenderTexture(value); });
    textures_.clear([](Texture2D value) { UnloadTexture(value); });
}
TextureHandle GpuResources::createTexture(const Image& image) {
    if (!textures_.available())
        throw std::runtime_error("Texture capacity exhausted (8)");
    if (!image.data || image.width <= 0 || image.height <= 0)
        throw std::invalid_argument("Invalid texture image");
    auto value = LoadTextureFromImage(image);
    if (!IsTextureValid(value)) {
        if (value.id)
            UnloadTexture(value);
        throw std::runtime_error("Cannot create texture");
    }
    SetTextureFilter(value, TEXTURE_FILTER_POINT);
    return textures_.insert(value);
}
TargetHandle GpuResources::createTarget(int width, int height) {
    if (!targets_.available())
        throw std::runtime_error("Render target capacity exhausted (4)");
    if (width <= 0 || height <= 0)
        throw std::invalid_argument("Invalid render target dimensions");
    auto value = LoadRenderTexture(width, height);
    if (!IsRenderTextureValid(value)) {
        UnloadRenderTexture(value);
        throw std::runtime_error("Cannot create render target");
    }
    SetTextureFilter(value.texture, TEXTURE_FILTER_POINT);
    return targets_.insert(value);
}
bool GpuResources::destroy(TextureHandle handle) noexcept {
    if (auto value = textures_.release(handle)) {
        UnloadTexture(*value);
        return true;
    }
    return false;
}
bool GpuResources::destroy(TargetHandle handle) noexcept {
    if (auto value = targets_.release(handle)) {
        UnloadRenderTexture(*value);
        return true;
    }
    return false;
}
const Texture2D& GpuResources::texture(TextureHandle handle) const {
    if (auto value = textures_.get(handle))
        return *value;
    throw std::invalid_argument("Invalid or stale texture handle");
}
const RenderTexture2D& GpuResources::target(TargetHandle handle) const {
    if (auto value = targets_.get(handle))
        return *value;
    throw std::invalid_argument("Invalid or stale render target handle");
}
} // namespace blobber

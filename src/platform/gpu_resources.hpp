#pragma once
#include "core/resource_pool.hpp"
#include "raylib.h"
#include "render/world_scene.hpp"
#include <utility>
namespace blobber {
struct TextureTag;
struct TargetTag;
struct MeshTag;
struct ShaderTag;
using TextureHandle = Handle<TextureTag>;
using TargetHandle = Handle<TargetTag>;
using MeshHandle = Handle<MeshTag>;
using ShaderHandle = Handle<ShaderTag>;
struct GpuMesh {
    unsigned vao = 0, vertices = 0, indices = 0;
    std::uint32_t count = 0;
    std::size_t bytes = 0;
};
struct GpuCounts {
    unsigned textures = 0, targets = 0, meshes = 0, shaders = 0;
    std::size_t bytes = 0;
};
struct GpuTransfers {
    std::uint64_t uploads = 0, bytes = 0;
};
class GpuResources {
  public:
    ~GpuResources();
    TextureHandle createTexture(const Image& image);
    TargetHandle createTarget(int width, int height);
    MeshHandle createMesh(const world_render::Scene&);
    ShaderHandle createShader(const char* vertex, const char* fragment);
    bool destroy(TextureHandle) noexcept;
    bool destroy(TargetHandle) noexcept;
    bool destroy(MeshHandle) noexcept;
    bool destroy(ShaderHandle) noexcept;
    const Texture2D& texture(TextureHandle) const;
    const RenderTexture2D& target(TargetHandle) const;
    const GpuMesh& mesh(MeshHandle) const;
    const Shader& shader(ShaderHandle) const;
    void draw(MeshHandle, std::uint32_t first, std::uint32_t count) const;
    void finishPreparation(); // Validate the completed staged draw before publication.
    GpuTransfers transfers() const noexcept { return transfers_; }
    GpuCounts counts() const noexcept { return counts_; }
    // Diagnostic preparation injection only. Negative disables; zero fails next acquisition.
    void failAfter(int acquisitions) noexcept { failAfter_ = acquisitions; }

  private:
    void checkpoint();
    ResourcePool<Texture2D, TextureTag, 512> textures_;
    ResourcePool<RenderTexture2D, TargetTag, 8> targets_;
    ResourcePool<GpuMesh, MeshTag, 8> meshes_;
    ResourcePool<Shader, ShaderTag, 8> shaders_;
    GpuCounts counts_{};
    GpuTransfers transfers_{};
    int failAfter_ = -1;
};
// A bounded registry owns descriptors; leases release the designated resource early.
template <class H> class GpuLease {
  public:
    GpuLease() = default;
    GpuLease(GpuResources& r, H h) : resources_(&r), handle_(h) {}
    ~GpuLease() { reset(); }
    GpuLease(const GpuLease&) = delete;
    GpuLease& operator=(const GpuLease&) = delete;
    GpuLease(GpuLease&& other) noexcept
        : resources_(other.resources_), handle_(std::exchange(other.handle_, {})) {}
    GpuLease& operator=(GpuLease&& other) noexcept {
        if (this != &other) {
            reset();
            resources_ = other.resources_;
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }
    H get() const noexcept { return handle_; }
    explicit operator bool() const noexcept { return bool(handle_); }
    void reset() noexcept {
        if (handle_)
            resources_->destroy(std::exchange(handle_, {}));
    }

  private:
    GpuResources* resources_ = nullptr;
    H handle_{};
};
} // namespace blobber

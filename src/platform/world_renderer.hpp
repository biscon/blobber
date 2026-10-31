#pragma once
#include "assets/world_assets.hpp"
#include "platform/gpu_resources.hpp"
#include <memory>
namespace blobber {
struct WorldImage {
    TargetHandle target{};
    std::uint64_t revision = 0;
};
struct WorldDrawStats {
    unsigned draws = 0, chunks = 0;
    bool cached = false;
};
class WorldRenderer {
  public:
    WorldRenderer(GpuResources&, world_assets::Publication);
    ~WorldRenderer() = default;
    WorldRenderer(const WorldRenderer&) = delete;
    WorldRenderer& operator=(const WorldRenderer&) = delete;
    void stage(GeometryView, const MovementState&, world_render::Surfaces = {},
               const std::vector<world_render::Surfaces>& overrides = {}, world_render::Limits = {});
    bool commit() noexcept;
    void cancel() noexcept { staged_.reset(); }
    WorldImage render(const MovementState&);
    void supersampling(bool enabled) noexcept { supersampling_ = enabled; }
    bool supersampling() const noexcept { return supersampling_; }
    bool staged() const noexcept { return bool(staged_); }
    WorldImage image() const noexcept;
    // Diagnostic readback only: scratch from the last supersampled draw, not a published scene.
    TargetHandle supersampledTarget() const noexcept { return high_.get(); }
    struct PreparationStats {
        double meshMs = 0, uploadMs = 0, firstDrawMs = 0;
    };
    PreparationStats preparation{};
    WorldDrawStats stats{};
    std::size_t cpuBytes() const noexcept;
    std::size_t gpuBytes() const noexcept;
    int anisotropy() const noexcept { return anisotropy_; }
    const world_render::Scene* scene() const noexcept { return active_ ? &active_->cpu : nullptr; }

  private:
    struct View {
        world_render::Scene cpu;
        GpuLease<MeshHandle> mesh;
        GpuLease<TargetHandle> output;
        world_render::Camera last{};
        bool ready = false, quality = false;
        std::uint64_t revision = 0;
    };
    GpuResources& resources_;
    world_assets::Publication publication_;
    std::vector<GpuLease<TextureHandle>> textures_;
    std::vector<std::vector<unsigned char>> alpha_;
    std::array<TextureHandle, 32> materials_{};
    unsigned materialCount_ = 0;
    GpuLease<ShaderHandle> world_, resolve_;
    GpuLease<TargetHandle> high_;
    std::unique_ptr<View> active_, staged_;
    world_render::DrawList draws_;
    int mvp_ = -1, view_ = -1, forward_ = -1, anisotropy_ = 1;
    bool supersampling_ = true;
    std::uint64_t revision_ = 0;
    WorldImage render(View&, const MovementState&);
};
} // namespace blobber

#pragma once
#include "assets/world_assets.hpp"
#include "platform/gpu_resources.hpp"
#include "render/world_effects.hpp"
#include "render/world_visibility.hpp"
#include <memory>
namespace blobber {
struct WorldImage {
    TargetHandle target{};
    std::uint64_t revision = 0;
};
struct WorldDrawStats {
    unsigned draws = 0, chunks = 0;
    bool cached = false;
    double visibilityMs = 0, submissionMs = 0;
};
class WorldRenderer {
  public:
    WorldRenderer(GpuResources&, world_assets::Publication, bool supersampling = true);
    WorldRenderer(GpuResources&, std::shared_ptr<const world_assets::CpuAssets>, bool supersampling = true);
    ~WorldRenderer() = default;
    WorldRenderer(const WorldRenderer&) = delete;
    WorldRenderer& operator=(const WorldRenderer&) = delete;
    void stage(GeometryView, const MovementState&, world_render::Surfaces = {},
               const std::vector<world_render::Surfaces>& overrides = {}, world_render::Limits = {});
    void stage(const world_render::FrameInput&);
    bool commit() noexcept;
    void clear() noexcept {
        staged_.reset();
        active_.reset();
    }
    void cancel() noexcept { staged_.reset(); }
    WorldImage render(const MovementState&);
    WorldImage render(const world_render::FrameInput&);
    WorldImage renderStaged(const world_render::FrameInput&);
    const PickingBuffer* picking() const noexcept { return active_ ? active_->publishedPicking : nullptr; }
    const world_render::VisibilityPipeline* visibility() const noexcept {
        return active_ ? active_->visibility.get() : nullptr;
    }
    const PickingBuffer* stagedPicking() const noexcept {
        return staged_ ? staged_->publishedPicking : nullptr;
    }
    const world_render::VisibilityPipeline* stagedVisibility() const noexcept {
        return staged_ ? staged_->visibility.get() : nullptr;
    }
    const world_assets::CpuAssets& assets() const noexcept { return *assets_; }
    // Diagnostic only; caller owns a 500x282 target, no gameplay readback.
    void diagnosticIds(TargetHandle);
    void measureFrames(bool enabled) noexcept { measure_ = enabled; }
    void supersampling(bool enabled); // Preparation boundary if high targets do not exist yet.
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
    struct MemoryReport {
        std::size_t installedTextures = 0, residentTextures = 0, residentAlpha = 0, residentMeshes = 0;
        std::size_t worldGpu = 0, applicationGpu = 0, peakApplicationGpu = 0;
        unsigned textureCount = 0;
    };
    MemoryReport memory() const noexcept;
    int anisotropy() const noexcept { return anisotropy_; }
    const world_render::Scene* scene() const noexcept { return active_ ? &active_->cpu : nullptr; }

  private:
    using SharedTexture = std::shared_ptr<GpuLease<TextureHandle>>;
    struct View {
        world_assets::AssetLease assets;
        std::vector<SharedTexture> textureLeases;
        std::vector<TextureHandle> textures;
        world_render::Scene cpu;
        world_assets::Pack pack;
        std::array<TextureHandle, 32> materials{};
        std::unique_ptr<world_render::VisibilityPipeline> visibility;
        std::unique_ptr<world_render::EffectsPipeline> effects;
        std::uint64_t effectRevision = 0;
        const PickingBuffer* publishedPicking = nullptr;
        std::uint64_t contentRevision = 0;
        MovementState movement{};
        GpuLease<MeshHandle> mesh;
        GpuLease<TargetHandle> output;
        world_render::Camera last{};
        bool ready = false, quality = false;
        std::uint64_t revision = 0;
    };
    GpuResources& resources_;
    std::shared_ptr<const world_assets::CpuAssets> assets_;
    const world_assets::Publication& publication_;
    std::vector<std::weak_ptr<GpuLease<TextureHandle>>> textures_;
    std::size_t peakApplicationGpu_ = 0;
    void acquireTextures(View&, const world_assets::AssetRequirements&);
    void stagePrepared(GeometryView, const MovementState&, world_render::Surfaces,
                       const std::vector<world_render::Surfaces>&, world_render::Limits,
                       const world_assets::AssetRequirements&);
    GpuLease<ShaderHandle> world_, resolve_, billboard_, sky_, creatureComposite_;
    GpuLease<MeshHandle> quad_, proxy_;
    std::array<int, 12> billboardLocations_{};
    void prepareBillboards();
    enum class BillboardPass { Scenery, LeafArt, Creatures };
    void drawBillboards(const View&, const world_render::WorldFrame&, const float*, const float*, bool,
                        const world_render::Scene*, BillboardPass);
    void drawEffects(const View&, const world_render::EffectFrame&, const float*, const float*, bool ground,
                     const world_render::WorldFrame&);
    int model_ = -1, idMode_ = -1, idColor_ = -1, uvMode_ = -1, doorUv_ = -1;
    GpuLease<TargetHandle> high_, creaturesLow_, creaturesHigh_;
    std::unique_ptr<View> active_, staged_;
    world_render::DrawList draws_;
    int mvp_ = -1, view_ = -1, forward_ = -1, anisotropy_ = 1;
    bool supersampling_ = true, measure_ = false;
    std::uint64_t revision_ = 0;
    WorldImage renderInput(View&, const world_render::FrameInput&);
    WorldImage render(View&, const MovementState&, const world_render::WorldFrame* = nullptr,
                      TargetHandle ids = {}, const world_render::EffectFrame* effects = nullptr);
};
} // namespace blobber

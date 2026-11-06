#pragma once
#include "game/actors.hpp"
#include "game/dialogue.hpp"
#include "render/world_scene.hpp"
namespace blobber::world_render {
Camera dialogueCamera(const Scene&, const world_assets::CpuAssets&, GeometryView, const Camera& source,
                      const ActorState&, const DialogueFraming&);
Camera interpolateCamera(const Camera&, const Camera&, float weight) noexcept;
// Per-view presentation state. No resources, allocations or actor ownership.
class DialogueCameraController {
  public:
    std::optional<Camera> update(const Scene&, const world_assets::CpuAssets&, GeometryView,
                                 const Camera& exploration, const ActorState* primary,
                                 const ActorState* target, const DialoguePlayer&);
    bool moving() const noexcept { return active_ && durationMs_ && elapsedMs_ < durationMs_; }

  private:
    Camera exploration_{}, primary_{}, current_{}, from_{}, to_{};
    bool active_ = false, leaving_ = false;
    unsigned target_ = 0, durationMs_ = 0;
    std::uint64_t startMs_ = 0, elapsedMs_ = 0, serial_ = 0;
    void sample(std::uint64_t) noexcept;
    void transition(const Scene&, const Camera&, unsigned, std::uint64_t);
};
} // namespace blobber::world_render

#pragma once
#include "render/scene_renderer.hpp"

namespace blobber {
// Owns all live frame scratch storage. A map/scene change requires a fresh pipeline.
class FramePipeline {
  public:
    bool update(const SceneRenderer& renderer, const Map& map, const MovementState& state,
                const ActorWorld* npcs = nullptr, const NpcAnimationSnapshot* animation = nullptr);
    const Frame& frame() const noexcept { return current_; }

  private:
    Frame source_, destination_, current_;
    std::uint64_t revision_ = 0, animationRevision_ = 0, actorRevision_ = 0, actorWorld_ = 0;
    bool hadAnimation_ = false;
    bool ready_ = false, wasAnimating_ = false;
};
} // namespace blobber

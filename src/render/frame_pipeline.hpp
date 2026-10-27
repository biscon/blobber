#pragma once
#include "render/scene_renderer.hpp"

namespace blobber {
// Owns all live frame scratch storage. A map/scene change requires a fresh pipeline.
class FramePipeline {
  public:
    bool update(const SceneRenderer& renderer, const Map& map, const MovementState& state,
                const ActorWorld* npcs = nullptr, const NpcAnimationSnapshot* animation = nullptr,
                const AttackVisualSnapshot* attacks = nullptr, std::uint64_t now = 0,
                const SpellArea* area = nullptr);
    const Frame& frame() const noexcept { return current_; }
    const PickingBuffer* picking() const noexcept { return ready_ && !wasAnimating_ ? &picking_ : nullptr; }

  private:
    Frame source_, destination_, current_;
    PickingBuffer picking_, effectPicking_;
    ProjectilePlayer projectiles_;
    std::uint64_t revision_ = 0, animationRevision_ = 0, actorRevision_ = 0, actorWorld_ = 0;
    SpellArea area_{};
    bool hadAnimation_ = false;
    bool ready_ = false, wasAnimating_ = false;
};
} // namespace blobber

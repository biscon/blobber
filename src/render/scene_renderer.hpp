#pragma once
#include "game/movement.hpp"
#include "render/npc_sprites.hpp"
#include "render/scene_cache.hpp"

namespace blobber {
class SceneRenderer {
  public:
    explicit SceneRenderer(const Materials& materials, const std::vector<NpcAppearance>& npcs = {})
        : views_(prepareScene(materials)), npcs_(npcs) {}
    std::size_t npcPreparedPixelBytes() const noexcept { return npcs_.preparedPixelBytes(); }
    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;
    void compose(const Map& map, Player player, Frame& output, const ActorWorld* npcs = nullptr,
                 const NpcAnimationSnapshot* animation = nullptr, PickingBuffer* picking = nullptr) const;
    void composeStrafe(const Map& map, Player from, Player to, float t, Frame& output,
                       const ActorWorld* npcs = nullptr,
                       const NpcAnimationSnapshot* animation = nullptr) const;

  private:
    void composeView(const Map& map, Player base, int phase, Frame& output, const ActorWorld* npcs = nullptr,
                     const NpcAnimationSnapshot* animation = nullptr, PickingBuffer* picking = nullptr) const;
    PreparedScene views_;
    PreparedNpcs npcs_;
};
} // namespace blobber

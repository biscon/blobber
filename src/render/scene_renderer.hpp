#pragma once
#include "game/movement.hpp"
#include "render/loot_sprites.hpp"
#include "render/npc_sprites.hpp"
#include "render/projectiles.hpp"
#include "render/scene_cache.hpp"

namespace blobber {
struct SpellArea;
class SceneRenderer {
  public:
    explicit SceneRenderer(const Materials& materials, const std::vector<NpcAppearance>& npcs = {})
        : views_(prepareScene(materials)), npcs_(npcs) {
        prepareFloorPreview();
    }
    SceneRenderer(PreparedScene&& scene, PreparedNpcs&& npcs,
                  const std::vector<NpcAttachments>& attachments = {}, PreparedLoot&& loot = {})
        : views_(std::move(scene)), npcs_(std::move(npcs)), loot_(std::move(loot)) {
        npcs_.setAttachments(attachments);
        prepareFloorPreview();
    }
    std::size_t npcPreparedPixelBytes() const noexcept { return npcs_.preparedPixelBytes(); }
    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;
    static std::optional<Cell> groundCell(GeometryView, Player, int x, int y) noexcept;
    void compose(GeometryView map, Player player, Frame& output, const ActorWorld* npcs = nullptr,
                 const NpcAnimationSnapshot* animation = nullptr, PickingBuffer* picking = nullptr,
                 const ProjectilePlayer* projectiles = nullptr, const SpellArea* area = nullptr,
                 const LootSnapshot* loot = nullptr) const;
    void composeStrafe(GeometryView map, Player from, Player to, float t, Frame& output,
                       const ActorWorld* npcs = nullptr, const NpcAnimationSnapshot* animation = nullptr,
                       PickingBuffer* picking = nullptr, const ProjectilePlayer* projectiles = nullptr,
                       const SpellArea* area = nullptr, const LootSnapshot* loot = nullptr) const;

  private:
    void composeView(GeometryView map, Player base, int phase, Frame& output,
                     const ActorWorld* npcs = nullptr, const NpcAnimationSnapshot* animation = nullptr,
                     PickingBuffer* picking = nullptr, const ProjectilePlayer* projectiles = nullptr,
                     const SpellArea* area = nullptr, const LootSnapshot* loot = nullptr) const;
    static void composeWeapons(const PreparedView&, const GroundSnapshot&, Player, int,
                               const std::array<float, viewWidth>&, const ActorWorld*,
                               const NpcAnimationSnapshot*, Frame&, PickingBuffer&) noexcept;
    void prepareFloorPreview();
    struct FloorSample {
        std::int8_t lateral = 0, forward = -1;
        bool edge = false;
    };
    std::vector<std::array<FloorSample, viewWidth * viewHeight>> floorSamples_;
    std::array<float, viewHeight> floorDepth_{};
    PreparedScene views_;
    PreparedNpcs npcs_;
    PreparedLoot loot_;
    mutable PickingBuffer lootScratch_;
};
} // namespace blobber

#pragma once
#include "assets/world_assets.hpp"
#include "game/combat.hpp"
#include "render/picking.hpp"
#include "render/world_doors.hpp"
#include "render/world_scene.hpp"

namespace blobber::world_render {
constexpr unsigned billboardCapacity = NpcWorld::maxInstances + lootCapacity + fixtureCapacity;
constexpr std::size_t viewScratchBudget = 8u * 1024 * 1024;
constexpr const char* visibilityVersion = "world-visibility-v2";
struct FrameInput {
    GeometryView geometry;
    const MovementState& movement;
    const ActorWorld* actors = nullptr;
    const NpcAnimationSnapshot* animation = nullptr;
    const LootSnapshot* loot = nullptr;
    const DoorSnapshot* doors = nullptr;
    const AttackVisualSnapshot* attacks = nullptr;
    const SpellArea* area = nullptr;
    std::uint64_t now = 0;
};
struct Billboard {
    Vec feet;
    float width = 0, height = 0, anchorX = .5f, anchorY = 1, depth = 0, shadowRadius = 0;
    std::uint16_t texture = UINT16_MAX, id = PickingBuffer::empty;
    bool alive = false, flash = false;
    NpcAttachment release{}, torso{};
};
struct WorldFrame {
    Camera camera{};
    std::array<Billboard, billboardCapacity> billboards{};
    unsigned count = 0;
    std::array<DoorBox, doorBoxCapacity> doors{};
    unsigned doorCount = 0;
    std::array<Cell, fixtureCapacity> proxies{};
    std::array<std::uint16_t, fixtureCapacity> proxyIds{};
    unsigned proxyCount = 0;
    PickingBuffer picking{true};
    std::array<LootId, lootCapacity> visibleLoot{};
    std::uint64_t revision = 0;
    bool moving = false;
};
// Borrows immutable scene/assets. Both must outlive this immovable prepared owner.
class VisibilityPipeline {
  public:
    VisibilityPipeline(const Scene&, const world_assets::CpuAssets&);
    VisibilityPipeline(const VisibilityPipeline&) = delete;
    bool update(const FrameInput&);
    const WorldFrame& frame() const noexcept { return frames_[current_]; }
    const PickingBuffer* picking() const noexcept {
        return ready_ && !frame().moving ? &frame().picking : nullptr;
    }
    bool lootVisible(LootId id) const noexcept;
    std::optional<Cell> groundCell(const FrameInput&, int x, int y) const noexcept;
    std::size_t bytes() const noexcept;

  private:
    const Scene& scene_;
    const Map* map_ = nullptr;
    const world_assets::CpuAssets& assets_;
    std::array<WorldFrame, 2> frames_;
    std::array<std::uint16_t, lootCapacity> lootOrder_{};
    std::array<unsigned, 7> icons_{};
    std::array<std::uint64_t, 10> stamp_{};
    PickingBuffer opaqueCache_{true};
    unsigned current_ = 0;
    bool ready_ = false;
    void build(WorldFrame&, const FrameInput&);
    void raster(WorldFrame&, bool rebuildOpaque);
};
// Shared CPU/GPU model geometry. Unit quad uses x[-.5,.5], y[0,1], v=1-y.
Scene billboardMesh();
Scene proxyMesh();
std::array<Vec, 4> corners(const Camera&, const Billboard&) noexcept;
struct Attachment {
    Vec position{};
    float depth = 0;
    bool valid = false;
};
Attachment attachment(const Camera&, const Billboard&, NpcAttachment) noexcept;
} // namespace blobber::world_render

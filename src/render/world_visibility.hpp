#pragma once
#include "assets/world_assets.hpp"
#include "game/combat.hpp"
#include "render/picking.hpp"
#include "render/world_doors.hpp"
#include "render/world_scene.hpp"

namespace blobber::world_render {
constexpr unsigned billboardCapacity = NpcWorld::maxInstances + lootCapacity + fixtureCapacity;
constexpr std::size_t viewScratchBudget = 8u * 1024 * 1024;
constexpr const char* visibilityVersion = "world-visibility-v15";
// Rectangles in stored texture UVs, not logical canvas coordinates.
struct BillboardClipRect {
    float left = 0, top = 0, right = 0, bottom = 0;
};
static_assert(sizeof(BillboardClipRect) == 4 * sizeof(float));
constexpr unsigned terrainClipCapacity = 8;
constexpr unsigned billboardClipCapacity = terrainClipCapacity + doorBoxCapacity;
struct TerrainClips {
    std::array<BillboardClipRect, terrainClipCapacity> rectangles{};
    unsigned count = 0;
};
struct BillboardClips {
    std::array<BillboardClipRect, billboardClipCapacity> rectangles{};
    unsigned count = 0;
    bool contains(float u, float v) const noexcept;
};
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
    unsigned guardianDoor = UINT16_MAX;
};
struct Billboard {
    Vec feet;
    float width = 0, height = 0, anchorX = .5f, anchorY = 1, depth = 0, shadowRadius = 0;
    std::uint16_t texture = UINT16_MAX, id = PickingBuffer::empty;
    bool alive = false, flash = false;
    NpcAttachment release{}, torso{};
    // Drawable crop in logical canvas space; attachments/shadows keep that space.
    struct Region {
        float left = 0, top = 0, right = 1, bottom = 1;
    } crop{};
};
// Three vec4s per leaf, directly consumable by the GL 3.3 shader. Axes are
// inverse box axes; mode is 0 geometric, 1 in front, 2 behind.
struct LeafQuery {
    std::array<float, 4> originHeight{}, xMode{}, zAxis{};
};
static_assert(sizeof(LeafQuery) == 12 * sizeof(float));
struct LeafQueries {
    std::array<LeafQuery, fixtureCapacity> leaves{};
    unsigned count = 0;
    bool hides(const Camera&, Vec ray, float depth, const std::uint8_t* relations = nullptr) const noexcept;
};
struct WorldFrame {
    Camera camera{};
    LeafQueries leaves;
    std::array<std::array<std::uint8_t, fixtureCapacity>, NpcWorld::maxInstances> bodyLeafRelations{};
    std::unique_ptr<float[]> bodyOpaqueDepth = std::make_unique<float[]>(viewWidth * viewHeight);
    std::array<int, NpcWorld::maxInstances> bodyIndex{};
    std::array<TerrainClips, NpcWorld::maxInstances> terrainClips{};
    std::array<bool, containerCapacity> containerOpened{};
    std::array<Billboard, billboardCapacity> billboards{};
    unsigned count = 0;
    std::array<DoorBox, doorBoxCapacity> doors{};
    unsigned doorCount = 0;
    std::uint16_t crestTexture = 0, vineTexture = 0;
    std::array<Cell, fixtureCapacity> proxies{};
    std::array<std::uint16_t, fixtureCapacity> proxyIds{};
    unsigned proxyCount = 0;
    PickingBuffer picking;
    std::array<LootId, lootCapacity> visibleLoot{};
    std::array<ContainerId, containerCapacity> visibleContainers{};
    std::uint64_t revision = 0;
    // Raw picking pixels/depth (including bodyOpaqueDepth) are valid only when
    // rasterized. Moving presentation may deliberately retain stale raster storage.
    bool moving = false, rasterized = false;
};
// Borrows immutable scene/assets. Both must outlive this immovable prepared owner.
class VisibilityPipeline {
  public:
    VisibilityPipeline(const Scene&, const world_assets::CpuAssets&);
    VisibilityPipeline(const VisibilityPipeline&) = delete;
    // Standalone diagnostics retain all-frame CPU coverage; interactive callers
    // skip unavailable picking during movement.
    bool update(const FrameInput&, bool rasterMoving = true);
    // Populate raw IDs/depth on demand; does not publish moving interaction.
    void rasterForDiagnostics();
    const WorldFrame& frame() const noexcept { return frames_[current_]; }
    const PickingBuffer* picking() const noexcept {
        return ready_ && !frame().moving ? &frame().picking : nullptr;
    }
    bool lootVisible(LootId id) const noexcept;
    bool containerVisible(ContainerId id) const noexcept {
        return ready_ && id.slot < containerCapacity &&
               frame().visibleContainers[id.slot].world == id.world && bool(id);
    }
    std::optional<Cell> groundCell(const FrameInput&, int x, int y) const noexcept;
    std::size_t bytes() const noexcept;

  private:
    const Scene& scene_;
    const Map* map_ = nullptr;
    const world_assets::CpuAssets& assets_;
    world_assets::Pack pack_;
    std::array<WorldFrame, 2> frames_;
    std::array<std::uint16_t, lootCapacity> lootOrder_{};
    std::array<unsigned, 5> icons_{};
    std::array<std::uint64_t, 11> stamp_{};
    PickingBuffer opaqueCache_, compositionScratch_;
    unsigned current_ = 0;
    bool ready_ = false;
    void build(WorldFrame&, const FrameInput&);
    void raster(WorldFrame&, bool rebuildOpaque);
    void finishPicking(WorldFrame&, const FrameInput&, bool rebuildOpaque);
};
// Shared CPU/GPU model geometry. Unit quad uses x[-.5,.5], y[0,1], v=1-y.
Scene billboardMesh();
Scene proxyMesh();
std::array<Vec, 4> corners(const Camera&, const Billboard&) noexcept;
void prepareTerrainClips(GeometryView, const Camera&, const Billboard&, TerrainClips&);
// Reuses caller-owned scratch; only NPC bodies receive masks.
void billboardClips(const WorldFrame&, const Billboard&, BillboardClips&) noexcept;
// null body requests ordinary geometric occlusion (loot/markers).
void prepareLeafQueries(const WorldFrame&, const Billboard*, LeafQueries&, bool cullToBody = false) noexcept;
struct Attachment {
    Vec position{};
    float depth = 0;
    bool valid = false;
};
Attachment attachment(const Camera&, const Billboard&, NpcAttachment) noexcept;
} // namespace blobber::world_render

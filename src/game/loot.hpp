#pragma once
#include "game/actors.hpp"
#include "game/recovery.hpp"
namespace blobber {
constexpr std::size_t lootPerActor = 8, lootReservedCapacity = NpcWorld::maxInstances * lootPerActor + 64;
constexpr std::size_t lootDropCapacity = 256, lootCapacity = lootReservedCapacity + lootDropCapacity;
struct LootId {
    std::uint64_t world = 0;
    std::uint16_t slot = UINT16_MAX;
    std::uint64_t generation = 1;
    explicit operator bool() const noexcept { return world && generation && slot < lootCapacity; }
};
inline bool operator==(LootId a, LootId b) noexcept {
    return a.world == b.world && a.slot == b.slot && a.generation == b.generation;
}
struct FloorLoot {
    Cell cell{};
    rules::ItemId item{};
    std::uint16_t quantity = 0;
    bool active = false;
    std::uint64_t generation = 1;
};
struct LootSnapshot {
    std::array<FloorLoot, lootCapacity> stacks{};
    std::uint64_t world = 0, revision = 1;
};
struct PickupRequest {
    LootId loot;
    rules::CharacterId recipient;
    std::uint64_t inventoryRevision = 0;
};
struct PickupResult {
    InventoryError error = InventoryError::None;
    const char* reason = "Collected.";
    explicit operator bool() const noexcept { return error == InventoryError::None; }
};
// Death reservations never recycle; separate player-drop slots use checked generations.
class LootWorld {
  public:
    LootWorld(const ActorWorld&, const RecoveryLevel&, std::uint32_t seed);
    const LootSnapshot& snapshot() const noexcept { return state_; }
    const FloorLoot* find(LootId) const noexcept;
    void reconcile(const ActorWorld&) noexcept;
    bool collect(LootId) noexcept;

    // Preparation-only owned values; no borrowed pointers or navigation workspace.
    struct Saved {
        LootSnapshot state;
        std::array<bool, NpcWorld::maxInstances> deaths{};
        std::uint64_t actorRevision = 0;
    };

  private:
    friend class GameSession;
    friend class GameSession;
    LootId availableDrop() const noexcept;
    void publishDrop(LootId, Cell, rules::ItemId, std::uint16_t) noexcept;
    LootSnapshot state_;
    std::array<bool, NpcWorld::maxInstances> deaths_{};
    std::uint64_t actorRevision_ = 0;
};
} // namespace blobber

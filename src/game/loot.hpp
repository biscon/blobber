#pragma once
#include "game/actors.hpp"
#include "game/recovery.hpp"
namespace blobber {
constexpr std::size_t lootPerActor = 8, lootCapacity = NpcWorld::maxInstances * lootPerActor + 64;
struct LootId {
    std::uint64_t world = 0;
    std::uint16_t slot = UINT16_MAX;
    explicit operator bool() const noexcept { return world && slot < lootCapacity; }
};
inline bool operator==(LootId a, LootId b) noexcept {
    return a.world == b.world && a.slot == b.slot;
}
struct FloorLoot {
    Cell cell{};
    rules::ItemId item{};
    std::uint16_t quantity = 0;
    bool active = false;
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
// Fixed reservations guarantee every death can publish all rewards. No slot reuse.
class LootWorld {
  public:
    LootWorld(const ActorWorld&, const RecoveryLevel&, std::uint32_t seed);
    const LootSnapshot& snapshot() const noexcept { return state_; }
    const FloorLoot* find(LootId) const noexcept;
    void reconcile(const ActorWorld&) noexcept;
    bool collect(LootId) noexcept;

  private:
    LootSnapshot state_;
    std::array<bool, NpcWorld::maxInstances> deaths_{};
    std::uint64_t actorRevision_ = 0;
};
} // namespace blobber

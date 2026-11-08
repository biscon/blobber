#pragma once
#include "rules/content.hpp"

namespace blobber {
using rules::bagCells;
using rules::EquipmentSlot;
using rules::equipmentSlots;
using rules::inventoryCells;
constexpr std::size_t inventoryCapacity = rules::partySize * inventoryCells;
struct ItemHandle {
    std::uint64_t owner = 0, generation = 0;
    std::uint16_t slot = UINT16_MAX;
    explicit operator bool() const noexcept { return owner && generation && slot < inventoryCapacity; }
};
inline bool operator==(ItemHandle a, ItemHandle b) noexcept {
    return a.owner == b.owner && a.generation == b.generation && a.slot == b.slot;
}
inline bool operator!=(ItemHandle a, ItemHandle b) noexcept {
    return !(a == b);
}
struct ItemLocation {
    rules::CharacterId character{};
    std::uint16_t cell = UINT16_MAX; // Bag then equipment; sentinel auto-transfer.
};
struct ItemStack {
    rules::ItemId item{};
    std::uint16_t quantity = 0;
    std::uint64_t generation = 1;
};
enum class InventoryError : std::uint8_t {
    None,
    Stale,
    InvalidItem,
    InvalidDestination,
    InvalidQuantity,
    Full,
    Overweight,
    Equipment,
    Unsafe,
    Moving,
    Unavailable,
    Funds,
    GoldOverflow
};
const char* inventoryMessage(InventoryError) noexcept;
enum class InventoryOp : std::uint8_t { Move, Split, Transfer };
struct InventoryRequest {
    InventoryOp operation = InventoryOp::Move;
    ItemHandle source;
    ItemLocation destination;
    std::uint16_t quantity = 0; // Zero means entire stack; Move/merge means what fits.
    std::uint64_t revision = 0;
};
struct InventoryResult {
    InventoryError error = InventoryError::None;
    bool changed = false;
    explicit operator bool() const noexcept { return error == InventoryError::None; }
};
struct AcquisitionRequest {
    rules::CharacterId character;
    rules::ItemId item;
    std::uint16_t quantity = 0;
    std::uint64_t revision = 0;
};
struct InventoryExchange {
    rules::CharacterId character;
    ItemHandle sold;
    rules::ItemId bought;
    unsigned quantity = 0;
    std::int64_t gold = 0; // Positive proceeds, negative payment; shared party purse.
    std::uint64_t revision = 0;
};
// Value storage; Party stages a copy before committing. Catalog and character strings stay outside it.
class Inventory {
  public:
    struct Saved {
        std::array<ItemStack, inventoryCapacity> stacks{};
        std::array<std::array<std::uint16_t, inventoryCells>, rules::partySize> locations = [] {
            std::array<std::array<std::uint16_t, inventoryCells>, rules::partySize> value{};
            for (auto& row : value)
                row.fill(UINT16_MAX);
            return value;
        }();
        std::uint64_t revision = 1;
    };
    Inventory();
    Saved checkpoint() const noexcept;
    void restore(const Saved&, const rules::Catalog&); // Preparation only; fresh owner identity.

    const ItemStack* stack(ItemHandle) const noexcept;
    ItemHandle at(ItemLocation) const noexcept;
    ItemLocation locate(ItemHandle) const noexcept;
    std::uint64_t revision() const noexcept { return revision_; }
    std::uint64_t identity() const noexcept { return identity_; }
    rules::CharacterLoadout loadout(rules::CharacterId) const noexcept;
    unsigned count(rules::CharacterId, rules::Ammunition, const rules::Catalog&) const noexcept;
    ItemHandle ammunition(rules::CharacterId, rules::Ammunition, const rules::Catalog&) const noexcept;

  private:
    friend class Party;
    void initialize(const rules::Catalog&, const rules::PresetParty&);
    InventoryResult apply(const InventoryRequest&, const rules::Catalog&) noexcept;
    bool consume(ItemHandle, unsigned quantity) noexcept;
    InventoryResult acquire(const AcquisitionRequest&, const rules::Catalog&) noexcept;

    std::array<ItemStack, inventoryCapacity> stacks_{};
    std::array<std::array<ItemHandle, inventoryCells>, rules::partySize> locations_{};
    std::uint64_t identity_ = 0, revision_ = 1;
    ItemHandle create(rules::ItemId, unsigned) noexcept;
    void erase(ItemHandle) noexcept;
    InventoryResult move(const InventoryRequest&, const rules::Catalog&) noexcept;
};
} // namespace blobber

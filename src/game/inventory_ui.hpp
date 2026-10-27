#pragma once
#include "game/command_types.hpp"
namespace blobber {
enum class InventoryMode : std::uint8_t { Browse, Move, Split, Transfer, Quantity };
enum class InventoryAction : std::uint8_t { Equip, Move, Split, Transfer, Use };
struct InventoryCell {
    ItemHandle handle;
    const rules::ItemDefinition* definition = nullptr;
    unsigned quantity = 0;
    rules::WeaponPreview weapon;
};
struct InventoryHud {
    bool open = false, pointerDown = false, dragging = false, splitDrag = false, itemTargeting = false;
    rules::CharacterId character{0};
    std::uint16_t cursor = 0;
    std::array<InventoryCell, 52> cells{};
    ItemHandle selected{}, source{};
    InventoryMode mode = InventoryMode::Browse;
    InventoryRequest pending;
    unsigned quantity = 1, maximum = 1;
    std::uint64_t revision = 0;
    float startX = 0, startY = 0, pointerX = 0, pointerY = 0;
    std::int64_t weight = 0, capacity = 0;
    int armorClass = 0, descriptionPage = 0;
    InventoryError management = InventoryError::None;
    std::array<char, 192> feedback{};
    PartyCommand itemCommand;
    std::array<CommandReason, 4> recipients{};
    const rules::ItemDefinition* usingItem = nullptr;
};
} // namespace blobber

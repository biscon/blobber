#pragma once
#include "game/inventory.hpp"

namespace blobber {
class Party;
std::optional<EquipmentSlot> itemEquipmentSlot(const rules::ItemDefinition&) noexcept;
struct ItemComparison {
    const rules::ItemDefinition* equipped = nullptr;
    rules::WeaponPreview offeredWeapon, equippedWeapon;
    int offeredArmorClass = 0, equippedArmorClass = 0;
    rules::CharacterStats currentStats{}, offeredStats{};
    bool equippable = false;
    const char* restriction = "";
};
ItemComparison compareItem(const Party&, rules::CharacterId, const rules::ItemDefinition&) noexcept;
} // namespace blobber

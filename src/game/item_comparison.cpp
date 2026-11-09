#include "game/item_comparison.hpp"
#include "game/party.hpp"
#include <algorithm>

namespace blobber {
std::optional<EquipmentSlot> itemEquipmentSlot(const rules::ItemDefinition& item) noexcept {
    const auto slot = rules::itemEquipmentPosition(item);
    return slot == EquipmentSlot::Count ? std::nullopt : std::optional<EquipmentSlot>{slot};
}
ItemComparison compareItem(const Party& party, rules::CharacterId who,
                           const rules::ItemDefinition& offered) noexcept {
    ItemComparison result;
    const auto* c = party.character(who);
    auto slot = itemEquipmentSlot(offered);
    if (!c || !slot)
        return result;
    if (*slot == EquipmentSlot::Ring1 && party.equipped(who, EquipmentSlot::Ring1) &&
        !party.equipped(who, EquipmentSlot::Ring2))
        slot = EquipmentSlot::Ring2;
    result.equippable = true;
    result.equipped = party.equipped(who, *slot);
    result.equippedArmorClass = result.offeredArmorClass = c->stats.armorClass;
    const auto index = unsigned(*slot);
    result.equippedWeapon = c->stats.weapons[index];
    auto loadout = party.inventory().loadout(who);
    const auto offeredId = party.catalog().findItem(offered.id);
    for (unsigned i = 0; i < bagCells; ++i)
        if (loadout.items[i].quantity && loadout.items[i].item.index == offeredId.index) {
            --loadout.items[i].quantity;
            break;
        }
    loadout.items[index] = {offeredId, 1};
    loadout.equipment.set(*slot, {std::uint16_t(index)});
    const auto checked = rules::deriveCharacter(party.catalog(), c->build, loadout);
    auto candidate = checked;
    if (checked.error == rules::CharacterError::InvalidEquipment) {
        // Show the hypothetical usable loadout while retaining the live restriction.
        if (offered.kind == rules::ItemKind::Weapon && (offered.twoHanded || offered.ranged))
            loadout.equipment.offHand = {};
        if (offered.kind == rules::ItemKind::Shield) {
            loadout.equipment.ranged = {};
            const auto* main = party.equipped(who, EquipmentSlot::MainHand);
            if (main && main->twoHanded)
                loadout.equipment.mainHand = {};
        }
        candidate = rules::deriveCharacter(party.catalog(), c->build.view(), loadout, false);
    } else if (checked.error == rules::CharacterError::ArmorProficiency)
        candidate = rules::deriveCharacter(party.catalog(), c->build.view(), loadout, false);
    if (candidate) {
        result.offeredArmorClass = candidate.value.armorClass;
        result.offeredWeapon = candidate.value.weapons[index];
        result.offeredStats = candidate.value;
        result.offeredStats.maxHitPoints += c->aidBonus;
        result.currentStats = c->stats;
        if (checked.error == rules::CharacterError::InvalidEquipment)
            result.restriction = "Unequip conflicting weapon or shield first.";
        else if (checked.error == rules::CharacterError::ArmorProficiency)
            result.restriction = "Missing armor training: cannot equip.";
        else if (result.offeredWeapon.heavyDisadvantage)
            result.restriction = "Heavy weapon: Small wielder has disadvantage.";
        else if (offered.kind == rules::ItemKind::Weapon && !result.offeredWeapon.proficient)
            result.restriction = "No weapon training: proficiency bonus does not apply.";
    } else {
        result.restriction = candidate.error == rules::CharacterError::Overweight
                                 ? "Carried weight exceeds capacity."
                             : candidate.error == rules::CharacterError::ArmorProficiency
                                 ? "Missing armor training: cannot equip."
                                 : "Unequip conflicting weapon or shield first.";
    }
    return result;
}
} // namespace blobber

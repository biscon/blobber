#include "game/party.hpp"
#include "game/trade.hpp"
#include <algorithm>

namespace blobber {
std::optional<EquipmentSlot> itemEquipmentSlot(const rules::ItemDefinition& item) noexcept {
    if (item.kind == rules::ItemKind::Armor)
        return EquipmentSlot::Body;
    if (item.kind == rules::ItemKind::Shield)
        return EquipmentSlot::OffHand;
    if (item.kind == rules::ItemKind::Weapon)
        return item.ranged ? EquipmentSlot::Ranged : EquipmentSlot::MainHand;
    return {};
}
ItemComparison compareItem(const Party& party, rules::CharacterId who,
                           const rules::ItemDefinition& offered) noexcept {
    ItemComparison result;
    const auto* c = party.character(who);
    const auto slot = itemEquipmentSlot(offered);
    if (!c || !slot)
        return result;
    result.equippable = true;
    result.equipped = party.equipped(who, *slot);
    result.equippedArmorClass = result.offeredArmorClass = c->stats.armorClass;
    const auto index = unsigned(*slot);
    result.equippedWeapon = c->stats.weapons[index];
    const auto* shield = party.equipped(who, EquipmentSlot::OffHand);
    const auto* cls = party.catalog().characterClass(c->build.characterClass);
    const auto* race = party.catalog().race(c->build.race);
    if (offered.kind == rules::ItemKind::Weapon) {
        auto& weapon = result.offeredWeapon;
        weapon.available = true;
        weapon.ranged = offered.ranged;
        weapon.proficient = ((cls->weapons | race->weapons) & rules::weaponBit(offered.weapon)) != 0;
        const auto strength = c->stats.modifiers[0], dexterity = c->stats.modifiers[1];
        const bool useDexterity = offered.finesse ? dexterity > strength : offered.ranged;
        weapon.ability = useDexterity ? rules::Ability::Dexterity : rules::Ability::Strength;
        const int modifier = useDexterity ? dexterity : strength;
        weapon.attackBonus = modifier + (weapon.proficient ? c->stats.proficiency : 0);
        weapon.damage = offered.damage;
        weapon.damage.modifier = modifier;
        weapon.damageType = offered.damageType;
        if (offered.versatileSides && !shield && !offered.ranged)
            weapon.damage.sides = offered.versatileSides;
        if (shield && (offered.twoHanded || offered.ranged))
            result.restriction = "Unequip shield before equipping this weapon.";
        else if (!weapon.proficient)
            result.restriction = "No weapon training: proficiency bonus does not apply.";
    } else {
        const auto training =
            cls->armor | (c->build.lifeDomain ? rules::armorBit(rules::ArmorType::Heavy) : 0);
        if (offered.kind == rules::ItemKind::Armor) {
            int ac = offered.baseArmorClass;
            if (offered.armor == rules::ArmorType::Light)
                ac += c->stats.modifiers[1];
            else if (offered.armor == rules::ArmorType::Medium)
                ac += std::min(2, c->stats.modifiers[1]);
            if (shield)
                ac += 2;
            if (c->build.defense)
                ++ac;
            result.offeredArmorClass = ac;
            if (!(training & rules::armorBit(offered.armor)))
                result.restriction = "Missing armor training: cannot equip.";
        } else {
            result.offeredArmorClass = c->stats.armorClass + (shield ? 0 : 2);
            if (!(training & rules::armorBit(rules::ArmorType::Shield)))
                result.restriction = "Missing shield training: cannot equip.";
            else if (party.equipped(who, EquipmentSlot::Ranged) ||
                     (party.equipped(who, EquipmentSlot::MainHand) &&
                      party.equipped(who, EquipmentSlot::MainHand)->twoHanded))
                result.restriction = "Unequip conflicting weapon before equipping shield.";
        }
    }
    return result;
}
} // namespace blobber

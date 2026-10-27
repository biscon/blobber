#include "rules/content.hpp"
#include <algorithm>

namespace blobber::rules {
bool validDisplayText(const std::string& text, std::size_t maximum) noexcept {
    if (text.empty() || text.size() > maximum)
        return false;
    bool nonspace = false;
    for (const unsigned char c : text) {
        if (c < 32 || c > 126)
            return false;
        nonspace |= c != ' ';
    }
    return nonspace;
}
bool validFormation(const std::array<CharacterId, partySize>& formation) noexcept {
    unsigned seen = 0;
    for (const auto id : formation) {
        if (id.index >= partySize || (seen & (1u << id.index)))
            return false;
        seen |= 1u << id.index;
    }
    return true;
}
const char* characterErrorMessage(CharacterError error) noexcept {
    switch (error) {
    case CharacterError::None:
        return "valid character";
    case CharacterError::InvalidIdentity:
        return "invalid character id or name";
    case CharacterError::InvalidReference:
        return "invalid race, class, or item reference";
    case CharacterError::UnsupportedLevel:
        return "only character level 1 is supported";
    case CharacterError::InvalidAbilities:
        return "base and final abilities must be 1..30";
    case CharacterError::InvalidFeature:
        return "class does not support selected feature (Defense or Life domain)";
    case CharacterError::InvalidItems:
        return "invalid item count or quantity";
    case CharacterError::InvalidEquipment:
        return "invalid equipment reference, quantity, slot, or hand conflict";
    case CharacterError::ArmorProficiency:
        return "missing armor or shield proficiency";
    case CharacterError::Overweight:
        return "carried weight exceeds Strength x 15 lb";
    }
    return "unknown character error";
}
CharacterResult deriveCharacter(const Catalog& catalog, const CharacterBuild& b) noexcept {
    const auto fail = [](CharacterError e) { return CharacterResult{{}, e}; };
    if (!validContentId(b.id) || !validDisplayText(b.name, 64))
        return fail(CharacterError::InvalidIdentity);
    const auto* race = catalog.race(b.race);
    const auto* cls = catalog.characterClass(b.characterClass);
    if (!race || !cls)
        return fail(CharacterError::InvalidReference);
    if (b.level != 1)
        return fail(CharacterError::UnsupportedLevel);
    if (b.lifeDomain && !hasFeature(cls->features, ClassFeature::LifeDomain))
        return {{}, CharacterError::InvalidFeature};
    if (b.defense && !cls->allowsDefense)
        return fail(CharacterError::InvalidFeature);
    CharacterStats stats;
    stats.proficiency = proficiencyBonus(b.level).value;
    for (std::size_t i = 0; i < abilityCount; ++i) {
        if (!abilityModifier(b.baseAbilities[i]))
            return fail(CharacterError::InvalidAbilities);
        stats.abilities[i] = b.baseAbilities[i] + race->abilityIncreases[i];
        const auto mod = abilityModifier(stats.abilities[i]);
        if (!mod)
            return fail(CharacterError::InvalidAbilities);
        stats.modifiers[i] = mod.value;
        stats.saves[i] = mod.value + (cls->saves[i] ? stats.proficiency : 0);
    }
    const int strength = stats.modifiers[0], dexterity = stats.modifiers[1];
    stats.initiative = dexterity;
    stats.maxHitPoints = std::max(1, cls->hitDie + stats.modifiers[2]) + race->hitPointsPerLevel;
    stats.carryingCapacity = std::int64_t(stats.abilities[0]) * 1500;
    stats.armorClass = 10 + dexterity;
    if (b.itemCount > maxStartingItems)
        return fail(CharacterError::InvalidItems);
    for (std::size_t i = 0; i < b.itemCount; ++i) {
        const auto* item = catalog.item(b.items[i].item);
        if (!item)
            return fail(CharacterError::InvalidReference);
        if (!b.items[i].quantity)
            return fail(CharacterError::InvalidItems);
        stats.carriedWeight += std::int64_t(item->weight) * b.items[i].quantity;
    }
    if (stats.carriedWeight > stats.carryingCapacity)
        return fail(CharacterError::Overweight);
    const std::array<OwnedItemId, 4> equipped{b.equipment.body, b.equipment.mainHand, b.equipment.offHand,
                                              b.equipment.ranged};
    std::array<const ItemDefinition*, 4> items{};
    for (std::size_t i = 0; i < equipped.size(); ++i) {
        const auto id = equipped[i];
        if (id.index == UINT16_MAX)
            continue;
        if (id.index >= b.itemCount || b.items[id.index].quantity != 1)
            return fail(CharacterError::InvalidEquipment);
        for (std::size_t j = 0; j < i; ++j)
            if (equipped[j].index == id.index)
                return fail(CharacterError::InvalidEquipment);
        items[i] = catalog.item(b.items[id.index].item);
    }
    if ((items[0] && items[0]->kind != ItemKind::Armor) || (items[1] && items[1]->kind != ItemKind::Weapon) ||
        (items[2] && items[2]->kind != ItemKind::Shield) || (items[1] && items[1]->twoHanded && items[2]))
        return fail(CharacterError::InvalidEquipment);
    if (items[3] && (items[3]->kind != ItemKind::Weapon || !items[3]->ranged || items[2]))
        return fail(CharacterError::InvalidEquipment);
    if (items[0]) {
        const auto& armor = *items[0];
        if (!((cls->armor | (b.lifeDomain ? armorBit(ArmorType::Heavy) : 0)) & armorBit(armor.armor)))
            return fail(CharacterError::ArmorProficiency);
        stats.armorClass = armor.baseArmorClass;
        if (armor.armor == ArmorType::Light)
            stats.armorClass += dexterity;
        if (armor.armor == ArmorType::Medium)
            stats.armorClass += std::min(2, dexterity);
        if (b.defense)
            ++stats.armorClass;
    }
    if (items[2]) {
        if (!((cls->armor | (b.lifeDomain ? armorBit(ArmorType::Heavy) : 0)) & armorBit(ArmorType::Shield)))
            return fail(CharacterError::ArmorProficiency);
        stats.armorClass += 2;
    }
    for (std::size_t i = 0; i < b.itemCount; ++i) {
        const auto& item = *catalog.item(b.items[i].item);
        if (item.kind != ItemKind::Weapon)
            continue;
        auto& preview = stats.weapons[i];
        preview.available = true;
        preview.ranged = item.ranged;
        preview.proficient = ((race->weapons | cls->weapons) & weaponBit(item.weapon)) != 0;
        preview.ability =
            (item.finesse ? dexterity > strength : item.ranged) ? Ability::Dexterity : Ability::Strength;
        const int modifier = stats.modifiers[static_cast<std::size_t>(preview.ability)];
        preview.attackBonus = modifier + (preview.proficient ? stats.proficiency : 0);
        preview.damage = item.damage;
        preview.damage.modifier = modifier;
        if (item.versatileSides && b.equipment.mainHand.index == i && !items[2])
            preview.damage.sides = item.versatileSides;
        preview.damageType = item.damageType;
    }
    return {stats, CharacterError::None};
}
} // namespace blobber::rules

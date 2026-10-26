#pragma once
#include "rules/resolve.hpp"
#include <limits>
#include <string>

namespace blobber::rules {
struct RaceId {
    std::uint16_t index = UINT16_MAX;
};
struct ClassId {
    std::uint16_t index = UINT16_MAX;
};
struct ItemId {
    std::uint16_t index = UINT16_MAX;
};
struct OwnedItemId {
    std::uint16_t index = UINT16_MAX;
};
struct CharacterId {
    std::uint16_t index = UINT16_MAX;
};
constexpr std::size_t partySize = 4, maxStartingItems = 16;
// Supported weapon identities also describe racial/class training for future loadouts.
enum class WeaponType : std::uint8_t {
    Dagger,
    Mace,
    Longsword,
    Shortsword,
    Shortbow,
    Longbow,
    Battleaxe,
    Handaxe,
    LightHammer,
    Warhammer,
    Rapier,
    HandCrossbow,
    Dart,
    Sling,
    Quarterstaff,
    LightCrossbow,
    Count
};
constexpr std::size_t weaponTypeCount = static_cast<std::size_t>(WeaponType::Count);
using WeaponTraining = std::uint32_t;
constexpr WeaponTraining weaponBit(WeaponType type) noexcept {
    return static_cast<unsigned>(type) < weaponTypeCount ? 1u << static_cast<unsigned>(type) : 0;
}
constexpr WeaponTraining allWeaponTraining = (1u << weaponTypeCount) - 1;
constexpr WeaponTraining simpleWeaponTraining =
    weaponBit(WeaponType::Dagger) | weaponBit(WeaponType::Mace) | weaponBit(WeaponType::Shortbow) |
    weaponBit(WeaponType::Handaxe) | weaponBit(WeaponType::LightHammer) | weaponBit(WeaponType::Dart) |
    weaponBit(WeaponType::Sling) | weaponBit(WeaponType::Quarterstaff) | weaponBit(WeaponType::LightCrossbow);
enum class ArmorType : std::uint8_t { Light, Medium, Heavy, Shield };
constexpr std::uint8_t armorBit(ArmorType type) noexcept {
    return static_cast<unsigned>(type) <= 3 ? static_cast<std::uint8_t>(1u << static_cast<unsigned>(type))
                                            : 0;
}
enum class ItemKind : std::uint8_t { Weapon, Armor, Shield, Ammunition, Gear };
struct RaceDefinition {
    std::string id, name;
    Abilities abilityIncreases{};
    int hitPointsPerLevel = 0;
    WeaponTraining weapons = 0;
};
struct ClassDefinition {
    std::string id, name;
    int hitDie = 6;
    std::array<bool, abilityCount> saves{};
    WeaponTraining weapons = 0;
    std::uint8_t armor = 0;
    bool allowsDefense = false;
};
struct ItemDefinition {
    std::string id, name, description;
    ItemKind kind = ItemKind::Gear;
    std::int32_t weight = 0; // Hundredths of a pound, per unit.
    WeaponType weapon = WeaponType::Dagger;
    Dice damage{}; // Weapons: modifier must be zero; ability is derived.
    DamageType damageType = DamageType::Piercing;
    bool ranged = false, finesse = false, twoHanded = false;
    int versatileSides = 0; // Zero means not versatile.
    ArmorType armor = ArmorType::Light;
    int baseArmorClass = 0;
};
struct OwnedItem {
    ItemId item;
    std::uint16_t quantity = 0;
};
struct Equipment {
    OwnedItemId body, mainHand, offHand;
    OwnedItemId ranged{}; // Readied alternate weapon, not an additional hand.
};
struct CharacterBuild {
    std::string id, name;
    RaceId race;
    ClassId characterClass;
    Abilities baseAbilities{};
    int level = 1;
    bool defense = false;
    std::array<OwnedItem, maxStartingItems> items{};
    std::size_t itemCount = 0;
    Equipment equipment;
};
struct PresetParty {
    std::array<CharacterBuild, partySize> characters;
    std::array<CharacterId, partySize> formation{{{0}, {1}, {2}, {3}}};
};
struct WeaponPreview {
    bool available = false;
    bool proficient = false;
    bool ranged = false;
    Ability ability = Ability::Strength;
    int attackBonus = 0;
    Dice damage{};
    DamageType damageType = DamageType::Piercing;
};
struct CharacterStats {
    Abilities abilities{}, modifiers{}, saves{};
    int proficiency = 0, initiative = 0, maxHitPoints = 0, armorClass = 0;
    std::int64_t carriedWeight = 0, carryingCapacity = 0;
    std::array<WeaponPreview, maxStartingItems> weapons{}; // Indexed by owned entry.
};
enum class CharacterError {
    None,
    InvalidIdentity,
    InvalidReference,
    UnsupportedLevel,
    InvalidAbilities,
    InvalidFeature,
    InvalidItems,
    InvalidEquipment,
    ArmorProficiency,
    Overweight
};
const char* characterErrorMessage(CharacterError error) noexcept;
struct CharacterResult {
    CharacterStats value{};
    CharacterError error = CharacterError::None;
    explicit operator bool() const noexcept { return error == CharacterError::None; }
};
class Catalog;
CharacterResult deriveCharacter(const Catalog& catalog, const CharacterBuild& build) noexcept;
bool validFormation(const std::array<CharacterId, partySize>& formation) noexcept;
bool validDisplayText(const std::string& text, std::size_t maximum) noexcept;
} // namespace blobber::rules

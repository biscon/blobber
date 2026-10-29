#pragma once
#include "rules/resolve.hpp"
#include "rules/spells.hpp"
#include <limits>
#include <string>
#include <string_view>

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
    Scimitar,
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
enum class ItemKind : std::uint8_t { Weapon, Armor, Shield, Ammunition, Gear, Consumable, Currency, Supply };
enum class Ammunition : std::uint8_t { None, Arrow };
enum class Consumable : std::uint8_t { None, Healing, Mana };
struct PortraitId {
    std::uint8_t index = UINT8_MAX;
};
struct SpellGrants {
    std::uint32_t cantrips = 0, learned = 0, prepared = 0, domain = 0, racial = 0;
};
struct RaceDefinition {
    std::string id, name;
    Abilities abilityIncreases{};
    int hitPointsPerLevel = 0;
    WeaponTraining weapons = 0;
    bool sleepImmune = false;
    bool wizardCantrip = false;
};
struct ClassDefinition {
    std::string id, name;
    int hitDie = 6;
    std::array<bool, abilityCount> saves{};
    WeaponTraining weapons = 0;
    std::uint8_t armor = 0;
    bool allowsDefense = false;
    Casting casting = Casting::None;
    ClassFeatures features = 0;
    std::array<SpellId, 16> spells{};
    std::size_t spellCount = 0;
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
    std::uint16_t stackMaximum = 0; // Zero selects legacy kind default.
    Ammunition ammunition = Ammunition::None;
    Consumable consumable = Consumable::None;
    bool metalArmor = false;
};
unsigned stackLimit(const ItemDefinition&) noexcept;
Ammunition ammunitionFor(const ItemDefinition&) noexcept;
struct OwnedItem {
    ItemId item;
    std::uint16_t quantity = 0;
};
struct Equipment {
    OwnedItemId body, mainHand, offHand;
    OwnedItemId ranged{}; // Readied alternate weapon, not an additional hand.
};
struct StartingGrant {
    ItemId item;
    std::uint16_t quantity = 1;
    std::uint8_t slot = 255;
};
struct GearOption {
    std::string id, name;
    std::array<StartingGrant, 2> items{};
    std::size_t count = 0;
};
struct GearGroup {
    std::array<GearOption, 2> options{};
    std::size_t count = 0;
};
struct CreationClass {
    ClassId characterClass;
    Abilities recommended{};
    std::array<GearGroup, 3> groups{};
    std::size_t groupCount = 0;
    std::array<StartingGrant, 6> supplies{};
    std::size_t supplyCount = 0;
};

struct CharacterIdentityView {
    std::string_view id, name;
    RaceId race;
    ClassId characterClass;
    Abilities baseAbilities{};
    int level = 1;
    bool defense = false, lifeDomain = false;
};
struct CharacterIdentity {
    std::string id, name;
    RaceId race;
    ClassId characterClass;
    Abilities baseAbilities{};
    int level = 1;
    bool defense = false;
    bool lifeDomain = false;
    PortraitId portrait;
    SpellGrants spells;
    CharacterIdentityView view() const noexcept {
        return {id, name, race, characterClass, baseAbilities, level, defense, lifeDomain};
    }
};
struct CharacterBuild : CharacterIdentity {
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
constexpr std::size_t maxCarriedItems = 52;
struct CharacterLoadout {
    std::array<OwnedItem, maxCarriedItems> items{};
    std::size_t itemCount = maxCarriedItems;
    Equipment equipment;
};
struct CharacterStats {
    Abilities abilities{}, modifiers{}, saves{};
    int proficiency = 0, initiative = 0, maxHitPoints = 0, armorClass = 0;
    std::int64_t carriedWeight = 0, carryingCapacity = 0;
    std::array<WeaponPreview, maxCarriedItems> weapons{}; // Indexed by owned entry.
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
CharacterResult deriveCharacter(const Catalog&, const CharacterIdentity&, const CharacterLoadout&) noexcept;
bool validFormation(const std::array<CharacterId, partySize>& formation) noexcept;
bool validDisplayText(std::string_view text, std::size_t maximum) noexcept;
CharacterResult deriveCharacter(const Catalog&, const CharacterIdentityView&,
                                const CharacterLoadout&) noexcept;
} // namespace blobber::rules

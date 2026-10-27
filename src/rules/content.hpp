#pragma once
#include "rules/character.hpp"
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace blobber::rules {
struct AttackId {
    std::uint16_t index = std::numeric_limits<std::uint16_t>::max();
};
struct MonsterId {
    std::uint16_t index = std::numeric_limits<std::uint16_t>::max();
};
enum class AttackKind : std::uint8_t { Melee, Ranged };
struct AttackDefinition {
    std::string id;
    AttackKind kind = AttackKind::Melee;
    std::int32_t bonus = 0;
    Dice damage;
    DamageType damageType = DamageType::Bludgeoning;
};
enum class Trait : std::uint8_t { NimbleEscape = 1 };
using Traits = std::uint8_t;
inline bool hasTrait(Traits traits, Trait trait) noexcept {
    return (traits & static_cast<Traits>(trait)) != 0;
}
struct MonsterDefinition {
    std::string id;
    Abilities abilities{};
    int armorClass = 0;
    std::int32_t hitPoints = 0;
    std::array<AttackId, 8> attacks{};
    std::size_t attackCount = 0;
    Traits traits = 0;
};
// Indices never recycle, and are local to this immutable catalog's lifetime.
class Catalog {
  public:
    static constexpr std::size_t maxMonsters = 32, maxAttacks = 64, maxIdBytes = 128;
    static constexpr std::size_t maxRaces = 16, maxClasses = 16, maxItems = 64, maxSpells = 32;
    Catalog(std::vector<AttackDefinition> attacks, std::vector<MonsterDefinition> monsters,
            std::vector<RaceDefinition> races = {}, std::vector<ClassDefinition> classes = {},
            std::vector<ItemDefinition> items = {}, std::optional<PresetParty> preset = std::nullopt,
            std::vector<SpellDefinition> spells = {});
    const SpellDefinition* spell(SpellId) const noexcept;
    SpellId findSpell(std::string_view) const noexcept;
    std::size_t spellCount() const noexcept { return spells_.size(); }
    const RaceDefinition* race(RaceId id) const noexcept;
    const ClassDefinition* characterClass(ClassId id) const noexcept;
    const ItemDefinition* item(ItemId id) const noexcept;
    RaceId findRace(std::string_view id) const noexcept;
    ClassId findClass(std::string_view id) const noexcept;
    ItemId findItem(std::string_view id) const noexcept;
    const PresetParty* preset() const noexcept { return preset_ ? &*preset_ : nullptr; }
    std::size_t raceCount() const noexcept { return races_.size(); }
    std::size_t classCount() const noexcept { return classes_.size(); }
    std::size_t itemCount() const noexcept { return items_.size(); }
    const AttackDefinition* attack(AttackId id) const noexcept;
    const MonsterDefinition* monster(MonsterId id) const noexcept;
    AttackId findAttack(std::string_view id) const noexcept;
    MonsterId findMonster(std::string_view id) const noexcept;
    std::size_t attackCount() const noexcept { return attacks_.size(); }
    std::size_t monsterCount() const noexcept { return monsters_.size(); }

  private:
    std::vector<SpellDefinition> spells_;
    std::vector<AttackDefinition> attacks_;
    std::vector<MonsterDefinition> monsters_;
    std::vector<RaceDefinition> races_;
    std::vector<ClassDefinition> classes_;
    std::vector<ItemDefinition> items_;
    std::optional<PresetParty> preset_;
};
bool validContentId(std::string_view id) noexcept;
} // namespace blobber::rules

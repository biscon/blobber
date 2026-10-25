#pragma once
#include "rules/resolve.hpp"
#include <limits>
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
struct MonsterDefinition {
    std::string id;
    Abilities abilities{};
    int armorClass = 0;
    std::int32_t hitPoints = 0;
    std::array<AttackId, 8> attacks{};
    std::size_t attackCount = 0;
};
// Indices never recycle, and are local to this immutable catalog's lifetime.
class Catalog {
  public:
    static constexpr std::size_t maxMonsters = 32, maxAttacks = 64, maxIdBytes = 128;
    Catalog(std::vector<AttackDefinition> attacks, std::vector<MonsterDefinition> monsters);
    const AttackDefinition* attack(AttackId id) const noexcept;
    const MonsterDefinition* monster(MonsterId id) const noexcept;
    AttackId findAttack(std::string_view id) const noexcept;
    MonsterId findMonster(std::string_view id) const noexcept;
    std::size_t attackCount() const noexcept { return attacks_.size(); }
    std::size_t monsterCount() const noexcept { return monsters_.size(); }

  private:
    std::vector<AttackDefinition> attacks_;
    std::vector<MonsterDefinition> monsters_;
};
bool validContentId(std::string_view id) noexcept;
} // namespace blobber::rules

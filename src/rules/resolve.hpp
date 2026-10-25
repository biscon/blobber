#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <random>

namespace blobber::rules {
enum class Error { None, InvalidAbility, InvalidScore, UnsupportedLevel, InvalidDice, InvalidRoll };
template <class T> struct Result {
    T value{};
    Error error = Error::None;
    explicit operator bool() const noexcept { return error == Error::None; }
};
enum class Ability : std::uint8_t { Strength, Dexterity, Constitution, Intelligence, Wisdom, Charisma };
constexpr std::size_t abilityCount = 6;
using Abilities = std::array<int, abilityCount>;
Result<int> abilityModifier(int score) noexcept;
Result<int> abilityModifier(const Abilities& scores, Ability ability) noexcept;
Result<int> proficiencyBonus(int level) noexcept;

struct Dice {
    int count = 1;
    int sides = 6;
    std::int32_t modifier = 0;
};
bool validDice(Dice dice) noexcept;
enum class DamageType : std::uint8_t { Bludgeoning, Piercing, Slashing };
struct DiceRoll {
    std::array<std::uint8_t, 64> faces{};
    std::size_t count = 0;
    std::int64_t total = 0;
};
enum class RollMode : std::uint8_t { Normal, Advantage, Disadvantage };
constexpr RollMode rollMode(bool advantage, bool disadvantage) noexcept {
    return advantage == disadvantage ? RollMode::Normal
           : advantage               ? RollMode::Advantage
                                     : RollMode::Disadvantage;
}
struct D20Roll {
    RollMode mode = RollMode::Normal;
    int first = 0, second = 0, selected = 0;
};
// Caller-owned, explicitly seeded. No clock, global state, or standard distribution.
class DiceRng {
  public:
    explicit DiceRng(std::uint32_t seed) : engine_(seed) {}
    Result<DiceRoll> roll(Dice dice) noexcept;
    Result<DiceRoll> damage(Dice dice, bool critical) noexcept;
    Result<D20Roll> d20(RollMode mode = RollMode::Normal) noexcept;

  private:
    std::uint32_t face(std::uint32_t sides) noexcept;
    DiceRoll rollValidated(Dice dice, bool critical) noexcept;
    std::mt19937 engine_;
};
enum class AttackOutcome { Miss, Hit, Critical };
struct AttackResult {
    D20Roll roll;
    std::int64_t total = 0;
    AttackOutcome outcome = AttackOutcome::Miss;
};
struct SaveResult {
    D20Roll roll;
    std::int64_t total = 0;
    bool succeeded = false;
};
Result<AttackResult> resolveAttack(D20Roll roll, std::int32_t bonus, std::int32_t armorClass) noexcept;
Result<SaveResult> resolveSave(D20Roll roll, std::int32_t bonus, std::int32_t difficulty) noexcept;
} // namespace blobber::rules

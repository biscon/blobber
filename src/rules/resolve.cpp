#include "rules/resolve.hpp"
#include <algorithm>

namespace blobber::rules {
namespace {
bool validMode(RollMode mode) noexcept {
    return mode == RollMode::Normal || mode == RollMode::Advantage || mode == RollMode::Disadvantage;
}
bool validRoll(D20Roll roll) noexcept {
    if (!validMode(roll.mode) || roll.first < 1 || roll.first > 20)
        return false;
    if (roll.mode == RollMode::Normal)
        return roll.second == 0 && roll.selected == roll.first;
    if (roll.second < 1 || roll.second > 20)
        return false;
    return roll.selected == (roll.mode == RollMode::Advantage ? std::max(roll.first, roll.second)
                                                              : std::min(roll.first, roll.second));
}
} // namespace
Result<int> abilityModifier(int score) noexcept {
    if (score < 1 || score > 30)
        return {{}, Error::InvalidScore};
    // Scores are positive, so integer division implements floor even below 10.
    return {score / 2 - 5};
}
Result<int> abilityModifier(const Abilities& scores, Ability ability) noexcept {
    const auto index = static_cast<std::size_t>(ability);
    if (index >= scores.size())
        return {{}, Error::InvalidAbility};
    return abilityModifier(scores[index]);
}
Result<int> proficiencyBonus(int level) noexcept {
    if (level < 1 || level > 3)
        return {{}, Error::UnsupportedLevel};
    return {2};
}
bool validDice(Dice dice) noexcept {
    if (dice.count < 1 || dice.count > 32)
        return false;
    switch (dice.sides) {
    case 4:
    case 6:
    case 8:
    case 10:
    case 12:
    case 20:
    case 100:
        return true;
    default:
        return false;
    }
}
std::uint32_t DiceRng::face(std::uint32_t sides) noexcept {
    const auto threshold = static_cast<std::uint32_t>(0u - sides) % sides;
    std::uint32_t word;
    do {
        word = static_cast<std::uint32_t>(engine_());
    } while (word < threshold);
    return word % sides + 1;
}
DiceRoll DiceRng::rollValidated(Dice dice, bool critical) noexcept {
    DiceRoll result;
    result.count = static_cast<std::size_t>(dice.count) * (critical ? 2 : 1);
    result.total = dice.modifier;
    for (std::size_t i = 0; i < result.count; ++i) {
        result.faces[i] = static_cast<std::uint8_t>(face(static_cast<std::uint32_t>(dice.sides)));
        result.total += result.faces[i];
    }
    return result;
}
Result<DiceRoll> DiceRng::roll(Dice dice) noexcept {
    if (!validDice(dice))
        return {{}, Error::InvalidDice};
    return {rollValidated(dice, false)};
}
Result<DiceRoll> DiceRng::damage(Dice dice, bool critical) noexcept {
    if (!validDice(dice))
        return {{}, Error::InvalidDice};
    auto result = rollValidated(dice, critical);
    result.total = std::max<std::int64_t>(0, result.total);
    return {result};
}
Result<D20Roll> DiceRng::d20(RollMode mode) noexcept {
    if (!validMode(mode))
        return {{}, Error::InvalidRoll};
    D20Roll result{mode, static_cast<int>(face(20)), 0, 0};
    result.selected = result.first;
    if (mode != RollMode::Normal) {
        result.second = static_cast<int>(face(20));
        result.selected = mode == RollMode::Advantage ? std::max(result.first, result.second)
                                                      : std::min(result.first, result.second);
    }
    return {result};
}
Result<AttackResult> resolveAttack(D20Roll roll, std::int32_t bonus, std::int32_t armorClass) noexcept {
    if (!validRoll(roll))
        return {{}, Error::InvalidRoll};
    AttackResult result{roll, static_cast<std::int64_t>(roll.selected) + bonus, AttackOutcome::Miss};
    if (roll.selected == 20)
        result.outcome = AttackOutcome::Critical;
    else if (roll.selected != 1 && result.total >= armorClass)
        result.outcome = AttackOutcome::Hit;
    return {result};
}
Result<SaveResult> resolveSave(D20Roll roll, std::int32_t bonus, std::int32_t difficulty) noexcept {
    if (!validRoll(roll))
        return {{}, Error::InvalidRoll};
    const auto total = static_cast<std::int64_t>(roll.selected) + bonus;
    return {{roll, total, total >= difficulty}};
}
} // namespace blobber::rules

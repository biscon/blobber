#include "rules/content.hpp"
#include "support/check.hpp"
#include <iostream>
#include <limits>

using namespace blobber::rules;
namespace {
D20Roll natural(int face) {
    return {RollMode::Normal, face, 0, face};
}
void abilities() {
    const int expected[]{-5, -4, -4, -3, -3, -2, -2, -1, -1, 0, 0, 1, 1, 2, 2,
                         3,  3,  4,  4,  5,  5,  6,  6,  7,  7, 8, 8, 9, 9, 10};
    for (int score = 1; score <= 30; ++score)
        check(abilityModifier(score).value == expected[score - 1],
              "Ability modifier including negative rounding");
    for (int score : {std::numeric_limits<int>::min(), 0, 31, std::numeric_limits<int>::max()})
        check(abilityModifier(score).error == Error::InvalidScore, "Invalid ability score");
    const Abilities scores{8, 14, 10, 12, 15, 9};
    check(abilityModifier(scores, Ability::Dexterity).value == 2, "Typed ability selects score");
    check(abilityModifier(scores, static_cast<Ability>(255)).error == Error::InvalidAbility,
          "Invalid ability enum");
    for (int level = 1; level <= 3; ++level)
        check(proficiencyBonus(level).value == 2, "Supported proficiency");
    check(!proficiencyBonus(0) && !proficiencyBonus(4), "Unsupported levels fail explicitly");
}
void randomness() {
    // Observable replay contract, independent of std::uniform_int_distribution.
    DiceRng golden(5489);
    for (const int expected : {13, 3, 15, 6, 5, 12, 10, 6, 19, 4})
        check(golden.d20().value.selected == expected, "Seed 5489 d20 replay sequence");
    DiceRng a(42), b(42), other(43);
    bool differs = false;
    for (int sides : {4, 6, 8, 10, 12, 20, 100}) {
        const auto x = a.roll({32, sides, -11});
        const auto y = b.roll({32, sides, -11});
        const auto z = other.roll({32, sides, -11});
        check(x && y && z, "All standard dice accepted");
        check(x.value.faces == y.value.faces && x.value.total == y.value.total,
              "Same seed replays mixed dice");
        differs |= x.value.faces != z.value.faces;
        std::int64_t sum = -11;
        for (std::size_t i = 0; i < x.value.count; ++i) {
            check(x.value.faces[i] >= 1 && x.value.faces[i] <= sides, "Each face remains in range");
            sum += x.value.faces[i];
        }
        check(x.value.count == 32 && sum == x.value.total, "Dice result retains faces and modifier");
    }
    check(differs, "Different seed changes sequence");
    DiceRng rejected(55), untouched(55);
    for (Dice dice : {Dice{0, 6, 0}, {-1, 6, 0}, {33, 6, 0}, {1, 0, 0}, {1, 3, 0}, {1, 101, 0}}) {
        check(rejected.roll(dice).error == Error::InvalidDice, "Invalid roll rejected");
        check(rejected.damage(dice, true).error == Error::InvalidDice, "Invalid critical damage rejected");
    }
    check(!rejected.d20(static_cast<RollMode>(255)), "Invalid roll mode rejected");
    for (int i = 0; i < 20; ++i)
        check(rejected.d20().value.first == untouched.d20().value.first, "Rejection never advances RNG");
    for (RollMode mode : {RollMode::Normal, RollMode::Advantage, RollMode::Disadvantage}) {
        DiceRng r(5489);
        const auto roll = r.d20(mode).value;
        check(roll.first == 13, "First d20 retained");
        check(roll.second == (mode == RollMode::Normal ? 0 : 3), "Extra d20 only when required");
        check(roll.selected == (mode == RollMode::Disadvantage ? 3 : 13), "Correct d20 selected");
        check(r.d20().value.first == (mode == RollMode::Normal ? 3 : 15), "Precisely one/two dice consumed");
    }
    check(rollMode(false, false) == RollMode::Normal && rollMode(true, true) == RollMode::Normal &&
              rollMode(true, false) == RollMode::Advantage && rollMode(false, true) == RollMode::Disadvantage,
          "Opposite advantage sources cancel without stacking");
}
void resolution() {
    check(resolveAttack(natural(10), 4, 15).value.outcome == AttackOutcome::Miss, "Below AC misses");
    const auto hit = resolveAttack(natural(11), 4, 15);
    check(hit && hit.value.outcome == AttackOutcome::Hit && hit.value.total == 15 &&
              hit.value.roll.first == 11,
          "Meeting AC hits with inspectable result");
    check(resolveAttack(natural(1), INT32_MAX, INT32_MIN).value.outcome == AttackOutcome::Miss,
          "Natural 1 misses regardless of bonus/AC");
    check(resolveAttack(natural(20), INT32_MIN, INT32_MAX).value.outcome == AttackOutcome::Critical,
          "Natural 20 critically hits regardless of bonus/AC");
    check(resolveSave(natural(1), 19, 20).value.succeeded,
          "Natural 1 is not an automatic ordinary save failure");
    check(!resolveSave(natural(20), 0, 21).value.succeeded,
          "Natural 20 is not an automatic ordinary save success");
    check(resolveSave(natural(10), 4, 14).value.succeeded && !resolveSave(natural(10), 4, 15).value.succeeded,
          "Save threshold inclusive");
    check(resolveSave(natural(20), INT32_MAX, INT32_MAX).value.total == std::int64_t(INT32_MAX) + 20,
          "Totals do not overflow int32");
    check(resolveAttack({RollMode::Disadvantage, 20, 2, 2}, 4, 15).value.outcome == AttackOutcome::Miss,
          "Unselected natural 20 does not critically hit");
    check(resolveAttack({RollMode::Advantage, 1, 11, 11}, 4, 15).value.outcome == AttackOutcome::Hit,
          "Unselected natural 1 does not miss");
    for (D20Roll bad : {natural(0),
                        natural(21),
                        D20Roll{RollMode::Normal, 10, 1, 10},
                        {RollMode::Normal, 10, 0, 11},
                        {RollMode::Advantage, 1, 20, 1},
                        {RollMode::Disadvantage, 1, 20, 20},
                        {RollMode::Advantage, 10, 0, 10},
                        {static_cast<RollMode>(255), 10, 0, 10}})
        check(!resolveAttack(bad, 4, 15) && !resolveSave(bad, 4, 15),
              "Malformed pure-resolution inputs rejected");
}
void damage() {
    DiceRng critical(42), ordinary(42);
    const auto crit = critical.damage({2, 6, 3}, true);
    const auto dice = ordinary.roll({4, 6, 0});
    check(crit && crit.value.count == 4 && crit.value.faces == dice.value.faces &&
              crit.value.total == dice.value.total + 3,
          "Critical doubles all dice, not the modifier");
    check(critical.damage({1, 4, -100}, false).value.total == 0, "Negative damage clamps to zero");
    check(critical.roll({1, 4, -100}).value.total < 0, "Ordinary dice totals remain signed");
    const auto maximum = critical.damage({32, 100, INT32_MAX}, true);
    check(maximum && maximum.value.count == 64 && maximum.value.total > INT32_MAX,
          "Maximum critical fits fixed storage and wide total");
    check(critical.damage({32, 100, INT32_MIN}, true).value.total == 0, "Extreme negative modifier safe");
}
void catalog() {
    AttackDefinition attack{"test", AttackKind::Melee, 2, {1, 4, -1}, DamageType::Bludgeoning};
    MonsterDefinition monster;
    monster.id = "fixture";
    monster.abilities.fill(10);
    monster.armorClass = 12;
    monster.hitPoints = 5;
    monster.attackCount = 1;
    monster.attacks[0] = {0};
    const Catalog valid({attack}, {monster});
    check(valid.monster(valid.findMonster("fixture"))->attacks[0].index == valid.findAttack("test").index,
          "String IDs resolve to typed references");
    check(!valid.attack({}) && !valid.monster({}) && !valid.attack({1}) && !valid.monster({1}) &&
              !valid.attack(valid.findAttack("missing")) && !valid.monster(valid.findMonster("missing")),
          "Invalid/default/unknown handles return null");
    for (int fault = 0; fault < 8; ++fault) {
        auto a = attack;
        auto m = monster;
        switch (fault) {
        case 0:
            a.kind = static_cast<AttackKind>(255);
            break;
        case 1:
            a.damageType = static_cast<DamageType>(255);
            break;
        case 2:
            a.damage.count = 33;
            break;
        case 3:
            m.attackCount = 9;
            break;
        case 4:
            m.attacks[0] = {};
            break;
        case 5:
            m.abilities[0] = 0;
            break;
        case 6:
            m.hitPoints = 0;
            break;
        case 7:
            m.armorClass = 31;
            break;
        }
        rejects([&] { Catalog c({a}, {m}); }, "In-memory construction cannot bypass validation");
    }
}
} // namespace
int main() {
    try {
        abilities();
        randomness();
        resolution();
        damage();
        catalog();
        std::cout << "Rules checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

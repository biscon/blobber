#pragma once
#include "rules/resolve.hpp"
#include <string>
namespace blobber::rules {
struct SpellId {
    std::uint16_t index = UINT16_MAX;
};
enum class SpellKind : std::uint8_t {
    SacredFlame,
    SpareTheDying,
    Bless,
    Bane,
    ShieldOfFaith,
    CureWounds,
    GuidingBolt,
    InflictWounds,
    FireBolt,
    AcidSplash,
    MageArmor,
    MagicMissile,
    BurningHands,
    Thunderwave,
    RayOfFrost,
    ShockingGrasp,
    PoisonSpray,
    Sleep,
    FalseLife,
    Grease,
    ColorSpray,
    Guidance,
    Resistance,
    Sanctuary,
    Aid,
    HoldPerson,
    LesserRestoration,
    SpiritualWeapon,
    ScorchingRay,
    Shatter,
    MirrorImage,
    Web,
    Count
};
enum class Casting : std::uint8_t { None, Wisdom, Intelligence };
enum class Feature : std::uint8_t {
    None,
    SecondWind,
    ArcaneRecovery,
    DismissConcentration,
    ActionSurge,
    PreserveLife,
    CunningAction,
    SpiritualWeapon,
    EscapeWeb,
    EndTurn,
    DismissSpiritualWeapon
};
enum class ClassFeature : std::uint8_t {
    SecondWind = 1,
    LifeDomain = 2,
    SneakAttack = 4,
    ArcaneRecovery = 8
};
using ClassFeatures = std::uint8_t;
inline bool hasFeature(ClassFeatures f, ClassFeature bit) noexcept {
    return (f & static_cast<ClassFeatures>(bit)) != 0;
}
struct SpellDefinition {
    std::string id, name, description;
    SpellKind kind = SpellKind::Count;
};
struct SpellRules {
    const char *id, *name, *description;
    Casting casting;
    int level;
    Dice dice;
    DamageType damage;
    Ability save;
    bool attack, hostile, area;
    unsigned maxTargets;
    bool ground = false, self = false;
};
const char* featureName(Feature) noexcept;
const SpellRules* spellRules(SpellKind) noexcept;
int maximumMana(Casting, int level) noexcept;
int spellManaCost(int spellLevel) noexcept;
bool spellClass(SpellKind, Casting) noexcept;
int maximumSpellLevel(int characterLevel) noexcept;
} // namespace blobber::rules

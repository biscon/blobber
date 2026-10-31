#pragma once
#include "rules/resolve.hpp"
#include <algorithm>
#include <limits>
#include <optional>
#include <string_view>

namespace blobber::rules {
inline constexpr unsigned damageTypeCount = unsigned(DamageType::Poison) + 1;
using DamageMask = std::uint16_t;
inline constexpr DamageMask allDamageTypes = (1u << damageTypeCount) - 1u;
constexpr DamageMask damageBit(DamageType type) noexcept {
    return unsigned(type) < damageTypeCount ? DamageMask(1u << unsigned(type)) : DamageMask{0};
}
struct DamageProfile {
    DamageMask immunities = 0, vulnerabilities = 0;
};
constexpr bool validDamageProfile(DamageProfile p) noexcept {
    return !((p.immunities | p.vulnerabilities) & ~allDamageTypes) && !(p.immunities & p.vulnerabilities);
}
enum class DamageAdjustment : std::uint8_t { Normal, Immune, Vulnerable };
struct DamageResolution {
    std::int64_t raw = 0, damage = 0;
    DamageAdjustment adjustment = DamageAdjustment::Normal;
};
inline DamageResolution resolveDamage(std::int64_t amount, DamageType type,
                                      DamageProfile profile = {}) noexcept {
    const auto raw = std::max<std::int64_t>(0, amount);
    if (profile.immunities & damageBit(type))
        return {raw, 0, DamageAdjustment::Immune};
    if (profile.vulnerabilities & damageBit(type))
        return {raw, raw > INT64_MAX / 2 ? INT64_MAX : raw * 2, DamageAdjustment::Vulnerable};
    return {raw, raw, DamageAdjustment::Normal};
}
inline const char* damageTypeName(DamageType type) noexcept {
    constexpr const char* names[]{"bludgeoning", "piercing", "slashing", "acid", "fire",      "force",
                                  "radiant",     "necrotic", "thunder",  "cold", "lightning", "poison"};
    return unsigned(type) < damageTypeCount ? names[unsigned(type)] : "unknown";
}
inline std::optional<DamageType> findDamageType(std::string_view name) noexcept {
    for (unsigned i = 0; i < damageTypeCount; ++i)
        if (name == damageTypeName(DamageType(i)))
            return DamageType(i);
    return {};
}
inline const char* damageAdjustmentName(DamageAdjustment value) noexcept {
    switch (value) {
    case DamageAdjustment::Normal:
        return "normal";
    case DamageAdjustment::Immune:
        return "immune";
    case DamageAdjustment::Vulnerable:
        return "vulnerable";
    }
    return "unknown";
}
} // namespace blobber::rules

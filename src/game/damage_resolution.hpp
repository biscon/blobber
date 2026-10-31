#pragma once
#include "game/actors.hpp"

namespace blobber {
class EffectController;
enum class FortitudeStatus : std::uint8_t { None, Saved, Failed, RadiantBypass, CriticalBypass };
struct FortitudeResult {
    FortitudeStatus status = FortitudeStatus::None;
    int roll = 0;
    std::int64_t total = 0, difficulty = 0;
};
inline const char* fortitudeStatusName(FortitudeStatus status) noexcept {
    switch (status) {
    case FortitudeStatus::None:
        return "none";
    case FortitudeStatus::Saved:
        return "saved";
    case FortitudeStatus::Failed:
        return "failed";
    case FortitudeStatus::RadiantBypass:
        return "radiant_bypass";
    case FortitudeStatus::CriticalBypass:
        return "critical_bypass";
    }
    return "unknown";
}
struct NpcDamageResult {
    ActorResult actor;
    rules::DamageResolution resolved;
    FortitudeResult fortitude;
};
// One checked mitigation/save/HP commit for every gameplay NPC damage source.
// Callers retain source-specific wake, interruption and effect reconciliation.
NpcDamageResult damageNpc(ActorWorld&, EffectController&, ActorId, std::int64_t amount, rules::DamageType,
                          bool critical, std::uint64_t when) noexcept;
} // namespace blobber

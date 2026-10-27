#pragma once
#include <cstdint>

namespace blobber::rules {
enum class Life : std::uint8_t { Conscious, Dying, Stable, Dead };
struct HealthState {
    int hitPoints = 0;
    Life life = Life::Conscious;
    std::uint8_t successes = 0, failures = 0;
};
enum class HealthError { None, InvalidState, InvalidAmount, InvalidRoll, NotEligible, InvalidCharacter };
struct HealthResult {
    HealthState state;
    HealthError error = HealthError::None;
    bool changed = false, died = false;
    explicit operator bool() const noexcept { return error == HealthError::None; }
};
bool validHealth(const HealthState& health, int maximum) noexcept;
HealthResult takeDamage(const HealthState& health, int maximum, std::int64_t amount,
                        bool critical = false) noexcept;
HealthResult receiveHealing(const HealthState& health, int maximum, std::int64_t amount) noexcept;
HealthResult stabilize(const HealthState& health, int maximum) noexcept;
HealthResult deathSave(const HealthState& health, int maximum, int face, int modifier = 0) noexcept;
} // namespace blobber::rules

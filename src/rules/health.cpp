#include "rules/health.hpp"
#include <algorithm>
namespace blobber::rules {
bool validHealth(const HealthState& h, int maximum) noexcept {
    if (maximum <= 0 || h.hitPoints < 0 || h.hitPoints > maximum || h.successes > 2 || h.failures > 3)
        return false;
    switch (h.life) {
    case Life::Conscious:
        return h.hitPoints > 0 && !h.successes && !h.failures;
    case Life::Dying:
        return h.hitPoints == 0 && h.failures < 3;
    case Life::Stable:
        return h.hitPoints == 0 && !h.successes && !h.failures;
    case Life::Dead:
        return h.hitPoints == 0;
    }
    return false;
}
namespace {
HealthResult invalid(const HealthState& h, int maximum) noexcept {
    return {h, validHealth(h, maximum) ? HealthError::None : HealthError::InvalidState};
}
void kill(HealthResult& r) noexcept {
    r.state.hitPoints = 0;
    r.state.life = Life::Dead;
    r.changed = r.died = true;
}
} // namespace
HealthResult takeDamage(const HealthState& h, int maximum, std::int64_t amount, bool critical) noexcept {
    auto r = invalid(h, maximum);
    if (!r)
        return r;
    if (amount < 0)
        return {h, HealthError::InvalidAmount};
    if (h.life == Life::Dead)
        return {h, HealthError::NotEligible};
    if (!amount)
        return r;
    r.changed = true;
    if (amount >= std::int64_t(h.hitPoints) + maximum) {
        kill(r);
        return r;
    }
    if (h.hitPoints == 0) {
        r.state.life = Life::Dying;
        r.state.failures = static_cast<std::uint8_t>(std::min(3, int(h.failures) + (critical ? 2 : 1)));
        if (r.state.failures == 3)
            kill(r);
    } else {
        r.state.hitPoints = static_cast<int>(std::max(std::int64_t{0}, std::int64_t(h.hitPoints) - amount));
        if (!r.state.hitPoints)
            r.state.life = Life::Dying;
    }
    return r;
}
HealthResult receiveHealing(const HealthState& h, int maximum, std::int64_t amount) noexcept {
    auto r = invalid(h, maximum);
    if (!r)
        return r;
    if (amount < 0)
        return {h, HealthError::InvalidAmount};
    if (h.life == Life::Dead)
        return {h, HealthError::NotEligible};
    if (!amount)
        return r;
    const int gain = static_cast<int>(std::min<std::int64_t>(amount, maximum - h.hitPoints));
    r.state = {h.hitPoints + gain, Life::Conscious, 0, 0};
    r.changed = gain != 0;
    return r;
}
HealthResult stabilize(const HealthState& h, int maximum) noexcept {
    auto r = invalid(h, maximum);
    if (!r)
        return r;
    if (h.life == Life::Stable)
        return r;
    if (h.life != Life::Dying)
        return {h, HealthError::NotEligible};
    r.state = {0, Life::Stable, 0, 0};
    r.changed = true;
    return r;
}
HealthResult deathSave(const HealthState& h, int maximum, int face, int modifier) noexcept {
    auto r = invalid(h, maximum);
    if (!r)
        return r;
    if (face < 1 || face > 20)
        return {h, HealthError::InvalidRoll};
    if (h.life != Life::Dying)
        return {h, HealthError::NotEligible};
    if (face == 20)
        return receiveHealing(h, maximum, 1);
    r.changed = true;
    if (face != 1 && std::int64_t(face) + modifier >= 10) {
        if (++r.state.successes == 3)
            r.state = {0, Life::Stable, 0, 0};
    } else {
        r.state.failures = static_cast<std::uint8_t>(std::min(3, int(h.failures) + (face == 1 ? 2 : 1)));
        if (r.state.failures == 3)
            kill(r);
    }
    return r;
}
} // namespace blobber::rules

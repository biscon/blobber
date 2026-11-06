#include "ui/screen_shake.hpp"
#include "game/combat.hpp"
#include <cmath>

namespace blobber {
namespace {
double smooth(double t) noexcept {
    return t * t * t * (t * (t * 6 - 15) + 10);
}
std::uint32_t hash(std::uint32_t v) noexcept {
    v ^= v >> 16;
    v *= 0x7feb352du;
    v ^= v >> 15;
    v *= 0x846ca68bu;
    return v ^ (v >> 16);
}
// Adapted from ../engine's ScreenShake: deterministic, bounded gradient noise.
float noise(double seconds, double frequency, std::uint32_t seed) noexcept {
    const double x = std::fmod(seconds, 65536.0 / frequency) * frequency + double(seed & 0xffffu) / 65536.0;
    const auto cell = static_cast<std::uint32_t>(std::floor(x));
    const double t = x - std::floor(x);
    const double a = (hash((cell & 0xffffu) ^ seed) & 1u) ? t : -t;
    const double b = (hash(((cell + 1u) & 0xffffu) ^ seed) & 1u) ? t - 1 : 1 - t;
    return float(2 * (a + (b - a) * smooth(t)));
}
bool active(std::uint64_t now, std::uint64_t start, std::uint32_t duration) noexcept {
    return duration && now >= start && now - start < duration;
}
float envelope(std::uint64_t age, std::uint32_t duration) noexcept {
    const double attack = std::min(10.0, duration * .1);
    if (age < attack)
        return float(smooth(age / attack));
    const double remaining = (duration - age) / (duration - attack);
    return float(remaining * remaining);
}
} // namespace
std::optional<ShakeSize> shakeForImpact(const CombatOutcome& out) noexcept {
    if (out.reason != CommandReason::Resolved)
        return {};
    if (out.event == CombatEvent::Cast &&
        (out.spell == rules::SpellKind::BurningHands || out.spell == rules::SpellKind::Thunderwave ||
         out.spell == rules::SpellKind::Shatter))
        return ShakeSize::Big;
    if (out.event != CombatEvent::Attack || !out.hit || out.targetGone || out.mirror.effect.serial ||
        out.damage <= 0)
        return {};
    if (out.critical)
        return ShakeSize::Medium;
    if (out.enemyAttack && out.kind == rules::AttackKind::Melee && out.spell == rules::SpellKind::Count)
        return ShakeSize::Little;
    return {};
}
void ScreenShakePlayer::reset(std::uint64_t world) noexcept {
    impulses_ = {};
    world_ = world;
    nextSeed_ = 0;
}
bool ScreenShakePlayer::record(const CombatOutcome& out, std::uint64_t now, std::uint64_t world) noexcept {
    if (world != world_)
        reset(world);
    if (out.attackId.world && out.attackId.world != world)
        return false;
    const auto size = shakeForImpact(out);
    if (!size || !std::isfinite(tuning_.multiplier) || tuning_.multiplier <= 0 ||
        !std::isfinite(tuning_.frequencyHz) || tuning_.frequencyHz <= 0 ||
        !std::isfinite(tuning_.limitPixels) || tuning_.limitPixels <= 0)
        return false;
    const auto preset = tuning_.presets[std::size_t(*size)];
    if (!std::isfinite(preset.pixels) || preset.pixels <= 0 ||
        !active(now, out.occurredMs, preset.durationMs))
        return false;
    for (auto& impulse : impulses_)
        if (!active(now, impulse.started, impulse.preset.durationMs)) {
            impulse = {out.occurredMs, preset, hash(++nextSeed_)};
            return true;
        }
    return false; // Bounded cosmetic exhaustion; no allocation or frame-path logging.
}
WorldViewTransform ScreenShakePlayer::sample(std::uint64_t now, std::uint64_t world) const noexcept {
    if (world != world_)
        return {};
    double x = 0, y = 0, bound = 0;
    for (const auto& impulse : impulses_) {
        if (!active(now, impulse.started, impulse.preset.durationMs))
            continue;
        const auto age = now - impulse.started;
        const double amplitude =
            double(impulse.preset.pixels) * tuning_.multiplier * envelope(age, impulse.preset.durationMs);
        bound += amplitude;
        x += amplitude * noise(double(age) / 1000, tuning_.frequencyHz, hash(impulse.seed));
        y += amplitude * noise(double(age) / 1000, tuning_.frequencyHz, hash(impulse.seed + 1));
    }
    if (bound == 0)
        return {};
    const double limit = std::min(bound, double(tuning_.limitPixels));
    // The smaller viewport dimension bounds both axes; crop follows the envelope,
    // not individual noise samples, avoiding rapid zoom pulses.
    return {float(std::clamp(x, -limit, limit)), float(std::clamp(y, -limit, limit)),
            1.f + float(2 * limit / std::min(viewWidth, viewHeight))};
}
std::size_t ScreenShakePlayer::activeCount(std::uint64_t now) const noexcept {
    std::size_t count = 0;
    for (const auto& impulse : impulses_)
        count += active(now, impulse.started, impulse.preset.durationMs);
    return count;
}
} // namespace blobber

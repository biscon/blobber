#pragma once
#include "ui/world_view_transform.hpp"
#include <array>
#include <cstdint>

namespace blobber {
struct CombatOutcome;
enum class ShakeSize : std::uint8_t { Little, Medium, Big };
struct ShakePreset {
    float pixels;
    std::uint32_t durationMs;
};
struct ScreenShakeTuning {
    std::array<ShakePreset, 3> presets{{{1.f, 140}, {2.5f, 230}, {5.f, 360}}};
    float frequencyHz = 14.f;
    float multiplier = 1.f; // Zero disables all screen shake.
    float limitPixels = 7.f;
};
inline constexpr ScreenShakeTuning screenShakeTuning{};
std::optional<ShakeSize> shakeForImpact(const CombatOutcome&) noexcept;

// Consume events once; sampling is read-only and uses the paused session clock.
class ScreenShakePlayer {
  public:
    static constexpr std::size_t capacity = 16;
    explicit ScreenShakePlayer(ScreenShakeTuning tuning = screenShakeTuning) noexcept : tuning_(tuning) {}
    void reset(std::uint64_t world) noexcept;
    bool record(const CombatOutcome&, std::uint64_t now, std::uint64_t world) noexcept;
    WorldViewTransform sample(std::uint64_t now, std::uint64_t world) const noexcept;
    std::size_t activeCount(std::uint64_t now) const noexcept;

  private:
    struct Impulse {
        std::uint64_t started = 0;
        ShakePreset preset{};
        std::uint32_t seed = 0;
    };
    ScreenShakeTuning tuning_;
    std::array<Impulse, capacity> impulses_{};
    std::uint64_t world_ = 0;
    std::uint32_t nextSeed_ = 0;
};
} // namespace blobber

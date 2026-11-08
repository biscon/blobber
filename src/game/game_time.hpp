#pragma once
#include <array>
#include <cinttypes>
#include <cstdio>
#include <cstdint>

namespace blobber {
namespace gameDuration {
inline constexpr std::uint64_t second = 1000, minute = 60 * second, hour = 60 * minute,
                               day = 24 * hour, round = 6 * second,
                               shortRest = hour, longRest = 8 * hour;
}
struct CalendarTime {
    std::uint64_t day = 1;
    unsigned hour = 8, minute = 0;
};
// Elapsed game time, not presentation milliseconds or a location-local effect deadline.
struct GameTime {
    std::uint64_t elapsedMs = 0;
    CalendarTime calendar() const noexcept {
        const auto minutes = elapsedMs / gameDuration::minute;
        const auto local = minutes % (24 * 60) + 8 * 60;
        return {1 + minutes / (24 * 60) + local / (24 * 60),
                unsigned(local % (24 * 60) / 60), unsigned(local % 60)};
    }
    bool reached(GameTime deadline) const noexcept { return elapsedMs >= deadline.elapsedMs; }
    // Saturation is deliberate: deadlines never wrap into the past.
    GameTime after(std::uint64_t durationMs) const noexcept {
        return {durationMs > UINT64_MAX - elapsedMs ? UINT64_MAX : elapsedMs + durationMs};
    }
    std::array<char, 32> text() const noexcept {
        std::array<char, 32> out{};
        const auto c = calendar();
        std::snprintf(out.data(), out.size(), "Day %" PRIu64 " %02u:%02u", c.day, c.hour, c.minute);
        return out;
    }
};
} // namespace blobber

#pragma once
#include "game/game_time.hpp"
#include <array>

namespace blobber::world_render {
struct OutdoorLighting {
    std::array<float, 3> ambient{1, 1, 1};
    bool operator==(const OutdoorLighting& other) const noexcept { return ambient == other.ambient; }
};
inline OutdoorLighting outdoorLighting(GameTime time) noexcept {
    const auto calendar = time.calendar();
    const unsigned minute = calendar.hour * 60 + calendar.minute;
    float day = 0;
    if (minute >= 7 * 60 && minute <= 17 * 60)
        day = 1;
    else if (minute > 5 * 60 && minute < 7 * 60)
        day = float(minute - 5 * 60) / 120.f;
    else if (minute > 17 * 60 && minute < 19 * 60)
        day = float(19 * 60 - minute) / 120.f;
    day = day * day * (3 - 2 * day);
    return {{{.30f + .70f * day, .36f + .64f * day, .50f + .50f * day}}};
}
} // namespace blobber::world_render

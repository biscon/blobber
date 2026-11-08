#pragma once
#include <cmath>
namespace blobber {
struct MenuBackgroundCrop {
    float x, y, width, height; // Normalized source-image coordinates.
};
inline MenuBackgroundCrop menuBackgroundCrop(double elapsedSeconds) noexcept {
    constexpr double period = 48;
    constexpr double tau = 6.283185307179586;
    const double phase =
        std::isfinite(elapsedSeconds) && elapsedSeconds > 0 ? std::fmod(elapsedSeconds, period) / period : 0;
    const double angle = tau * phase;
    const double travel = .5 - .5 * std::cos(angle);
    const double extent = 1 / (1.025 + .03 * travel);
    const double margin = 1 - extent;
    return {float(margin * (.5 + .28 * std::sin(angle))), float(margin * (.5 + .20 * (2 * travel - 1))),
            float(extent), float(extent)};
}
} // namespace blobber

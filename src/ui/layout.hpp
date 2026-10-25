#pragma once
#include "render/layout.hpp"
#include <cmath>
#include <optional>
namespace blobber {
struct ArtPoint {
    float x = 0, y = 0;
};
struct UiRect {
    int x, y, width, height;
    bool contains(ArtPoint p) const noexcept {
        return p.x >= x && p.y >= y && p.x < x + width && p.y < y + height;
    }
};
namespace hudLayout {
constexpr UiRect card(int i) {
    return {4 + i * 159, 288, 155, 68};
}
constexpr UiRect action(int i, bool production = false) {
    return {10, 38 + i * (production ? 14 : 16), 112, production ? 13 : 15};
}
constexpr UiRect target(int i) {
    return {10, 150 + i * 13, 112, 12};
}
constexpr UiRect previousPage{10, 135, 18, 13}, nextPage{104, 135, 18, 13};
constexpr UiRect confirm{10, 204, 112, 16}, feedback{10, 223, 112, 25}, log{10, 251, 112, 27};
constexpr UiRect expandedLog{152, 28, 460, 232};
constexpr UiRect viewport{viewX, viewY, viewWidth, viewHeight};
constexpr UiRect result{210, 105, 340, 90}, resultButton{300, 164, 160, 20};
constexpr UiRect demoNext{390, 8, 110, 16}, demoReset{505, 8, 120, 16};
} // namespace hudLayout
// Raylib pointer coordinates are logical; aspectFit operates in framebuffer pixels.
inline std::optional<ArtPoint> artPoint(float x, float y, int logicalWidth, int logicalHeight,
                                        int renderWidth, int renderHeight) noexcept {
    if (logicalWidth <= 0 || logicalHeight <= 0 || renderWidth <= 0 || renderHeight <= 0 ||
        !std::isfinite(x) || !std::isfinite(y))
        return {};
    const auto fit = aspectFit(renderWidth, renderHeight);
    const float px = x * (float(renderWidth) / logicalWidth), py = y * (float(renderHeight) / logicalHeight);
    if (px < fit.x || py < fit.y || px >= fit.x + fit.width || py >= fit.y + fit.height)
        return {};
    return ArtPoint{(px - fit.x) * artWidth / fit.width, (py - fit.y) * artHeight / fit.height};
}
} // namespace blobber

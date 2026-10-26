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
    return {508, 18 + i * 70, 128, 66};
}
constexpr UiRect portrait(int i) {
    const auto r = card(i);
    return {r.x + 3, r.y + 4, 48, 56};
}
constexpr UiRect action(int i, bool = false) {
    return {12 + i * 47, 317, 34, 32};
}
constexpr UiRect detail(int i) {
    const auto r = card(i);
    return {r.x + 114, r.y + 54, 12, 10};
}

constexpr UiRect header{4, 2, 632, 13}, gear{621, 2, 14, 13};
constexpr UiRect actions{4, 304, 300, 54}, log{308, 304, 328, 54};
constexpr UiRect logExpand{620, 306, 12, 10}, logLatest{578, 306, 38, 10};
constexpr UiRect expandedLog{72, 48, 496, 244}, closeOverlay{551, 51, 14, 12};
constexpr UiRect details{148, 65, 344, 210}, detailsClose{474, 68, 14, 12};
constexpr UiRect menu{222, 103, 196, 134}, resume{242, 153, 156, 24}, quit{242, 191, 156, 24};
constexpr UiRect viewport{viewX, viewY, viewWidth, viewHeight};
constexpr UiRect result{102, 118, 304, 91}, resultButton{174, 174, 160, 22};
constexpr UiRect demoNext{400, 2, 50, 13}, demoReset{453, 2, 56, 13};
// Diagnostic compatibility: these legacy controls have no interactive surface.
constexpr UiRect target(int) {
    return {-100, -100, 0, 0};
}
constexpr UiRect previousPage{-100, -100, 0, 0}, nextPage{-100, -100, 0, 0}, confirm{-100, -100, 0, 0};
constexpr UiRect feedback{10, 350, 284, 8};
constexpr int logColumns = 76, expandedLogColumns = 116;
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

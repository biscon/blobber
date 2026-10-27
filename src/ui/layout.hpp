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
    return {r.x + 5, r.y + 4, 46, 56};
}
constexpr UiRect action(int i, bool = false) {
    return {12 + i * 47, 319, 34, 30};
}
constexpr UiRect detail(int i) {
    const auto r = card(i);
    return {r.x + 114, r.y + 54, 12, 10};
}

constexpr int spellRows = 7;
constexpr UiRect spellbook{8, 132, 306, 168}, spellbookClose{296, 137, 12, 11};
constexpr UiRect spellDetails{143, 153, 165, 140};
constexpr UiRect spellEntry(int row) {
    return {14, 154 + row * 18, 123, 17};
}
constexpr UiRect spellPrevious{14, 281, 62, 12}, spellNext{90, 281, 64, 12};
constexpr UiRect spellTargetPanel{4, 304, 380, 54};
constexpr UiRect spellCast{288, 310, 90, 19}, spellBack{288, 334, 41, 17}, spellCancel{333, 334, 45, 17};
constexpr UiRect recipientChip(int i) {
    return {202 + i * 26, 322, 23, 28};
}
constexpr UiRect castingLog{388, 304, 248, 54};
constexpr int castingLogColumns = 55;
constexpr UiRect header{4, 2, 632, 13}, gear{621, 2, 14, 13};
constexpr UiRect actions{4, 304, 300, 54}, log{308, 304, 328, 54};
constexpr UiRect logExpand{620, 306, 12, 10}, logLatest{578, 306, 38, 10};
constexpr UiRect expandedLog{72, 48, 496, 244}, closeOverlay{551, 51, 14, 12};
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

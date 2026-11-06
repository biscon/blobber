#pragma once
#include "ui/layout.hpp"

namespace blobber {
// Logical art coordinates. Only the world viewport and attached decorations move.
struct WorldViewTransform {
    float x = 0, y = 0, zoom = 1;
    bool identity() const noexcept { return x == 0 && y == 0 && zoom == 1; }
    ArtPoint forward(ArtPoint p) const noexcept {
        if (identity())
            return p;
        constexpr float cx = viewX + viewWidth * .5f, cy = viewY + viewHeight * .5f;
        return {cx + (p.x - cx) * zoom + x, cy + (p.y - cy) * zoom + y};
    }
    // Reject fixed HUD space before undoing the crop. Return full art coordinates.
    std::optional<ArtPoint> inverse(ArtPoint p) const noexcept {
        if (!hudLayout::viewport.contains(p))
            return {};
        if (identity())
            return p;
        constexpr float cx = viewX + viewWidth * .5f, cy = viewY + viewHeight * .5f;
        return ArtPoint{cx + (p.x - cx - x) / zoom, cy + (p.y - cy - y) / zoom};
    }
    // Pixel scissor enclosing transformed geometry, intersected with the viewport.
    UiRect nativeClip(UiRect rect) const noexcept {
        constexpr float scale = float(gameWidth) / artWidth;
        const auto a = forward({float(rect.x), float(rect.y)});
        const auto b = forward({float(rect.x + rect.width), float(rect.y + rect.height)});
        const int left =
            std::clamp(int(std::floor(a.x * scale)), int(viewX * scale), int((viewX + viewWidth) * scale));
        const int top =
            std::clamp(int(std::floor(a.y * scale)), int(viewY * scale), int((viewY + viewHeight) * scale));
        const int right = std::clamp(int(std::ceil(b.x * scale)), left, int((viewX + viewWidth) * scale));
        const int bottom = std::clamp(int(std::ceil(b.y * scale)), top, int((viewY + viewHeight) * scale));
        return {left, top, right - left, bottom - top};
    }
};
} // namespace blobber

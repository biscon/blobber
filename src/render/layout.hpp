#pragma once
#include <algorithm>

namespace blobber {
constexpr int artWidth = 640, artHeight = 360, gameWidth = 1920, gameHeight = 1080;
constexpr int viewWidth = 500, viewHeight = 272, viewX = 132, viewY = 8;
struct FitRect {
    float x, y, width, height;
};
inline FitRect aspectFit(int width, int height) {
    if (width <= 0 || height <= 0)
        return {0, 0, 0, 0};
    const float scale = std::min(float(width) / gameWidth, float(height) / gameHeight);
    return {(width - gameWidth * scale) / 2, (height - gameHeight * scale) / 2, gameWidth * scale,
            gameHeight * scale};
}
} // namespace blobber

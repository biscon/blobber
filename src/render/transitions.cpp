#include "render/transitions.hpp"
#include "render/projection.hpp"
#include <algorithm>
#include <stdexcept>

namespace blobber {
namespace {
constexpr float horizon = ViewProjection::horizon;
Color at(const Frame& frame, int x, int y) {
    return frame[std::clamp(y, 0, viewHeight - 1) * viewWidth + std::clamp(x, 0, viewWidth - 1)];
}
} // namespace
void composeTransition(const Frame& from, const Frame& to, Action action, float t, Frame& result) {
    if (action == Action::Left || action == Action::Right)
        throw std::invalid_argument("Use SceneRenderer::composeStrafe for side steps");
    if (result.data() == from.data() || result.data() == to.data())
        throw std::invalid_argument("Transition output must not alias its inputs");
    if (t <= 0) {
        result.copyFrom(from);
        return;
    }
    if (t >= 1) {
        result.copyFrom(to);
        return;
    }
    const float eased = t * t * (3 - 2 * t);
    for (int y = 0; y < viewHeight; ++y) {
        for (int x = 0; x < viewWidth; ++x) {
            Color pixel;
            if (action == Action::Forward || action == Action::Back) {
                // Old frame expands towards the viewer; the destination settles from
                // a small inset. Reverse the same operation for a backward step.
                const bool back = action == Action::Back;
                const float phase = back ? 1 - eased : eased;
                const Frame& old = back ? to : from;
                const Frame& next = back ? from : to;
                const float scale = 1 + phase * 0.32f;
                const float insetScale = 0.74f + 0.26f * phase;
                const float nx = (x - viewWidth * 0.5f) / insetScale + viewWidth * 0.5f;
                const float ny = (y - horizon) / insetScale + horizon;
                // Expanding inset replaces the old image without alpha-blended ghost walls.
                if (phase > 0.38f && nx >= 0 && nx < viewWidth && ny >= 0 && ny < viewHeight)
                    pixel = at(next, static_cast<int>(nx), static_cast<int>(ny));
                else
                    pixel = at(old, static_cast<int>((x - viewWidth * 0.5f) / scale + viewWidth * 0.5f),
                               static_cast<int>((y - horizon) / scale + horizon));
            } else {
                const bool left = action == Action::TurnLeft;
                const int px = left ? viewWidth - 1 - x : x;
                const int shift = static_cast<int>(eased * viewWidth);
                const int boundary = viewWidth - shift;
                const Frame& source = px < boundary ? from : to;
                float sx;
                if (px < boundary)
                    sx = viewWidth * eased * (2 - eased) + px * (1 - eased);
                else
                    sx = (px - boundary) * eased;
                const int sampleX = left ? viewWidth - 1 - static_cast<int>(sx) : static_cast<int>(sx);
                pixel = at(source, sampleX, y);
            }
            result[y * viewWidth + x] = pixel;
        }
    }
}
} // namespace blobber

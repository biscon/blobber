#include "render/scene_cache.hpp"
#include "render/lighting.hpp"
#include "world/grid.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace blobber {
float ViewProjection::focalLength() {
    constexpr float pi = 3.14159265358979323846f;
    return viewWidth * 0.5f / std::tan(horizontalFovDegrees * pi / 360.0f);
}

namespace {
const float focal = ViewProjection::focalLength();
constexpr float horizon = ViewProjection::horizon, setback = ViewProjection::cameraSetback;
constexpr float farDepth = ViewProjection::visibleRows + 0.5f + setback;
Color shadeOpaque(Color color, float depth, float face = 1.0f) {
    color.a = 255;
    return shadeScene(color, depth, face);
}
void bakeWalls(const Material& material, float offset, PreparedView& view) {
    // At fixed cardinal views the projected geometry is invariant. Prepare it once.
    // The near faces and inward-facing sides of solid cells are the only visible faces.
    // Wider FOVs need more lateral slots in the distant rows, not just new scaling.
    const int lateralLimit = static_cast<int>(std::ceil(farDepth * viewWidth * 0.5f / focal + 0.5f + offset));
    for (int distance = ViewProjection::visibleRows; distance >= 0; --distance) {
        for (int lateral = -lateralLimit; lateral <= lateralLimit; ++lateral) {
            for (bool side : {false, true}) {
                const float relativeX = lateral - offset;
                if ((!side && distance == 0) || (side && std::abs(relativeX) <= 0.5f))
                    continue;
                const int sideNeighbor = side ? (relativeX > 0 ? -1 : 1) : 0;
                PreparedSprite sprite{lateral, distance, sideNeighbor,
                                      static_cast<std::uint32_t>(view.columns.size()), 0};
                for (int x = 0; x < viewWidth; ++x) {
                    const float slope = (x + 0.5f - viewWidth * 0.5f) / focal;
                    float depth, u;
                    if (side) {
                        const float wallX = relativeX + sideNeighbor * 0.5f;
                        if (std::abs(slope) < 0.00001f)
                            continue;
                        depth = wallX / slope;
                        const float nearEdge = distance - 0.5f + setback;
                        if (depth < std::max(0.001f, nearEdge) || depth >= distance + 0.5f + setback)
                            continue;
                        u = depth - nearEdge;
                    } else {
                        depth = distance - 0.5f + setback;
                        u = slope * depth - (relativeX - 0.5f);
                        if (u < 0 || u >= 1)
                            continue;
                    }
                    const float top =
                        horizon - focal * (ViewProjection::wallHeight - ViewProjection::eyeHeight) / depth;
                    const float bottom = horizon + focal * ViewProjection::eyeHeight / depth;
                    const int y0 = std::max(0, static_cast<int>(std::ceil(top - 0.5f)));
                    const int y1 = std::min(viewHeight, static_cast<int>(std::ceil(bottom - 0.5f)));
                    const auto count = static_cast<std::uint32_t>(y1 - y0);
                    if (view.columns.size() == PreparedView::maxColumns ||
                        view.pixels.size() + 2 * count > PreparedView::maxPixels)
                        throw std::runtime_error("Prepared view exceeds its column/pixel capacity");
                    PreparedColumn column{x, y0, depth, count, {}};
                    for (int parity = 0; parity < 2; ++parity) {
                        column.pixelOffsets[parity] = static_cast<std::uint32_t>(view.pixels.size());
                        for (int y = y0; y < y1; ++y) {
                            const float v = (y + 0.5f - top) / (bottom - top);
                            view.pixels.push_back(
                                shadeOpaque(material.sample(u + ((lateral + parity) & 1), v), depth,
                                            side ? 0.78f : 1.0f));
                        }
                    }
                    view.columns.push_back(column);
                    ++sprite.columnCount;
                }
                if (sprite.columnCount) {
                    if (view.sprites.size() == PreparedView::maxSprites)
                        throw std::runtime_error("Prepared view exceeds sprite capacity");
                    view.sprites.push_back(sprite);
                }
            }
        }
    }
}
void bakeBackgrounds(const Material& floor, const Material& ceiling, float offset, PreparedView& view) {
    for (int direction = 0; direction < 4; ++direction) {
        const Cell f = forward(static_cast<Heading>(direction)), r = right(static_cast<Heading>(direction));
        for (int parity = 0; parity < 4; ++parity) {
            Frame& frame = view.backgrounds[direction * 4 + parity];
            for (int y = 0; y < viewHeight; ++y) {
                const bool isFloor = y + 0.5f >= horizon;
                const float planeHeight = isFloor ? ViewProjection::eyeHeight
                                                  : ViewProjection::wallHeight - ViewProjection::eyeHeight;
                const float depth = focal * planeHeight / std::abs(y + 0.5f - horizon);
                for (int x = 0; x < viewWidth; ++x) {
                    const int index = y * viewWidth + x;
                    if (depth > farDepth) {
                        frame[index] = sceneDarkness;
                        continue;
                    }
                    const float lateral = (x + 0.5f - viewWidth * 0.5f) * depth / focal;
                    // Independent world X/Y parity anchors the floor through a strafe;
                    // toggling both axes would make it jump diagonally at the endpoint.
                    const float wx = 0.5f + (parity / 2) + r.x * (lateral + offset) + f.x * (depth - setback);
                    const float wy = 0.5f + (parity % 2) + r.y * (lateral + offset) + f.y * (depth - setback);
                    const Color texel = (isFloor ? floor : ceiling).sample(wx, wy);
                    frame[index] = shadeOpaque(texel, depth, isFloor ? 0.85f : 0.58f);
                }
            }
        }
    }
}
} // namespace
PreparedScene prepareScene(const Materials& materials) {
    PreparedScene views;
    for (int phase = 0; phase < ViewProjection::strafeSteps; ++phase) {
        const float offset = float(phase) / ViewProjection::strafeSteps;
        bakeWalls(materials.wall, offset, views[phase]);
        bakeBackgrounds(materials.floor, materials.ceiling, offset, views[phase]);
    }
    return views;
}
} // namespace blobber

#pragma once
#include "render/frame.hpp"
#include "render/material.hpp"
#include "render/preparation_limits.hpp"
#include "render/projection.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace blobber {
// Internal immutable ranges: offsets remain valid after vector growth during loading.
struct PreparedColumn {
    int x, top;
    float depth;
    std::uint32_t length;
    std::array<std::uint32_t, 2> pixelOffsets;
};
struct PreparedSprite {
    int lateral, distance, sideNeighbor;
    std::uint32_t firstColumn, columnCount;
};
struct PreparedView {
    static constexpr std::size_t maxSprites = 1024, maxColumns = 512000, maxPixels = 16000000;
    std::vector<PreparedSprite> sprites;
    std::vector<PreparedColumn> columns;
    std::vector<Color> pixels;
    std::array<Frame, 16> backgrounds;
};
using PreparedScene = std::array<PreparedView, ViewProjection::strafeSteps>;
struct ScenePreparationStats {
    std::size_t sourceBytes = 0, loadingHighWater = 0;
};
std::size_t sceneStorageBytes(const PreparedScene& scene) noexcept;
PreparedScene prepareScene(const Materials& materials, PreparationLimits limits = {},
                           ScenePreparationStats* stats = nullptr); // Offline only.
} // namespace blobber

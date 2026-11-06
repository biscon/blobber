#pragma once
#include <algorithm>
#include <array>
#include <string_view>

namespace blobber::world_render {
enum class DungeonLighting { Current, Gentle, Strong, Early };

inline DungeonLighting effectiveDungeonLighting(DungeonLighting selected, std::string_view pack) noexcept {
    return pack == "underkeep" ? selected : DungeonLighting::Current;
}

// Onset in camera-depth cells and additional distance slope. Fog uses unmodified depth.
inline std::array<float, 2> dungeonLightingParameters(DungeonLighting preset) noexcept {
    switch (preset) {
    case DungeonLighting::Gentle:
        return {2.f, .5f};
    case DungeonLighting::Strong:
        return {2.f, 1.f};
    case DungeonLighting::Early:
        return {0.f, .5f};
    default:
        return {0.f, 0.f};
    }
}

inline float dungeonLightingDepth(float depth, DungeonLighting preset) noexcept {
    const auto p = dungeonLightingParameters(preset);
    return depth + p[1] * std::max(depth - p[0], 0.f);
}
} // namespace blobber::world_render

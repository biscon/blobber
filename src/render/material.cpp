#include "render/material.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace blobber {
Material::Material(int width, int height, std::vector<Color> pixels)
    : width_(width), height_(height), pixels_(std::move(pixels)) {
    if (width <= 0 || height <= 0 || width > maxDimension || height > maxDimension ||
        pixels_.size() != static_cast<std::size_t>(width) * static_cast<std::size_t>(height))
        throw std::invalid_argument("Invalid material dimensions or pixel count");
}
Color Material::sample(float u, float v) const noexcept {
    const auto mirror = [](float value) {
        const float base = std::floor(value);
        return (static_cast<int>(base) & 1) ? 1 - (value - base) : value - base;
    };
    const int x = std::clamp(static_cast<int>(mirror(u) * width_), 0, width_ - 1);
    const int y = std::clamp(static_cast<int>(mirror(v) * height_), 0, height_ - 1);
    return pixels_[y * width_ + x];
}
} // namespace blobber

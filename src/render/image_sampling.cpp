#include "render/image_sampling.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace blobber {
namespace {
void validateFootprint(double left, double top, double right, double bottom) {
    if (!std::isfinite(left) || !std::isfinite(top) || !std::isfinite(right) || !std::isfinite(bottom) ||
        right <= left || bottom <= top)
        throw std::invalid_argument("Invalid area sampling footprint");
}
Color average(const std::array<double, 4>& total, double area) {
    const auto byte = [](double value) {
        return static_cast<unsigned char>(std::clamp(std::round(value), 0.0, 255.0));
    };
    const auto alpha = byte(total[3] / area);
    if (!alpha || total[3] <= 0)
        return {0, 0, 0, 0};
    return {byte(total[0] / total[3]), byte(total[1] / total[3]), byte(total[2] / total[3]), alpha};
}
} // namespace
AreaSampler::AreaSampler(const Color* pixels, int width, int height, int stride, unsigned char cutoff)
    : width_(width), height_(height) {
    if (!pixels || width <= 0 || height <= 0 || width > 4096 || height > 4096 || stride < width)
        throw std::invalid_argument("Invalid area sampler image");
    const auto count = static_cast<std::size_t>(width + 1) * (height + 1);
    if (count > 256u * 1024u * 1024u / sizeof(Sum))
        throw std::runtime_error("Area sampling scratch exceeds 256 MiB");
    sums_.resize(count);
    for (int y = 0; y < height; ++y) {
        Sum row{};
        for (int x = 0; x < width; ++x) {
            const auto c = pixels[static_cast<std::size_t>(y) * stride + x];
            const double alpha = c.a > cutoff ? c.a : 0;
            row[0] += c.r * alpha;
            row[1] += c.g * alpha;
            row[2] += c.b * alpha;
            row[3] += alpha;
            auto& sum = sums_[static_cast<std::size_t>(y + 1) * (width + 1) + x + 1];
            const auto& above = sums_[static_cast<std::size_t>(y) * (width + 1) + x + 1];
            for (int k = 0; k < 4; ++k)
                sum[k] = row[k] + above[k];
        }
    }
}
AreaSampler::Sum AreaSampler::integral(double x, double y) const {
    x = std::clamp(x, 0.0, double(width_));
    y = std::clamp(y, 0.0, double(height_));
    const int ix = std::min(int(x), width_ - 1), iy = std::min(int(y), height_ - 1);
    const double fx = x - ix, fy = y - iy;
    Sum result{};
    for (int dy = 0; dy < 2; ++dy)
        for (int dx = 0; dx < 2; ++dx) {
            const auto& s = sums_[static_cast<std::size_t>(iy + dy) * (width_ + 1) + ix + dx];
            const double weight = (dx ? fx : 1 - fx) * (dy ? fy : 1 - fy);
            for (int k = 0; k < 4; ++k)
                result[k] += s[k] * weight;
        }
    return result;
}
Color AreaSampler::sample(double left, double top, double right, double bottom) const {
    validateFootprint(left, top, right, bottom);
    const auto a = integral(left, top), b = integral(right, top);
    const auto c = integral(left, bottom), d = integral(right, bottom);
    Sum total{};
    for (int k = 0; k < 4; ++k)
        total[k] = std::max(0.0, d[k] - b[k] - c[k] + a[k]);
    return average(total, (right - left) * (bottom - top));
}
} // namespace blobber

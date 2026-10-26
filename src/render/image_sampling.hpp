#pragma once
#include <array>
#include <raylib.h>
#include <vector>

namespace blobber {
enum class ImageSampling { Nearest, Area };

// Offline only. Continuous summed-area integration of straight-alpha RGBA.
// RGB is averaged in the source's encoded color space, weighted by alpha.
// Out-of-image samples are transparent. The table owns no borrowed image data.
class AreaSampler {
  public:
    AreaSampler(const Color* pixels, int width, int height, int stride, unsigned char cutoff = 0);
    Color sample(double left, double top, double right, double bottom) const;
    std::size_t storageBytes() const noexcept { return sums_.capacity() * sizeof(Sum); }

  private:
    using Sum = std::array<double, 4>;
    Sum integral(double x, double y) const;
    int width_, height_;
    std::vector<Sum> sums_;
};
} // namespace blobber

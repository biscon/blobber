#pragma once
#include <array>
#include <optional>
#include <raylib.h>
#include <vector>

namespace blobber {
class Material {
  public:
    static constexpr int maxDimension = 4096;
    Material(int width, int height, std::vector<Color> pixels);
    Color sample(float u, float v) const noexcept;
    std::size_t storageBytes() const noexcept { return sizeof(*this) + pixels_.capacity() * sizeof(Color); }

  private:
    int width_, height_;
    std::vector<Color> pixels_;
};
// A concrete, fixed set; no dynamic material manager is needed yet.
struct Materials {
    Material wall, floor, ceiling;
    std::optional<Material> grease{}, web{}, weapon{};
};
} // namespace blobber

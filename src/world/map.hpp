#pragma once

#include "world/grid.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace blobber {
// Immutable after loading. Coordinates outside its bounds are solid.
class Map {
  public:
    static constexpr int maxDimension = 1024;
    explicit Map(const std::vector<std::string>& rows);
    static Map load(const std::string& path);
    bool solid(Cell cell) const noexcept;
    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }
    bool connected(Cell start) const; // Loading-time validation; may allocate.
  private:
    int width_ = 0, height_ = 0;
    std::vector<std::uint8_t> cells_;
};
} // namespace blobber

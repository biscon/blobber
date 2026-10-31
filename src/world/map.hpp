#pragma once

#include "world/grid.hpp"
#include "world/surfaces.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace blobber {
// Immutable after loading. Coordinates outside its bounds are solid.
class Map {
  public:
    static constexpr int maxDimension = 1024;
    explicit Map(const std::vector<std::string>& rows, WorldDefinition world = {});
    static Map load(const std::string& path);
    bool solid(Cell cell) const noexcept;
    CellSurface surface(Cell cell) const noexcept;
    bool canStep(Cell from, Cell to) const noexcept;
    const WorldDefinition& definition() const noexcept { return world_; }
    bool flat() const noexcept { return flat_; }
    float upper() const noexcept { return upper_; }
    float lower() const noexcept { return lower_; }
    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }
    bool connected(Cell start) const; // Loading-time validation; may allocate.
  private:
    int width_ = 0, height_ = 0;
    std::vector<std::uint8_t> cells_;
    WorldDefinition world_;
    float lower_ = 0, upper_ = 1;
    bool flat_ = true;
    void validateWorld();
};
} // namespace blobber

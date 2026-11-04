#pragma once
#include "world/fixtures.hpp"
#include <array>
#include <bitset>
namespace blobber {
namespace generation {
struct FloorLayout;
}
enum class MapTile : std::uint8_t { Hidden, Floor, Wall };
enum class MapMarker : std::uint8_t {
    None,
    Party,
    ClosedDoor,
    OpenDoor,
    GuardianGate,
    Key,
    Upstairs,
    Downstairs,
    Objective
};
MapMarker mapFixtureMarker(const FloorGeometry&, unsigned slot, bool reveal) noexcept;

// Headless layout in art units. Oversized axes retain the party at the center.
struct MapGridView {
    static constexpr int cellSize = 9;
    Cell first{};
    int columns = 0, rows = 0;
    float x = 0, y = 0;
    static MapGridView fit(int width, int height, Cell party, int x, int y, int w, int h) noexcept;
    bool contains(Cell c) const noexcept {
        return c.x >= first.x && c.y >= first.y && c.x < first.x + columns && c.y < first.y + rows;
    }
    float left(Cell c) const noexcept { return x + (c.x - first.x) * cellSize; }
    float top(Cell c) const noexcept { return y + (c.y - first.y) * cellSize; }
};

// View-owned derived knowledge; immutable Map outlives this model. No saved state.
class MapOverlayModel {
  public:
    void prepare(const Map&, const generation::FloorLayout* rooms = nullptr) noexcept;
    void refresh(const FloorGeometry*) noexcept;
    MapTile tile(Cell, bool reveal = false) const noexcept;

  private:
    const Map* map_ = nullptr;
    std::array<std::uint8_t, discoveryCapacity> rooms_{};
    std::bitset<discoveryCapacity> floors_, walls_;
    unsigned roomCount_ = 0;
    bool fog_ = false, ready_ = false;
    std::uint64_t world_ = 0, discovery_ = 0;
    bool inside(Cell) const noexcept;
};
} // namespace blobber

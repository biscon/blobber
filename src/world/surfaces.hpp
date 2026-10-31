#pragma once
#include "world/grid.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>
namespace blobber {
constexpr std::int16_t noCeiling = INT16_MAX;
constexpr std::size_t worldDefinitionBudget = 16u * 1024 * 1024;
enum class Environment : std::uint8_t { Dungeon, Daylight };
struct CellSurface {
    std::int16_t floor = 0, ceiling = 4;
    std::uint8_t wallMaterial = 0, floorMaterial = 1, ceilingMaterial = 2;
    Environment environment = Environment::Dungeon;
};
static_assert(sizeof(CellSurface) == 8);
enum class DecorationKind : std::uint8_t { Crest, Vine, Bench, Urn };
struct Decoration {
    Cell cell;
    Heading face = Heading::North; // Direction from the cell toward its supporting wall.
    DecorationKind kind = DecorationKind::Crest;
    std::uint8_t material = 0;
    float offset = .5f, elevation = .55f, width = .45f, height = .45f, depth = .2f;
    bool cutout() const noexcept { return kind == DecorationKind::Crest || kind == DecorationKind::Vine; }
};
struct WorldDefinition {
    std::vector<CellSurface> surfaces;
    std::vector<Decoration> decorations;
    std::size_t bytes() const noexcept {
        return sizeof(*this) + surfaces.capacity() * sizeof(CellSurface) +
               decorations.capacity() * sizeof(Decoration);
    }
    std::uint64_t fingerprint() const noexcept;
};
inline float elevation(std::int16_t q) noexcept {
    return float(q) * .25f;
}
inline bool surfacePassage(CellSurface a, CellSurface b) noexcept {
    const int delta = int(a.floor) - b.floor;
    const int bottom = a.floor > b.floor ? a.floor : b.floor;
    const int top = a.ceiling < b.ceiling ? a.ceiling : b.ceiling;
    return delta >= -1 && delta <= 1 && top - bottom >= 4;
}
} // namespace blobber

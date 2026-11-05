#pragma once
#include "world/grid.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
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
constexpr unsigned containerCapacity = 8;
constexpr std::uint8_t noContainer = 255;
enum class DecorationKind : std::uint8_t {
    Crest,
    Vine,
    Bench,
    Urn,
    DampA,
    DampB,
    Cracks,
    Soot,
    Blood,
    BannerA,
    BannerB,
    WebA,
    WebB,
    BonesA,
    BonesB,
    Pilaster,
    WallBase,
    DoorSurround,
    Cage,
    Rack,
    Chest
};
constexpr unsigned decorationKinds = unsigned(DecorationKind::Chest) + 1;
const char* decorationName(DecorationKind) noexcept;
struct Decoration {
    Cell cell;
    Heading face = Heading::North; // Direction from the cell toward its supporting wall.
    DecorationKind kind = DecorationKind::Crest;
    std::uint8_t material = 0;
    float offset = .5f, elevation = .55f, width = .45f, height = .45f, depth = .2f;
    std::uint8_t container = noContainer;
    bool sealed = false;
    bool cutout() const noexcept {
        return kind == DecorationKind::Crest || kind == DecorationKind::Vine ||
               (kind >= DecorationKind::DampA && kind <= DecorationKind::BonesB);
    }
    bool blocks() const noexcept { return kind == DecorationKind::Cage || kind == DecorationKind::Rack; }
    // Physical reservations protect even walkable chest cells from other props/NPCs.
    unsigned footprintCount() const noexcept { return kind == DecorationKind::Rack ? 2 : model() ? 1 : 0; }
    Cell footprint(unsigned i) const noexcept { return cell + forward(face) * int(i); }
    bool model() const noexcept { return kind >= DecorationKind::Cage; }
    bool floorCutout() const noexcept {
        return kind == DecorationKind::BonesA || kind == DecorationKind::BonesB ||
               kind == DecorationKind::Blood;
    }
    bool web() const noexcept { return kind == DecorationKind::WebA || kind == DecorationKind::WebB; }
};
constexpr unsigned buildingCapacity = 128;
// Local footprint cells rotate with the model about the anchor cell center.
struct BuildingPlacement {
    std::string model;
    Cell anchor, footprint;
    Heading facing = Heading::North;
    unsigned width = 1, depth = 1;
    Cell cell(unsigned x, unsigned z) const noexcept {
        const auto r = right(facing), f = forward(facing);
        return anchor + r * (footprint.x + int(x)) + f * -(footprint.y + int(z));
    }
};
constexpr unsigned sceneryCapacity = 32;
// Open-terrain physical base; visual overhangs do not reserve extra cells.
struct SceneryPlacement {
    std::string model;
    Cell anchor, footprint;
    Heading facing = Heading::North;
    unsigned width = 1, depth = 1;
    Cell cell(unsigned x, unsigned z) const noexcept {
        return anchor + right(facing) * (footprint.x + int(x)) + forward(facing) * -(footprint.y + int(z));
    }
};
struct WorldDefinition {
    std::string assetPack = "dungeon";
    std::vector<CellSurface> surfaces;
    std::vector<Decoration> decorations;
    std::vector<BuildingPlacement> buildings;
    std::vector<SceneryPlacement> scenery;
    std::size_t bytes() const noexcept {
        std::size_t n = sizeof(*this) + assetPack.capacity() + surfaces.capacity() * sizeof(CellSurface) +
                        decorations.capacity() * sizeof(Decoration) +
                        buildings.capacity() * sizeof(BuildingPlacement) +
                        scenery.capacity() * sizeof(SceneryPlacement);
        for (const auto& b : buildings)
            n += b.model.capacity();
        for (const auto& s : scenery)
            n += s.model.capacity();
        return n;
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

#pragma once
#include "generation/floor.hpp"

namespace blobber::generation {
// Reservations are floor-local. Absent sites have cell (-1,-1).
enum class CaveSite : unsigned {
    Camp,
    SpiderArch,
    ArchWeb,
    PocketWeb,
    SoloNewt,
    Fork,
    FarBank,
    NorthContact,
    SouthContact,
    Rope,
    Mother,
    WestClutch,
    EastClutch,
    WestMelee,
    EastMelee,
    WestHatch,
    EastHatch,
    Optional,
    Goal,
    HighGallery,
    LowGallery,
    Lookout,
    Count
};
constexpr unsigned caveSiteCount = unsigned(CaveSite::Count);
struct CaveSites {
    std::array<Cell, caveSiteCount> cells;
    CaveSites() { cells.fill({-1, -1}); }
    Cell& operator[](CaveSite site) { return cells[unsigned(site)]; }
    Cell operator[](CaveSite site) const { return cells[unsigned(site)]; }
};
struct CaveFloor {
    FloorLayout layout;
    CaveSites sites;
    Map map;
};
struct CaveResult {
    std::optional<CaveFloor> floor;
    GenerationDiagnostics diagnostics;
};
CaveResult generateCave(std::uint32_t seed, unsigned depth, bool forceFallback = false);
Failure validateCave(const CaveFloor&);
const char* siteName(CaveSite);
} // namespace blobber::generation

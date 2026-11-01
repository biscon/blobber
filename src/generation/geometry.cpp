#include "generation/floor.hpp"
namespace blobber::generation {
Cell rotate(Cell c, int extent, unsigned n) noexcept {
    for (unsigned i = 0; i < n % 4; ++i)
        c = {extent - 1 - c.y, c.x};
    return c;
}
const BiomeDefinition& underkeepBiome() noexcept {
    static constexpr BiomeDefinition biome{BiomeId::Underkeep,
                                           AlgorithmId::StoneRooms,
                                           "underkeep",
                                           {LayoutProfile::CompactWings, LayoutProfile::ChamberNetwork}};
    return biome;
}
std::optional<FloorSpec> floorSpec(unsigned depth) noexcept {
    if (depth < 1 || depth > 3)
        return std::nullopt;
    return FloorSpec{depth, floorDimension, floorDimension, BiomeId::Underkeep, depth, depth > 1, depth == 3};
}
const char* profileName(LayoutProfile p) noexcept {
    return p == LayoutProfile::CompactWings ? "compact_wings" : "chamber_network";
}
bool inside(const FloorLayout& f, Cell c) noexcept {
    return c.x >= 0 && c.y >= 0 && c.x < f.width && c.y < f.height;
}
bool floorCell(const FloorLayout& f, Cell c) noexcept {
    return inside(f, c) && f.floor[unsigned(c.y * f.width + c.x)] == 1;
}
unsigned regionAt(const FloorLayout& f, Cell c) noexcept {
    return inside(f, c) ? f.membership[unsigned(c.y * f.width + c.x)] : noRegion;
}
Cell pathCell(const FloorLayout& f, const Connection& e, unsigned step) noexcept {
    const unsigned index = e.first + step;
    if (step >= e.count || index >= f.pathCount || index >= maxCells)
        return {-1, -1};
    const auto v = f.paths[index];
    return {v % f.width, v / f.width};
}
unsigned sectorJunction(const FloorLayout& f) noexcept {
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto& e = f.connections[i];
        if (e.a >= f.roomCount || e.b >= f.roomCount)
            continue;
        if (f.rooms[e.a].role == Role::Approach && f.rooms[e.b].role == Role::Junction)
            return e.b;
        if (f.rooms[e.b].role == Role::Approach && f.rooms[e.a].role == Role::Junction)
            return e.a;
    }
    return maxRooms;
}
const Connection* connectionBetween(const FloorLayout& f, unsigned a, unsigned b) noexcept {
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto& e = f.connections[i];
        if ((e.a == a && e.b == b) || (e.b == a && e.a == b))
            return &e;
    }
    return nullptr;
}
Heading incomingFacing(const Connection& e, unsigned room) noexcept {
    return e.a == room ? e.facingA : e.facingB;
}
Cell threshold(const Connection& e) noexcept {
    return e.socketA;
}
const char* roleName(Role r) noexcept {
    switch (r) {
    case Role::Junction:
        return "junction";
    case Role::Entrance:
        return "entrance";
    case Role::Approach:
        return "approach";
    case Role::Objective:
        return "objective";
    case Role::Optional:
        return "optional";
    }
    return "invalid";
}
const char* errorName(Error e) noexcept {
    switch (e) {
    case Error::None:
        return "none";
    case Error::Request:
        return "invalid_request";
    case Error::Capacity:
        return "capacity";
    case Error::Template:
        return "invalid_template";
    case Error::Bounds:
        return "bounds";
    case Error::Room:
        return "invalid_room";
    case Error::Port:
        return "unmatched_port";
    case Error::Connection:
        return "invalid_connection";
    case Error::Anchor:
        return "invalid_anchor";
    case Error::Disconnected:
        return "disconnected";
    case Error::Loop:
        return "unhelpful_loop";
    case Error::Branch:
        return "invalid_optional_branch";
    case Error::Fixture:
        return "invalid_fixture";
    case Error::Progression:
        return "invalid_progression";
    case Error::Population:
        return "invalid_population";
    case Error::Landing:
        return "unsafe_landing";
    case Error::ForcedFallback:
        return "forced_fallback";
    }
    return "invalid";
}
} // namespace blobber::generation

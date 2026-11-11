#include "game/perception.hpp"
#include "world/view.hpp"
#include <algorithm>
#include <cstdlib>
namespace blobber {
static bool sight(GeometryView map, Cell from, Cell target, bool blockingEndpoint,
                  bool blockDoors = false) noexcept {
    if (map.solid(from) || (!blockingEndpoint && map.solid(target)))
        return false;
    const int dx = target.x - from.x, dz = target.y - from.y, nx = std::abs(dx), nz = std::abs(dz);
    const int sx = dx > 0 ? 1 : -1, sz = dz > 0 ? 1 : -1;
    const std::int64_t h0 = map.surface(from).floor + 2, dh = map.surface(target).floor + 2 - h0;
    auto clear = [&](Cell c, std::int64_t n, std::int64_t d) {
        if (blockDoors && map.fixtures()) {
            const auto i = map.fixtures()->at(c);
            if (i >= 0) {
                const auto kind = map.fixtures()->definition().entries[unsigned(i)].kind;
                if (kind == FixtureKind::Door || kind == FixtureKind::GuardianGate)
                    return false;
            }
        }
        if (blockingEndpoint && c == target && map.solid(c))
            return true;
        if (map.solid(c))
            return false;
        const auto s = map.surface(c);
        const auto h = h0 * d + dh * n;
        return h > std::int64_t(s.floor) * d && (s.ceiling == noCeiling || h < std::int64_t(s.ceiling) * d);
    };
    Cell c = from;
    int ix = 0, iz = 0;
    std::int64_t en = 0, ed = 1;
    while (true) {
        const std::int64_t xn = 2 * ix + 1, xd = 2 * nx, zn = 2 * iz + 1, zd = 2 * nz;
        const bool useX = ix < nx && (iz == nz || xn * zd <= zn * xd);
        const bool useZ = iz < nz && (ix == nx || zn * xd <= xn * zd);
        const std::int64_t ex = useX ? xn : useZ ? zn : 1, den = useX ? xd : useZ ? zd : 1;
        if (!clear(c, en, ed) || !clear(c, ex, den))
            return false;
        if (c == target)
            return true;
        if (useX && useZ && (!clear(c + Cell{sx, 0}, ex, den) || !clear(c + Cell{0, sz}, ex, den)))
            return false;
        if (useX) {
            c.x += sx;
            ++ix;
        }
        if (useZ) {
            c.y += sz;
            ++iz;
        }
        en = ex;
        ed = den;
    }
}

bool lineOfSight(GeometryView map, Cell from, Cell target) noexcept {
    return sight(map, from, target, false);
}
bool alertLineOfSight(GeometryView map, Cell from, Cell target) noexcept {
    return sight(map, from, target, false, true);
}
bool discoveryVisible(GeometryView map, Cell from, Heading facing, Cell target) noexcept {
    if (unsigned(facing) >= 4 || !inBowRange(from, target))
        return false;
    const auto f = forward(facing), delta = target + from * -1;
    const int ahead = delta.x * f.x + delta.y * f.y, side = delta.x * f.y - delta.y * f.x;
    return ((ahead == 1 && std::abs(side) <= 1) || (ahead > 0 && std::abs(side) < ahead)) &&
           sight(map, from, target, true);
}

bool inBowRange(Cell from, Cell to) noexcept {
    const auto dx = to.x - from.x, dy = to.y - from.y;
    return (dx || dy) && dx * dx + dy * dy <= 36;
}
bool npcSees(GeometryView map, Cell from, Heading facing, Cell target) noexcept {
    if (static_cast<unsigned>(facing) >= 4 || !inBowRange(from, target))
        return false;
    const auto f = forward(facing);
    const int dx = target.x - from.x, dy = target.y - from.y;
    const int ahead = dx * f.x + dy * f.y, side = dx * f.y - dy * f.x;
    return ahead > 0 && std::abs(side) <= ahead && lineOfSight(map, from, target);
}
bool combatAdjacent(GeometryView map, Cell from, Cell target) noexcept {
    const int dx = std::abs(target.x - from.x), dy = std::abs(target.y - from.y);
    return std::max(dx, dy) == 1 && map.reach(from, target) && lineOfSight(map, from, target);
}
bool inFrontBand(Cell from, Heading heading, Cell target) noexcept {
    if (static_cast<unsigned>(heading) >= 4)
        return false;
    const auto f = forward(heading);
    const Cell delta{target.x - from.x, target.y - from.y};
    const int ahead = delta.x * f.x + delta.y * f.y, side = delta.x * f.y - delta.y * f.x;
    return ahead == 1 && std::abs(side) <= 1;
}
bool inMeleeFront(GeometryView map, Cell from, Heading heading, Cell target) noexcept {
    return inFrontBand(from, heading, target) && map.reach(from, target) && lineOfSight(map, from, target);
}
bool inPartyView(GeometryView map, Cell from, Heading facing, Cell target) noexcept {
    if (static_cast<unsigned>(facing) >= 4 || !inBowRange(from, target))
        return false;
    const auto f = forward(facing);
    const Cell delta{target.x - from.x, target.y - from.y};
    const int ahead = delta.x * f.x + delta.y * f.y, side = delta.x * f.y - delta.y * f.x;
    return ((ahead == 1 && std::abs(side) <= 1) || (ahead > 0 && std::abs(side) < ahead)) &&
           lineOfSight(map, from, target);
}
Heading headingToward(Cell from, Cell to) noexcept {
    const int dx = to.x - from.x, dy = to.y - from.y;
    return std::abs(dx) > std::abs(dy) ? (dx > 0 ? Heading::East : Heading::West)
                                       : (dy > 0 ? Heading::South : Heading::North);
}
bool inPartyViewport(Cell from, Heading heading, Cell target, Cell subcell) noexcept {
    if (unsigned(heading) >= 4 || unsigned(subcell.x) >= 2 || unsigned(subcell.y) >= 2)
        return false;
    const auto f = forward(heading), r = right(heading);
    const float dx = target.x - from.x + .5f * subcell.x - .25f,
                dz = target.y - from.y + .5f * subcell.y - .25f;
    const float ahead = dx * f.x + dz * f.y + partyCameraSetback, lateral = dx * r.x + dz * r.y;
    // Leave a small world-space margin so a sliver at the edge cannot hold the
    // locked camera. This is a body-axis test, not a sprite/alpha readback.
    return ahead > 0 && std::abs(lateral) < ahead * partyViewHalfTangent - .05f;
}
bool inPartyView(GeometryView map, Cell from, Heading heading, Cell target, Cell subcell) noexcept {
    return inBowRange(from, target) && inPartyViewport(from, heading, target, subcell) &&
           lineOfSight(map, from, target);
}
int cellDistance(Cell a, Cell b) noexcept {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}
} // namespace blobber

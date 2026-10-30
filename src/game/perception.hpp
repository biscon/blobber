#pragma once
#include "world/fixtures.hpp"
#include "world/map.hpp"
#include <cstdlib>
namespace blobber {
// Shared integer supercover for runtime geometry and allocation-free generation tiles.
template <class Solid>
bool gridSight(Solid solid, Cell from, Cell target, bool blockingEndpoint = false) noexcept {
    if (solid(from) || (!blockingEndpoint && solid(target)))
        return false;
    const int dx = target.x - from.x, dy = target.y - from.y, nx = std::abs(dx), ny = std::abs(dy),
              sx = dx > 0 ? 1 : -1, sy = dy > 0 ? 1 : -1;
    int ix = 0, iy = 0;
    auto c = from;
    while (ix < nx || iy < ny) {
        const auto a = (1 + 2 * ix) * ny, b = (1 + 2 * iy) * nx;
        if (a == b) {
            if (solid(c + Cell{sx, 0}) || solid(c + Cell{0, sy}))
                return false;
            c = c + Cell{sx, sy};
            ++ix;
            ++iy;
        } else if (a < b) {
            c.x += sx;
            ++ix;
        } else {
            c.y += sy;
            ++iy;
        }
        if (solid(c) && !(blockingEndpoint && c == target))
            return false;
    }
    return true;
}
bool discoveryVisible(GeometryView, Cell, Heading, Cell) noexcept;
bool lineOfSight(GeometryView, Cell, Cell) noexcept;
bool inBowRange(Cell, Cell) noexcept;
bool combatAdjacent(GeometryView, Cell, Cell) noexcept;
bool inFrontBand(Cell, Heading, Cell) noexcept;
bool inMeleeFront(GeometryView, Cell, Heading, Cell) noexcept;
bool inPartyView(GeometryView, Cell, Heading, Cell) noexcept;
bool npcSees(GeometryView, Cell, Heading, Cell) noexcept;
Heading headingToward(Cell, Cell) noexcept;
int cellDistance(Cell, Cell) noexcept;
} // namespace blobber

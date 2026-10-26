#include "game/perception.hpp"
#include <algorithm>
#include <cstdlib>
namespace blobber {
bool lineOfSight(const Map& map, Cell from, Cell target) noexcept {
    if (map.solid(from) || map.solid(target))
        return false;
    const int dx = target.x - from.x, dy = target.y - from.y;
    // Integer supercover ray between cell centers. Exact corner crossings test
    // both neighboring cells, so two diagonal floors cannot see through a seam.
    const int nx = std::abs(dx), ny = std::abs(dy), sx = dx > 0 ? 1 : -1, sy = dy > 0 ? 1 : -1;
    int ix = 0, iy = 0;
    Cell c = from;
    while (ix < nx || iy < ny) {
        const int xCross = (1 + 2 * ix) * ny, yCross = (1 + 2 * iy) * nx;
        if (xCross == yCross) {
            if (map.solid({c.x + sx, c.y}) || map.solid({c.x, c.y + sy}))
                return false;
            c.x += sx;
            c.y += sy;
            ++ix;
            ++iy;
        } else if (xCross < yCross) {
            c.x += sx;
            ++ix;
        } else {
            c.y += sy;
            ++iy;
        }
        if (map.solid(c))
            return false;
    }
    return true;
}

bool inBowRange(Cell from, Cell to) noexcept {
    const auto dx = to.x - from.x, dy = to.y - from.y;
    return (dx || dy) && dx * dx + dy * dy <= 36;
}
bool npcSees(const Map& map, Cell from, Heading facing, Cell target) noexcept {
    if (static_cast<unsigned>(facing) >= 4 || !inBowRange(from, target))
        return false;
    const auto f = forward(facing);
    const int dx = target.x - from.x, dy = target.y - from.y;
    const int ahead = dx * f.x + dy * f.y, side = dx * f.y - dy * f.x;
    return ahead > 0 && std::abs(side) <= ahead && lineOfSight(map, from, target);
}
bool combatAdjacent(const Map& map, Cell from, Cell target) noexcept {
    const int dx = std::abs(target.x - from.x), dy = std::abs(target.y - from.y);
    return std::max(dx, dy) == 1 && lineOfSight(map, from, target);
}
bool inFrontBand(Cell from, Heading heading, Cell target) noexcept {
    if (static_cast<unsigned>(heading) >= 4)
        return false;
    const auto f = forward(heading);
    const Cell delta{target.x - from.x, target.y - from.y};
    const int ahead = delta.x * f.x + delta.y * f.y, side = delta.x * f.y - delta.y * f.x;
    return ahead == 1 && std::abs(side) <= 1;
}
bool inMeleeFront(const Map& map, Cell from, Heading heading, Cell target) noexcept {
    return inFrontBand(from, heading, target) && lineOfSight(map, from, target);
}
bool inPartyView(const Map& map, Cell from, Heading facing, Cell target) noexcept {
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
int cellDistance(Cell a, Cell b) noexcept {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}
} // namespace blobber

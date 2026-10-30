#include "game/perception.hpp"
#include <algorithm>
#include <cstdlib>
namespace blobber {
static bool sight(GeometryView map, Cell from, Cell target, bool blockingEndpoint) noexcept {
    return gridSight([&](Cell c) { return map.solid(c); }, from, target, blockingEndpoint);
}

bool lineOfSight(GeometryView map, Cell from, Cell target) noexcept {
    return sight(map, from, target, false);
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
bool inMeleeFront(GeometryView map, Cell from, Heading heading, Cell target) noexcept {
    return inFrontBand(from, heading, target) && lineOfSight(map, from, target);
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
int cellDistance(Cell a, Cell b) noexcept {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}
} // namespace blobber

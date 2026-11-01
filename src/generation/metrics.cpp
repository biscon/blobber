#include "generation/floor.hpp"
#include <algorithm>
#include <limits>
namespace blobber::generation {
namespace {
unsigned walk(const FloorLayout& f, Cell start, Cell goal, bool key, bool guardian) noexcept {
    std::array<bool, maxCells> blocked{};
    std::array<std::uint16_t, maxCells> queue{}, distance{};
    distance.fill(UINT16_MAX);
    for (unsigned i = 0; i < f.fixtures.count; ++i) {
        const auto& d = f.fixtures.entries[i];
        if (stairFixture(d.kind) || (d.kind == FixtureKind::GuardianGate && !guardian) ||
            (d.kind == FixtureKind::Door && d.key && !key))
            blocked[unsigned(d.cell.y * f.width + d.cell.x)] = true;
    }
    unsigned head = 0, tail = 1;
    const auto first = unsigned(start.y * f.width + start.x);
    queue[0] = std::uint16_t(first);
    distance[first] = 0;
    while (head < tail) {
        const auto n = queue[head++];
        const Cell c{int(n) % f.width, int(n) / f.width};
        if (c == goal)
            return distance[n];
        for (unsigned h = 0; h < 4; ++h) {
            const auto p = c + forward(Heading(h));
            if (!floorCell(f, p))
                continue;
            const auto j = unsigned(p.y * f.width + p.x);
            if (blocked[j] || distance[j] != UINT16_MAX)
                continue;
            queue[tail++] = std::uint16_t(j);
            distance[j] = std::uint16_t(distance[n] + 1);
        }
    }
    return maxCells;
}
} // namespace
LayoutMetrics layoutMetrics(const FloorLayout& f) noexcept {
    LayoutMetrics m;
    if (f.width < 3 || f.height < 3 || f.width > 40 || f.height > 40 || f.roomCount > maxRooms ||
        f.connectionCount > maxConnections || f.fixtures.count > fixtureCapacity ||
        f.fixtures.landingCount != 2)
        return m;
    std::array<unsigned, maxRooms> areas{};
    for (unsigned i = 0; i < unsigned(f.width * f.height); ++i) {
        m.walkableCells += f.floor[i] == 1;
        if (f.membership[i] < f.roomCount)
            ++areas[f.membership[i]];
    }
    std::sort(areas.begin(), areas.begin() + f.roomCount);
    m.minRoomArea = areas[0];
    m.maxRoomArea = areas[f.roomCount - 1];
    for (unsigned i = 0; i < f.roomCount; ++i)
        m.distinctRoomAreas += i == 0 || areas[i] != areas[i - 1];
    std::array<bool, maxCells + 1> lengths{};
    for (unsigned i = 0; i < f.connectionCount; ++i)
        if (f.connections[i].count <= maxCells)
            lengths[f.connections[i].count] = true;
    m.distinctConnectionLengths = unsigned(std::count(lengths.begin(), lengths.end(), true));
    Cell start = f.spawn.cell;
    for (unsigned i = 0; i < f.fixtures.count; ++i)
        if (f.fixtures.entries[i].kind == FixtureKind::Key) {
            const auto target = f.fixtures.entries[i].cell;
            m.progressionSteps += walk(f, start, target, false, false);
            start = target;
        }
    m.progressionSteps += walk(f, start, f.guardianApproach, true, false);
    m.progressionSteps += walk(f, f.guardianApproach, f.fixtures.landings[1], true, true);
    return m;
}
} // namespace blobber::generation

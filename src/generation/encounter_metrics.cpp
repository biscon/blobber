#include "generation/floor.hpp"
#include <algorithm>
namespace blobber::generation {
namespace {
struct Walk {
    const FloorLayout& f;
    const Map& map;
    std::array<std::uint16_t, maxCells> parent{}, queue{};
    std::array<std::uint16_t, maxCells> path{};
    unsigned length = 0;
    bool find(Cell from, Cell to, bool key, const Connection* edge = nullptr) noexcept {
        parent.fill(UINT16_MAX);
        length = 0;
        auto index = [&](Cell c) { return unsigned(c.y * f.width + c.x); };
        const auto start = index(from), goal = index(to);
        if (!floorCell(f, from) || !floorCell(f, to))
            return false;
        unsigned head = 0, tail = 1;
        queue[0] = std::uint16_t(start);
        parent[start] = std::uint16_t(start);
        while (head < tail && parent[goal] == UINT16_MAX) {
            const auto n = queue[head++];
            const Cell here{int(n) % f.width, int(n) / f.width};
            for (unsigned h = 0; h < 4; ++h) {
                const auto c = here + forward(Heading(h));
                if (!floorCell(f, c) || map.movementBlocked(c))
                    continue;
                const auto next = index(c);
                if (parent[next] != UINT16_MAX)
                    continue;
                bool blocked = false;
                for (unsigned i = 0; i < f.fixtures.count; ++i) {
                    const auto& fixture = f.fixtures.entries[i];
                    if (fixture.cell == c &&
                        (stairFixture(fixture.kind) || fixture.kind == FixtureKind::GuardianGate ||
                         (fixture.kind == FixtureKind::Door && fixture.key && !key)))
                        blocked = true;
                }
                if (edge) {
                    const auto region = regionAt(f, c);
                    bool allowed = region == edge->a || region == edge->b;
                    if (region == noRegion)
                        for (unsigned i = 0; i < edge->count; ++i)
                            allowed |= c == pathCell(f, *edge, i);
                    blocked |= !allowed;
                }
                if (!blocked) {
                    parent[next] = std::uint16_t(n);
                    queue[tail++] = std::uint16_t(next);
                }
            }
        }
        if (parent[goal] == UINT16_MAX)
            return false;
        for (auto at = goal; at != start; at = parent[at])
            path[length++] = std::uint16_t(at);
        std::reverse(path.begin(), path.begin() + length);
        return true;
    }
};
struct Accumulator {
    const FloorLayout& f;
    unsigned seen = 0, steps = 0, interval = 0, first = 0, longest = 0;
    void step(Cell c) noexcept {
        ++steps;
        ++interval;
        const auto room = regionAt(f, c);
        for (unsigned i = 0; i < f.encounterCount; ++i)
            if (f.encounters[i].room == room && !(seen & (1u << i))) {
                if (!seen)
                    first = interval;
                longest = std::max(longest, interval);
                interval = 0;
                seen |= 1u << i;
            }
    }
    void add(const Walk& walk) noexcept {
        for (unsigned i = 0; i < walk.length; ++i) {
            const auto n = walk.path[i];
            step({int(n) % f.width, int(n) / f.width});
        }
    }
};
} // namespace
EncounterMetrics encounterMetrics(const FloorLayout& f, const Map& map) noexcept {
    EncounterMetrics out;
    if (!f.routeCount || f.routeCount > f.route.size() || f.roomCount > maxRooms ||
        f.encounterCount > f.encounters.size() || f.fixtures.count > fixtureCapacity)
        return out;
    Walk walk{f, map};
    Accumulator direct{f}, alternate{f};
    Cell key{};
    bool hasKey = f.depth == 1;
    for (unsigned i = 0; i < f.fixtures.count; ++i)
        if (f.fixtures.entries[i].kind == FixtureKind::Key)
            key = f.fixtures.entries[i].cell;
    for (unsigned i = 1; i < f.routeCount; ++i) {
        const auto a = f.route[i - 1], b = f.route[i];
        if (a >= f.roomCount || b >= f.roomCount)
            return {};
        hasKey |= f.rooms[a].anchor == key;
        const auto* edge = connectionBetween(f, a, b);
        if (!edge || !walk.find(f.rooms[a].anchor, f.rooms[b].anchor, hasKey, edge))
            return {};
        direct.add(walk);
    }
    Cell from = f.spawn.cell;
    if (f.depth > 1) {
        if (!walk.find(from, key, false))
            return {};
        alternate.add(walk);
        from = key;
    }
    if (!walk.find(from, f.rooms[f.route[f.routeCount - 1]].anchor, true))
        return {};
    alternate.add(walk);
    out.firstEncounterSteps = direct.first;
    out.longestEmptySteps = direct.longest;
    out.routeSteps = direct.steps;
    out.alternateFirstSteps = alternate.first;
    out.alternateLongestEmptySteps = alternate.longest;
    out.alternateSteps = alternate.steps;
    return out;
}
} // namespace blobber::generation

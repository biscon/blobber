#include "generation/encounter_planning.hpp"
#include "game/perception.hpp"
#include <algorithm>
#include <tuple>
namespace blobber::generation {
namespace {
constexpr unsigned states = maxRooms * 64;
constexpr std::uint16_t none = UINT16_MAX;
unsigned bits(unsigned n) noexcept {
    unsigned count = 0;
    for (; n; n &= n - 1)
        ++count;
    return count;
}
struct Planner {
    const FloorLayout& f;
    std::array<std::array<std::uint16_t, maxRooms>, maxRooms> edge{};
    std::array<std::uint16_t, states> distance{}, parent{}, heap{}, position{};
    unsigned count = 0, entry = maxRooms, approach = maxRooms, key = maxRooms, sector = maxRooms;
    explicit Planner(const FloorLayout& source) : f(source) {
        for (auto& row : edge)
            row.fill(none);
        sector = sectorJunction(f);
        // Exact anchor-to-anchor distance through each explicit passage and its two rooms.
        std::array<std::uint16_t, maxCells> queue{}, d{};
        for (unsigned i = 0; i < f.roomCount; ++i) {
            if (f.rooms[i].role == Role::Entrance)
                entry = i;
            if (f.rooms[i].role == Role::Approach)
                approach = i;
            for (unsigned j = 0; j < f.fixtures.count; ++j)
                if (f.fixtures.entries[j].kind == FixtureKind::Key &&
                    f.fixtures.entries[j].cell == f.rooms[i].anchor)
                    key = i;
        }
        for (unsigned e = 0; e < f.connectionCount; ++e) {
            const auto& c = f.connections[e];
            d.fill(none);
            unsigned head = 0, tail = 1;
            auto index = [&](Cell p) { return unsigned(p.y * f.width + p.x); };
            const auto start = index(f.rooms[c.a].anchor);
            queue[0] = std::uint16_t(start);
            d[start] = 0;
            while (head < tail) {
                const auto n = queue[head++];
                const Cell here{int(n) % f.width, int(n) / f.width};
                for (unsigned h = 0; h < 4; ++h) {
                    const auto p = here + forward(Heading(h));
                    if (!floorCell(f, p))
                        continue;
                    const auto region = regionAt(f, p);
                    bool allowed = region == c.a || region == c.b;
                    if (region == noRegion)
                        for (unsigned k = 0; k < c.count && !allowed; ++k)
                            allowed = p == pathCell(f, c, k);
                    const auto to = index(p);
                    if (allowed && d[to] == none) {
                        d[to] = std::uint16_t(d[n] + 1);
                        queue[tail++] = std::uint16_t(to);
                    }
                }
            }
            edge[c.a][c.b] = edge[c.b][c.a] = d[index(f.rooms[c.b].anchor)];
        }
    }
    bool less(unsigned a, unsigned b) const noexcept {
        return std::tie(distance[a], a) < std::tie(distance[b], b);
    }
    void swap(unsigned a, unsigned b) noexcept {
        std::swap(heap[a], heap[b]);
        position[heap[a]] = std::uint16_t(a);
        position[heap[b]] = std::uint16_t(b);
    }
    void push(unsigned state) noexcept {
        unsigned p = position[state];
        if (p == none) {
            p = count++;
            heap[p] = std::uint16_t(state);
            position[state] = std::uint16_t(p);
        }
        while (p && less(heap[p], heap[(p - 1) / 2])) {
            swap(p, (p - 1) / 2);
            p = (p - 1) / 2;
        }
    }
    unsigned pop() noexcept {
        const auto state = heap[0];
        position[state] = none;
        --count;
        if (count) {
            heap[0] = heap[count];
            position[heap[0]] = 0;
            unsigned p = 0;
            for (;;) {
                const auto a = p * 2 + 1, b = a + 1;
                if (a >= count)
                    break;
                const auto child = b < count && less(heap[b], heap[a]) ? b : a;
                if (!less(heap[child], heap[p]))
                    break;
                swap(p, child);
                p = child;
            }
        }
        return state;
    }
    bool route(unsigned selected, unsigned excluded, std::array<std::uint8_t, maxRooms * 2>& route,
               unsigned& length, unsigned& walk, unsigned& first, unsigned& gap) noexcept {
        std::array<unsigned, maxRooms> flags{};
        unsigned bit = 1;
        for (unsigned r = 0; r < f.roomCount; ++r)
            if ((selected & (1u << r)) || r == key) {
                flags[r] = bit;
                bit <<= 1;
            }
        if (bit > 64)
            return false;
        const unsigned complete = bit - 1;
        distance.fill(none);
        parent.fill(none);
        position.fill(none);
        count = 0;
        const auto start = entry + flags[entry] * maxRooms;
        distance[start] = 0;
        parent[start] = std::uint16_t(start);
        push(start);
        unsigned goal = states;
        while (count) {
            const auto state = pop(), here = state % maxRooms, mask = state / maxRooms;
            if (here == approach && mask == complete) {
                goal = state;
                break;
            }
            for (unsigned next = 0; next < f.roomCount; ++next) {
                if (edge[here][next] == none || (excluded & (1u << next)) ||
                    f.rooms[next].role == Role::Objective || f.rooms[next].role == Role::Optional)
                    continue;
                if (key < maxRooms && !(mask & flags[key]) && next == sector)
                    continue;
                if (next == approach && mask != complete)
                    continue;
                const auto to = next + (mask | flags[next]) * maxRooms;
                const auto cost = unsigned(distance[state]) + edge[here][next];
                if (cost < distance[to]) {
                    distance[to] = std::uint16_t(cost);
                    parent[to] = std::uint16_t(state);
                    push(to);
                }
            }
        }
        if (goal == states)
            return false;
        length = 0;
        auto at = goal;
        do {
            if (length == route.size())
                return false;
            route[length++] = std::uint8_t(at % maxRooms);
            if (at == start)
                break;
            at = parent[at];
        } while (true);
        std::reverse(route.begin(), route.begin() + length);
        walk = distance[goal];
        first = gap = 0;
        unsigned interval = 0, visited = 0;
        for (unsigned i = 1; i < length; ++i) {
            interval += edge[route[i - 1]][route[i]];
            const auto r = route[i];
            if (((selected & (1u << r)) && !(visited & (1u << r))) || r == approach) {
                if (!visited)
                    first = interval;
                gap = std::max(gap, interval);
                interval = 0;
                visited |= 1u << r;
            }
        }
        return true;
    }
};
} // namespace
bool planEncounterSites(FloorLayout& f, const DepthBand& band, std::array<unsigned, 7>& rooms) noexcept {
    Planner p(f);
    if (p.entry == maxRooms || p.approach == maxRooms)
        return false;
    unsigned junctions = 0;
    std::array<unsigned, 8> ids{};
    for (unsigned r = 0; r < f.roomCount; ++r)
        if (f.rooms[r].role == Role::Junction) {
            if (junctions == ids.size())
                return false;
            ids[junctions++] = r;
        }
    bool found = false;
    std::tuple<unsigned, unsigned, unsigned, unsigned, unsigned, unsigned> best{};
    for (unsigned mask = 0; mask < (1u << junctions); ++mask) {
        if (bits(mask) != band.count)
            continue;
        unsigned selected = 0;
        for (unsigned i = 0; i < junctions; ++i)
            if (mask & (1u << i))
                selected |= 1u << ids[i];
        for (unsigned leaf = 0; leaf < f.roomCount; ++leaf) {
            if (f.rooms[leaf].role != Role::Optional)
                continue;
            for (unsigned second = 0; second < (band.optionalCount == 1 ? 1u : f.roomCount); ++second) {
                if (band.optionalCount == 2 &&
                    (second == leaf || second == p.key || second == p.sector || (selected & (1u << second)) ||
                     (f.rooms[second].role != Role::Optional && f.rooms[second].role != Role::Junction)))
                    continue;
                const auto excluded = (1u << leaf) | (band.optionalCount == 2 ? 1u << second : 0);
                std::array<std::uint8_t, maxRooms * 2> route{};
                unsigned length, walk, first, gap;
                if (!p.route(selected, excluded, route, length, walk, first, gap))
                    continue;
                const auto score = std::make_tuple(first, gap, walk, selected, leaf, second);
                if (found && !(score < best))
                    continue;
                found = true;
                best = score;
                f.route = route;
                f.routeCount = length;
                unsigned count = 0, visited = 0;
                for (unsigned i = 1; i < length; ++i) {
                    const auto room = route[i];
                    if ((selected & (1u << room)) && !(visited & (1u << room))) {
                        rooms[count++] = room;
                        visited |= 1u << room;
                    }
                }
                rooms[count++] = leaf;
                if (band.optionalCount == 2)
                    rooms[count++] = second;
            }
        }
    }
    return found;
}

bool placeEncounterFormation(FloorLayout& f, unsigned room, EncounterRecipe recipe, EncounterRole role,
                             Heading facing) noexcept {
    const auto& r = encounterRecipe(recipe);
    if (f.encounterCount == f.encounters.size() || f.populationCount + r.count > f.population.size())
        return false;
    const auto front = forward(facing), side = right(facing), anchor = f.rooms[room].anchor;
    // Compact, connected formation: at most two melee bodies per cell, separate supports.
    std::array<Cell, 6> cells{};
    const auto legal = [&](Cell c) {
        if (!floorCell(f, c) || regionAt(f, c) != room ||
            f.fixtures.npcExcluded[unsigned(c.y * f.width + c.x)])
            return false;
        for (unsigned i = 0; i < f.fixtures.count; ++i)
            if (c == f.fixtures.entries[i].cell)
                return false;
        // Never occupy the square directly inside a room port.
        for (unsigned i = 0; i < f.connectionCount; ++i) {
            const auto& e = f.connections[i];
            if ((e.a == room && c == e.socketA + forward(e.facingA) * -1) ||
                (e.b == room && c == e.socketB + forward(e.facingB) * -1))
                return false;
        }
        if (!gridSight([&](Cell p) { return !floorCell(f, p); }, anchor, c))
            return false;
        if (role == EncounterRole::Guardian &&
            !gridSight([&](Cell p) { return !floorCell(f, p); }, f.guardianApproach, c))
            return false;
        return true;
    };
    for (unsigned n = 0; n < r.count; ++n) {
        const bool ranged = r.members[n] == ActorRecipe::Archer || r.members[n] == ActorRecipe::Shaman;
        bool placed = false;
        std::tuple<int, int, int, int> best{};
        for (int depth = 0; depth <= 1; ++depth)
            for (int lateral = -1; lateral <= 1; ++lateral) {
                const auto c = anchor + front * depth + side * lateral;
                if (!legal(c))
                    continue;
                unsigned occupancy = 0;
                bool support = false;
                for (unsigned j = 0; j < n; ++j)
                    if (cells[j] == c) {
                        ++occupancy;
                        support |= r.members[j] == ActorRecipe::Archer || r.members[j] == ActorRecipe::Shaman;
                    }
                if (occupancy >= 2 || (ranged && occupancy) || support)
                    continue;
                // Two rows keep every member within the existing radius-three alert.
                // Melee shares the front before consuming support positions.
                const auto score =
                    std::make_tuple(ranged ? depth : -depth, int(occupancy), std::abs(lateral), lateral);
                if (!placed || score < best) {
                    best = score;
                    cells[n] = c;
                    placed = true;
                }
            }
        if (!placed)
            return false;
    }
    f.encounters[f.encounterCount++] = {recipe, role, std::uint8_t(room), std::uint8_t(f.populationCount),
                                        std::uint8_t(r.count)};
    for (unsigned n = 0; n < r.count; ++n) {
        unsigned occupancy = 0;
        for (unsigned j = 0; j < n; ++j)
            occupancy += cells[j] == cells[n];
        const auto i = f.populationCount++;
        f.population[i] = {cells[n],        rotate({int(occupancy), 0}, 2, unsigned(facing)),
                           facing,          r.members[n],
                           std::uint8_t(i), role == EncounterRole::Guardian};
    }
    return true;
}
} // namespace blobber::generation

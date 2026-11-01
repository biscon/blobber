#include "generation/stone_layout.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>
namespace blobber::generation {
namespace {
struct Random {
    std::uint64_t state;
    unsigned next(unsigned bound) noexcept {
        state += 0x9e3779b97f4a7c15ull;
        auto z = state;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
        z ^= z >> 31;
        return unsigned((std::uint64_t(std::uint32_t(z >> 32)) * bound) >> 32);
    }
};
int distance(Cell a, Cell b) noexcept {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}
Heading opposite(Heading h) noexcept {
    return Heading((unsigned(h) + 2) % 4);
}
struct Socket {
    Cell cell;
    Heading facing;
};
struct Sockets {
    std::array<Socket, 160> values{};
    unsigned count = 0;
};
Sockets sockets(const FloorLayout& f, unsigned room) noexcept {
    Sockets result;
    const auto& r = f.rooms[room];
    for (int y = r.origin.y; y < r.origin.y + r.size.y; ++y)
        for (int x = r.origin.x; x < r.origin.x + r.size.x; ++x) {
            const Cell c{x, y};
            if (regionAt(f, c) != room)
                continue;
            for (unsigned d = 0; d < 4; ++d) {
                const auto h = Heading(d);
                if (r.shape == 4 &&
                    !(c == r.origin + rotate({4, 4}, 5, r.rotation) && h == Heading((1 + r.rotation) % 4)))
                    continue;
                const auto p = c + forward(h);
                if (p.x < 1 || p.y < 1 || p.x >= f.width - 1 || p.y >= f.height - 1 || floorCell(f, p))
                    continue;
                unsigned adjacent = 0;
                for (unsigned k = 0; k < 4; ++k)
                    adjacent += floorCell(f, p + forward(Heading(k)));
                if (adjacent == 1 && result.count < result.values.size())
                    result.values[result.count++] = {p, h};
            }
        }
    return result;
}
struct Shape {
    int width = 0, height = 0;
    std::array<std::uint8_t, 81> cells{};
    Room room{};
};
Shape shape(unsigned index, Role role, Random& rng, Heading port) noexcept {
    Shape s;
    auto& r = s.room;
    r.role = role;
    r.shape = std::uint8_t(index == 0 ? 3 : index == 1 ? 2 : index == 3 ? 1 : rng.next(4));
    if (role == Role::Approach)
        r.shape = 0;
    if (role == Role::Entrance || role == Role::Objective)
        r.shape = 4;
    if (r.shape == 4) {
        s.width = s.height = 5;
        r.rotation = std::uint8_t((unsigned(port) + 3) % 4);
        for (int y = 0; y < 5; ++y)
            for (int x = 0; x < 5; ++x) {
                const auto p = rotate({x, y}, 5, r.rotation);
                s.cells[unsigned(p.y * 9 + p.x)] =
                    std::uint8_t((x < 3 && y < 3) || (x == 0 && y == 3) || y == 4);
            }
        r.anchor = rotate({1, 1}, 5, r.rotation);
    } else {
        if (index == 0) {
            s.width = 3;
            s.height = 7 + int(rng.next(3));
        } else if (index == 1) {
            s.width = 5 + int(rng.next(2));
            s.height = 6 + int(rng.next(2));
        } else if (index == 2) {
            s.width = 3;
            s.height = 4;
            r.shape = 0;
        } else if (index == 4) {
            s.width = 4;
            s.height = 5;
            r.shape = 0;
        } else if (r.shape == 3) {
            s.width = 3;
            s.height = 6 + int(rng.next(4));
        } else if (r.shape == 1 || r.shape == 2) {
            s.width = 5;
            s.height = 5 + int(rng.next(3));
        } else {
            s.width = 3 + int(rng.next(4));
            s.height = 4 + int(rng.next(3));
        }
        if (role == Role::Approach) {
            s.width = 5;
            s.height = 5 + int(rng.next(2));
        }
        for (int y = 0; y < s.height; ++y)
            for (int x = 0; x < s.width; ++x) {
                bool open = true;
                if (r.shape == 2 && x >= 3 && y < s.height - 3)
                    open = false;
                if (r.shape == 1 && x == 2 && y == 2)
                    open = false;
                s.cells[unsigned(y * 9 + x)] = std::uint8_t(open);
            }
        if (rng.next(2)) {
            auto old = s.cells;
            s.cells.fill(0);
            for (int y = 0; y < s.height; ++y)
                for (int x = 0; x < s.width; ++x)
                    s.cells[unsigned(x * 9 + y)] = old[unsigned(y * 9 + x)];
            std::swap(s.width, s.height);
        }
        // Find the closest clear anchor to the geometric centre; pillars may need zero clearance.
        int best = 999;
        r.clearance = 0;
        for (int clearance = 1; clearance >= 0; --clearance) {
            for (int y = 0; y < s.height; ++y)
                for (int x = 0; x < s.width; ++x) {
                    bool valid = true;
                    for (int dy = -clearance; dy <= clearance; ++dy)
                        for (int dx = -clearance; dx <= clearance; ++dx)
                            if (x + dx < 0 || y + dy < 0 || x + dx >= s.width || y + dy >= s.height ||
                                !s.cells[unsigned((y + dy) * 9 + x + dx)])
                                valid = false;
                    const int score = std::abs(2 * x - s.width + 1) + std::abs(2 * y - s.height + 1);
                    if (valid && score < best) {
                        best = score;
                        r.anchor = {x, y};
                        r.clearance = std::uint8_t(clearance);
                    }
                }
            if (best < 999)
                break;
        }
    }
    r.size = {s.width, s.height};
    r.facing = port;
    return s;
}
bool stamp(FloorLayout& f, Shape& s, Cell origin) noexcept {
    if (origin.x < 1 || origin.y < 1 || origin.x + s.width >= f.width || origin.y + s.height >= f.height)
        return false;
    for (int y = 0; y < s.height; ++y)
        for (int x = 0; x < s.width; ++x)
            if (s.cells[unsigned(y * 9 + x)]) {
                const auto c = origin + Cell{x, y};
                if (floorCell(f, c))
                    return false;
                for (unsigned h = 0; h < 4; ++h)
                    if (floorCell(f, c + forward(Heading(h))))
                        return false;
            }
    auto r = s.room;
    r.id = std::uint8_t(f.roomCount);
    r.origin = origin;
    r.anchor = r.anchor + origin;
    if (r.role == Role::Entrance || r.role == Role::Objective)
        r.facing = Heading((2 + r.rotation) % 4);
    f.rooms[f.roomCount] = r;
    for (int y = 0; y < s.height; ++y)
        for (int x = 0; x < s.width; ++x)
            if (s.cells[unsigned(y * 9 + x)]) {
                const auto i = unsigned((origin.y + y) * f.width + origin.x + x);
                f.floor[i] = 1;
                f.membership[i] = std::uint8_t(f.roomCount);
            }
    ++f.roomCount;
    return true;
}
void unstamp(FloorLayout& f) noexcept {
    --f.roomCount;
    for (unsigned i = 0; i < maxCells; ++i)
        if (f.membership[i] == f.roomCount) {
            f.floor[i] = 0;
            f.membership[i] = noRegion;
        }
    f.rooms[f.roomCount] = {};
}
// An indexed heap bounds both memory and work; one state per (cell, incoming heading).
struct Router {
    static constexpr unsigned states = maxCells * 4;
    static constexpr std::uint16_t absent = UINT16_MAX;
    std::array<std::uint32_t, states> cost{};
    std::array<std::uint16_t, states> parent{}, heap{}, position{};
    std::array<std::uint8_t, maxCells> allowed{};
    unsigned count = 0;
    const FloorLayout* floor = nullptr;
    Cell goal{};
    unsigned priority(unsigned state) const noexcept {
        const auto n = state / 4;
        return cost[state] + unsigned(10 * distance({int(n) % floor->width, int(n) / floor->width}, goal));
    }
    bool less(unsigned a, unsigned b) const noexcept {
        const auto x = priority(a), y = priority(b);
        return x < y || (x == y && a < b);
    }
    void swap(unsigned a, unsigned b) noexcept {
        std::swap(heap[a], heap[b]);
        position[heap[a]] = std::uint16_t(a);
        position[heap[b]] = std::uint16_t(b);
    }
    void push(unsigned state) noexcept {
        unsigned at = position[state];
        if (at == absent) {
            at = count++;
            heap[at] = std::uint16_t(state);
            position[state] = std::uint16_t(at);
        }
        while (at && less(heap[at], heap[(at - 1) / 2])) {
            swap(at, (at - 1) / 2);
            at = (at - 1) / 2;
        }
    }
    unsigned pop() noexcept {
        const auto state = heap[0];
        swap(0, --count);
        position[state] = absent;
        unsigned at = 0;
        while (at * 2 + 1 < count) {
            unsigned child = at * 2 + 1;
            if (child + 1 < count && less(heap[child + 1], heap[child]))
                ++child;
            if (!less(heap[child], heap[at]))
                break;
            swap(at, child);
            at = child;
        }
        return state;
    }
    bool route(const FloorLayout& f, Socket a, Socket b, std::array<std::uint16_t, maxCells>& path,
               unsigned& length) noexcept {
        floor = &f;
        goal = b.cell;
        count = 0;
        length = 0;
        cost.fill(UINT32_MAX / 2);
        parent.fill(absent);
        position.fill(absent);
        allowed.fill(0);
        for (int y = 1; y < f.height - 1; ++y)
            for (int x = 1; x < f.width - 1; ++x) {
                const Cell c{x, y};
                if (floorCell(f, c))
                    continue;
                bool valid = true;
                for (unsigned h = 0; h < 4; ++h) {
                    const auto n = c + forward(Heading(h));
                    if (floorCell(f, n) && !(c == a.cell && n == a.cell + forward(a.facing) * -1) &&
                        !(c == b.cell && n == b.cell + forward(b.facing) * -1))
                        valid = false;
                }
                allowed[unsigned(y * f.width + x)] = std::uint8_t(valid);
            }
        if (!inside(f, a.cell) || !inside(f, b.cell) || !allowed[unsigned(a.cell.y * f.width + a.cell.x)] ||
            !allowed[unsigned(b.cell.y * f.width + b.cell.x)])
            return false;
        // Straight and one-bend monotone paths have minimum length and avoid a heap search.
        auto dogleg = [&](bool horizontalFirst) {
            length = 0;
            Cell c = a.cell;
            path[length++] = std::uint16_t(c.y * f.width + c.x);
            for (unsigned leg = 0; leg < 2; ++leg) {
                const bool horizontal = (leg == 0) == horizontalFirst;
                int& coordinate = horizontal ? c.x : c.y;
                const int target = horizontal ? b.cell.x : b.cell.y;
                while (coordinate != target) {
                    coordinate += coordinate < target ? 1 : -1;
                    const auto n = unsigned(c.y * f.width + c.x);
                    if (!allowed[n])
                        return false;
                    path[length++] = std::uint16_t(n);
                }
            }
            if (length > 1) {
                const Cell first{path[1] % f.width, path[1] / f.width};
                const Cell last{path[length - 2] % f.width, path[length - 2] / f.width};
                if (!(first == a.cell + forward(a.facing)) || !(last == b.cell + forward(b.facing)))
                    return false;
            }
            return true;
        };
        if (dogleg(true) || dogleg(false))
            return true;
        length = 0;
        const unsigned start = unsigned(a.cell.y * f.width + a.cell.x) * 4 + unsigned(a.facing);
        cost[start] = 0;
        push(start);
        while (count) {
            const auto state = pop();
            const auto n = state / 4;
            const Cell here{int(n) % f.width, int(n) / f.width};
            if (here == b.cell) {
                for (auto v = state;; v = parent[v]) {
                    if (length == maxCells)
                        return false;
                    path[length++] = std::uint16_t(v / 4);
                    if (v == start)
                        break;
                }
                std::reverse(path.begin(), path.begin() + length);
                return true;
            }
            for (unsigned h = 0; h < 4; ++h) {
                if (here == a.cell && h != unsigned(a.facing))
                    continue;
                const auto next = here + forward(Heading(h));
                if (!inside(f, next))
                    continue;
                const auto ni = unsigned(next.y * f.width + next.x);
                if (!allowed[ni] || (next == b.cell && h != unsigned(opposite(b.facing))))
                    continue;
                const unsigned ns = ni * 4 + h, value = cost[state] + 10 + (state % 4 == h ? 0u : 3u);
                if (value < cost[ns]) {
                    cost[ns] = value;
                    parent[ns] = std::uint16_t(state);
                    push(ns);
                }
            }
        }
        return false;
    }
};
static_assert(sizeof(Router) + sizeof(FloorLayout) * 2 + fixtureScratchBytes + maxCells * 6 < 128 * 1024,
              "Bounded preparation scratch");
unsigned walk(const FloorLayout& f, Cell a, Cell b) noexcept {
    std::array<std::uint16_t, maxCells> q{}, d{};
    d.fill(UINT16_MAX);
    unsigned head = 0, tail = 1;
    const auto first = unsigned(a.y * f.width + a.x);
    q[0] = std::uint16_t(first);
    d[first] = 0;
    while (head < tail) {
        const auto i = q[head++];
        const Cell c{int(i) % f.width, int(i) / f.width};
        if (c == b)
            return d[i];
        for (unsigned h = 0; h < 4; ++h) {
            const auto n = c + forward(Heading(h));
            if (!floorCell(f, n))
                continue;
            const auto j = unsigned(n.y * f.width + n.x);
            if (d[j] != UINT16_MAX)
                continue;
            d[j] = std::uint16_t(d[i] + 1);
            q[tail++] = std::uint16_t(j);
        }
    }
    return UINT16_MAX;
}
bool connect(FloorLayout& f, unsigned a, unsigned b, Socket sa, Socket sb, Router& router, bool loop = false,
             unsigned minLength = 1) noexcept {
    if (f.connectionCount == maxConnections || connectionBetween(f, a, b))
        return false;
    std::array<std::uint16_t, maxCells> path{};
    unsigned length = 0;
    if (!router.route(f, sa, sb, path, length) || length < minLength || length > 20 ||
        f.pathCount + length > maxCells)
        return false;
    if (loop && walk(f, sa.cell + forward(sa.facing) * -1, sb.cell + forward(sb.facing) * -1) < length + 9)
        return false;
    auto& e = f.connections[f.connectionCount++];
    e = {std::uint8_t(a),
         std::uint8_t(b),
         sa.cell,
         sb.cell,
         sa.facing,
         sb.facing,
         std::uint16_t(f.pathCount),
         std::uint16_t(length)};
    for (unsigned i = 0; i < length; ++i) {
        f.paths[f.pathCount++] = path[i];
        f.floor[path[i]] = 1;
    }
    return true;
}
bool attach(FloorLayout& f, unsigned parent, Role role, Random& rng, Router& router,
            GenerationRequest request, unsigned attempt, unsigned limit = 32) noexcept {
    const auto ports = sockets(f, parent);
    if (!ports.count)
        return false;
    for (unsigned proposal = 0; proposal < limit; ++proposal) {
        const auto a = ports.values[rng.next(ports.count)];
        Random interior{seedFor(request, Domain::Interior, f.roomCount, attempt)};
        auto s = shape(f.roomCount, role, interior, opposite(a.facing));
        unsigned shortChance = f.profile == LayoutProfile::CompactWings ? 75 : 25;
        unsigned gap = rng.next(100) < shortChance ? 1 : 3 + rng.next(6);
        if (role == Role::Objective)
            gap = 3 + rng.next(3);
        const int offset = gap == 1 ? 0 : int(rng.next(7)) - 3;
        // Candidate near-side interior cells; the destination socket faces back at its parent.
        std::array<Cell, 81> candidates{};
        unsigned n = 0;
        for (int y = 0; y < s.height; ++y)
            for (int x = 0; x < s.width; ++x) {
                const Cell c{x, y};
                const auto p = c + forward(s.room.facing);
                if (!s.cells[unsigned(y * 9 + x)])
                    continue;
                if (s.room.shape == 4 && !(c == rotate({4, 4}, 5, s.room.rotation)))
                    continue;
                if (p.x >= 0 && p.y >= 0 && p.x < s.width && p.y < s.height &&
                    s.cells[unsigned(p.y * 9 + p.x)])
                    continue;
                candidates[n++] = c;
            }
        if (!n)
            continue;
        const auto local = candidates[rng.next(n)];
        const auto target = a.cell + forward(a.facing) * int(gap) + right(a.facing) * offset;
        const auto origin = target + local * -1;
        const auto id = f.roomCount;
        if (!stamp(f, s, origin))
            continue;
        const Socket b{target + forward(s.room.facing), s.room.facing};
        if (connect(f, parent, id, a, b, router, false, role == Role::Objective ? 3u : 1u))
            return true;
        unstamp(f);
    }
    return false;
}
bool addLoop(FloorLayout& f, Random& rng, Router& router) noexcept {
    for (unsigned attempt = 0; attempt < 96; ++attempt) {
        unsigned a = rng.next(f.roomCount), b = rng.next(f.roomCount);
        if (a == b || connectionBetween(f, a, b))
            continue;
        const auto sa = sockets(f, a), sb = sockets(f, b);
        if (!sa.count || !sb.count)
            continue;
        unsigned best = 999;
        Socket x{}, y{};
        for (unsigned i = 0; i < sa.count; ++i)
            for (unsigned j = 0; j < sb.count; ++j) {
                const unsigned score = unsigned(distance(sa.values[i].cell, sb.values[j].cell));
                if (score < best) {
                    best = score;
                    x = sa.values[i];
                    y = sb.values[j];
                }
            }
        if (connect(f, a, b, x, y, router, true))
            return true;
    }
    return false;
}
bool upstreamConnected(const FloorLayout& f, unsigned omit) noexcept {
    std::array<bool, maxRooms> seen{};
    unsigned first = omit == 0 ? 1 : 0;
    seen[first] = true;
    for (unsigned pass = 0; pass < f.roomCount; ++pass)
        for (unsigned i = 0; i < f.connectionCount; ++i) {
            const auto& e = f.connections[i];
            if (e.a == omit || e.b == omit)
                continue;
            if (seen[e.a] || seen[e.b])
                seen[e.a] = seen[e.b] = true;
        }
    for (unsigned i = 0; i < f.roomCount; ++i)
        if (i != omit && !seen[i])
            return false;
    return true;
}
bool build(FloorLayout& f, GenerationRequest request, unsigned attempt) noexcept {
    Random rng{seedFor(request, Domain::Topology, 0, attempt)};
    Router router;
    Random interior{seedFor(request, Domain::Interior, 0, attempt)};
    auto initial = shape(0, Role::Junction, interior, Heading::East);
    if (!stamp(f, initial, {13 + int(rng.next(6)), 12 + int(rng.next(5))}))
        return false;
    const unsigned junctions = 6 + rng.next(3);
    for (unsigned i = 1; i < junctions; ++i) {
        bool placed = false;
        for (unsigned parent = 0; parent < 4 && !placed; ++parent)
            placed = attach(f, i < 3 ? 0 : rng.next(f.roomCount), Role::Junction, rng, router, request,
                            attempt, 8);
        if (!placed)
            return false;
    }
    if (!addLoop(f, rng, router))
        return false;
    f.loopConnection = f.connectionCount - 1;
    if (!addLoop(f, rng, router))
        return false;
    std::array<unsigned, maxRooms> degree{};
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        ++degree[f.connections[i].a];
        ++degree[f.connections[i].b];
    }
    std::array<unsigned, maxRooms> sectors{};
    unsigned count = 0;
    for (unsigned i = 0; i < junctions; ++i)
        if (degree[i] == 2 && upstreamConnected(f, i))
            sectors[count++] = i;
    if (!count)
        return false;
    const unsigned sector = sectors[rng.next(count)];
    // Attach terminal safety regions before optional leaves consume their available walls.
    const auto approach = f.roomCount;
    if (!attach(f, sector, Role::Approach, rng, router, request, attempt) ||
        !attach(f, approach, Role::Objective, rng, router, request, attempt))
        return false;
    std::array<unsigned, maxRooms> entranceParents{};
    for (unsigned i = 0; i < junctions; ++i)
        entranceParents[i] = i;
    if (request.depth == 1) {
        // Reserve roughly half the depth-1 walk for the stair nooks and terminal connection.
        constexpr int targetToGuardian = 28;
        std::array<unsigned, maxRooms> scores{};
        for (unsigned i = 0; i < junctions; ++i)
            scores[i] = unsigned(
                std::abs(int(walk(f, f.rooms[i].anchor, f.rooms[approach].anchor)) - targetToGuardian));
        std::sort(entranceParents.begin(), entranceParents.begin() + junctions, [&](unsigned a, unsigned b) {
            return scores[a] < scores[b] || (scores[a] == scores[b] && a < b);
        });
    }
    auto leaf = [&](Role role) {
        for (unsigned i = 0; i < junctions * 2; ++i) {
            const auto parent =
                role == Role::Entrance && request.depth == 1 ? entranceParents[i / 2] : rng.next(junctions);
            if (parent != sector && attach(f, parent, role, rng, router, request, attempt, 2))
                return true;
        }
        return false;
    };
    if (!leaf(Role::Entrance) || !leaf(Role::Optional))
        return false;
    if (junctions < 8 && rng.next(2) && !leaf(Role::Optional))
        return false;
    for (unsigned i = 0; i < f.roomCount; ++i)
        if (f.rooms[i].role == Role::Entrance)
            f.spawn = {f.rooms[i].anchor, f.rooms[i].facing};
    return true;
}
} // namespace
bool buildStoneLayout(FloorLayout& f, GenerationRequest request, unsigned attempt, bool fallback) noexcept {
    const auto& profiles = stoneBiome().profiles;
    f.profile = profiles[unsigned(seedFor(request, Domain::Topology, 77)) % profiles.size()];
    f.family = fallback ? 1u : 0u;
    if (fallback)
        return buildFallback(f);
    return build(f, request, attempt);
}
} // namespace blobber::generation

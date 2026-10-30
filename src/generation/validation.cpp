#include "generation/floor.hpp"
#include <algorithm>
#include <cstdlib>
namespace blobber::generation {
namespace {
struct Scratch {
    std::array<std::uint8_t, maxCells> allowed{};
    std::array<std::int16_t, maxCells> distance{};
    std::array<std::uint16_t, maxCells> queue{};
};
bool inside(const FloorLayout& f, Cell c) noexcept {
    return c.x >= 0 && c.y >= 0 && c.x < f.width && c.y < f.height;
}
bool open(const FloorLayout& f, Cell c) noexcept {
    return inside(f, c) && f.floor[std::size_t(c.y * f.width + c.x)] == 1;
}
Cell center(const Room& r) noexcept {
    return {4 + 9 * r.site.x, 4 + 9 * r.site.y};
}
unsigned search(const FloorLayout& f, Scratch& s, Cell start, Cell blocked = {-1, -1}) noexcept {
    s.distance.fill(-1);
    if (!open(f, start) || start == blocked)
        return 0;
    unsigned head = 0, tail = 1;
    const auto index = unsigned(start.y * f.width + start.x);
    s.queue[0] = std::uint16_t(index);
    s.distance[index] = 0;
    while (head < tail) {
        const auto i = s.queue[head++];
        const Cell here{int(i) % f.width, int(i) / f.width};
        for (unsigned d = 0; d < 4; ++d) {
            const auto next = here + forward(static_cast<Heading>(d));
            if (!open(f, next) || next == blocked)
                continue;
            const auto n = unsigned(next.y * f.width + next.x);
            if (s.distance[n] >= 0)
                continue;
            s.distance[n] = std::int16_t(s.distance[i] + 1);
            s.queue[tail++] = std::uint16_t(n);
        }
    }
    return tail;
}
} // namespace
std::size_t scratchBytes() noexcept {
    return sizeof(Scratch) + 2 * sizeof(FloorLayout) + fixtureScratchBytes;
}
Failure validate(const FloorLayout& f) noexcept {
    if (f.width < 3 || f.height < 3 || f.width > 40 || f.height > 40 || f.roomCount > maxRooms ||
        f.connectionCount > maxConnections)
        return {Error::Capacity};
    if (f.width != floorDimension || f.height != floorDimension || f.roomCount < 8 || f.roomCount > 12 ||
        !f.connectionCount || f.loopConnection >= f.connectionCount || f.family > 1 || f.rotation > 3)
        return {Error::Bounds};
    Scratch s;
    std::array<unsigned, maxRooms> degree{}, ports{};
    std::array<unsigned, 5> roles{};
    int entrance = -1, objective = -1, approach = -1;
    for (unsigned i = 0; i < f.roomCount; ++i) {
        const auto& r = f.rooms[i];
        if (r.site.x < 0 || r.site.x > 3 || r.site.y < 0 || r.site.y > 3 || r.id >= maxRooms ||
            r.shape >= templates().size() || r.rotation > 3 || r.ports > 15 ||
            unsigned(r.role) >= roles.size() || unsigned(r.facing) > 3)
            return {Error::Room, int(i)};
        for (unsigned j = 0; j < i; ++j)
            if (r.site == f.rooms[j].site || r.id == f.rooms[j].id)
                return {Error::Room, int(i)};
        ++roles[unsigned(r.role)];
        if (r.role == Role::Entrance)
            entrance = int(i);
        if (r.role == Role::Objective)
            objective = int(i);
        if (r.role == Role::Approach)
            approach = int(i);
        const auto c = center(r);
        if (!open(f, r.anchor) || std::abs(r.anchor.x - c.x) > 2 || std::abs(r.anchor.y - c.y) > 2)
            return {Error::Anchor, int(i)};
        if (r.role == Role::Entrance || r.role == Role::Approach || r.role == Role::Objective)
            for (int y = -1; y <= 1; ++y)
                for (int x = -1; x <= 1; ++x)
                    if (!open(f, r.anchor + Cell{x, y}))
                        return {Error::Anchor, int(i)};
        for (int y = -2; y <= 2; ++y)
            for (int x = -2; x <= 2; ++x) {
                const auto p = c + Cell{x, y};
                s.allowed[p.y * f.width + p.x] = 1;
            }
    }
    if (roles[unsigned(Role::Entrance)] != 1 || roles[unsigned(Role::Approach)] != 1 ||
        roles[unsigned(Role::Objective)] != 1 || roles[unsigned(Role::Optional)] < 1 ||
        roles[unsigned(Role::Optional)] > 2)
        return {Error::Room};
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto e = f.connections[i];
        if (e.a >= f.roomCount || e.b >= f.roomCount || e.a == e.b)
            return {Error::Connection, -1, int(i)};
        for (unsigned j = 0; j < i; ++j) {
            const auto other = f.connections[j];
            if ((e.a == other.a && e.b == other.b) || (e.a == other.b && e.b == other.a))
                return {Error::Connection, -1, int(i)};
        }
        const auto& a = f.rooms[e.a];
        const auto& b = f.rooms[e.b];
        int direction = -1;
        for (unsigned d = 0; d < 4; ++d)
            if (a.site + forward(static_cast<Heading>(d)) == b.site)
                direction = int(d);
        if (direction < 0)
            return {Error::Connection, -1, int(i)};
        ++degree[e.a];
        ++degree[e.b];
        ports[e.a] |= 1u << direction;
        ports[e.b] |= 1u << ((direction + 2) % 4);
        const auto delta = forward(static_cast<Heading>(direction));
        for (int step = 3; step <= 6; ++step) {
            const auto p = center(a) + delta * step;
            if (!open(f, p))
                return {Error::Port, -1, int(i)};
            s.allowed[p.y * f.width + p.x] = 1;
        }
        if (a.role == Role::Objective && b.role != Role::Approach)
            return {Error::Connection, int(e.a), int(i)};
        if (b.role == Role::Objective && a.role != Role::Approach)
            return {Error::Connection, int(e.b), int(i)};
        if ((a.role == Role::Optional && (b.role == Role::Approach || b.role == Role::Objective)) ||
            (b.role == Role::Optional && (a.role == Role::Approach || a.role == Role::Objective)))
            return {Error::Branch, -1, int(i)};
    }
    for (unsigned i = 0; i < f.roomCount; ++i) {
        if (ports[i] != f.rooms[i].ports)
            return {Error::Port, int(i)};
        if (f.rooms[i].role == Role::Optional && degree[i] != 1)
            return {Error::Branch, int(i)};
    }
    if (degree[unsigned(entrance)] != 1 || degree[unsigned(objective)] != 1 ||
        degree[unsigned(approach)] != 2)
        return {Error::Connection};
    if (!(f.spawn == Player{f.rooms[unsigned(entrance)].anchor, f.rooms[unsigned(entrance)].facing}) ||
        !(ports[unsigned(entrance)] & (1u << unsigned(f.spawn.heading))))
        return {Error::Anchor, entrance};
    unsigned floors = 0;
    for (int y = 0; y < f.height; ++y)
        for (int x = 0; x < f.width; ++x) {
            const auto i = unsigned(y * f.width + x);
            if (f.floor[i] > 1)
                return {Error::Bounds};
            if (!f.floor[i])
                continue;
            if (x == 0 || y == 0 || x == f.width - 1 || y == f.height - 1)
                return {Error::Bounds};
            if (!s.allowed[i])
                return {Error::Port};
            ++floors;
        }
    if (search(f, s, f.spawn.cell) != floors)
        return {Error::Disconnected};
    const auto e = f.connections[f.loopConnection];
    const auto a = center(f.rooms[e.a]), b = center(f.rooms[e.b]);
    const Cell direction{(b.x - a.x) / 9, (b.y - a.y) / 9};
    const auto from = a + direction * 2, to = a + direction * 7;
    const auto blocked = a + direction * 4;
    search(f, s, from);
    const auto index = unsigned(to.y * f.width + to.x);
    const auto direct = s.distance[index];
    const auto reached = search(f, s, from, blocked);
    if (direct < 0 || reached != floors - 1 || s.distance[index] < direct + 8)
        return {Error::Loop, -1, int(f.loopConnection)};
    return validateFixtures(f);
}
static_assert(sizeof(Scratch) + 2 * sizeof(FloorLayout) + fixtureScratchBytes < 128 * 1024,
              "Generation scratch budget");
} // namespace blobber::generation

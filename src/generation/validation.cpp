#include "generation/floor.hpp"
#include <algorithm>
#include <cstdlib>
namespace blobber::generation {
namespace {
struct Scratch {
    std::array<std::uint8_t, maxCells> owner{};
    std::array<std::uint16_t, maxCells> step{}, queue{};
    std::array<std::int16_t, maxCells> distance{};
};
unsigned search(const FloorLayout& f, Scratch& s, Cell start, Cell blocked = {-1, -1}) noexcept {
    s.distance.fill(-1);
    if (!floorCell(f, start) || start == blocked)
        return 0;
    unsigned head = 0, tail = 1;
    const auto first = unsigned(start.y * f.width + start.x);
    s.queue[0] = std::uint16_t(first);
    s.distance[first] = 0;
    while (head < tail) {
        const auto i = s.queue[head++];
        const Cell c{int(i) % f.width, int(i) / f.width};
        for (unsigned h = 0; h < 4; ++h) {
            const auto next = c + forward(Heading(h));
            if (!floorCell(f, next) || next == blocked)
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
int manhattan(Cell a, Cell b) noexcept {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}
} // namespace
std::size_t scratchBytes() noexcept {
    // Includes the routing heap, two candidate values, progression queue and connection scratch.
    return maxCells * 4 * (sizeof(std::uint32_t) + 3 * sizeof(std::uint16_t)) + maxCells +
           sizeof(FloorLayout) * 2 + fixtureScratchBytes + maxCells * 6;
}
Failure validateStone(const FloorLayout& f) noexcept {
    if (f.width != floorDimension || f.height != floorDimension || f.roomCount < 10 || f.roomCount > 12 ||
        unsigned(f.profile) > 1 || f.family > 1 || f.rotation > 3)
        return {Error::Bounds};
    std::array<unsigned, maxRooms> areas{};
    unsigned count = 0, small = maxCells, large = 0, types = 0;
    bool landmark = false;
    for (unsigned r = 0; r < f.roomCount; ++r) {
        unsigned area = 0;
        for (unsigned i = 0; i < unsigned(f.width * f.height); ++i)
            area += f.membership[i] == r;
        const auto& room = f.rooms[r];
        if (room.shape > 4 || room.rotation > 3)
            return {Error::Room, int(r)};
        if (room.role == Role::Junction || room.role == Role::Optional) {
            if (room.size.x < 3 || room.size.y < 3 || room.size.x > 9 || room.size.y > 9 || area < 12 ||
                area > 54)
                return {Error::Room, int(r)};
            areas[count++] = area;
            small = std::min(small, area);
            large = std::max(large, area);
            landmark |= room.shape == 2 || room.shape == 1 ||
                        std::max(room.size.x, room.size.y) >= 2 * std::min(room.size.x, room.size.y);
        }
    }
    std::sort(areas.begin(), areas.begin() + count);
    for (unsigned i = 0; i < count; ++i)
        types += i == 0 || areas[i] != areas[i - 1];
    const auto openCount = std::count(f.floor.begin(), f.floor.end(), std::uint8_t(1));
    if (openCount < 230 || openCount > 390)
        return {Error::Bounds};
    if (types < 4 || large < small * 2 || !landmark)
        return {Error::Room};
    std::array<bool, maxCells + 1> lengths{};
    unsigned distinct = 0;
    bool bend = false;
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto& e = f.connections[i];
        if (!lengths[e.count]) {
            lengths[e.count] = true;
            ++distinct;
        }
        if (f.rooms[e.a].role == Role::Junction && f.rooms[e.b].role == Role::Junction)
            for (unsigned j = 2; j < e.count; ++j) {
                const auto a = pathCell(f, e, j - 2), b = pathCell(f, e, j - 1), c = pathCell(f, e, j);
                bend |= !(b + a * -1 == c + b * -1);
            }
    }
    if (distinct < 3 || !bend)
        return {Error::Connection};
    return {};
}
Failure validate(const FloorLayout& f) noexcept {
    if (f.width < 3 || f.height < 3 || f.width > 40 || f.height > 40 || f.roomCount > maxRooms ||
        f.connectionCount > maxConnections || f.pathCount > maxCells)
        return {Error::Capacity};
    if (!f.roomCount || !f.connectionCount || f.loopConnection >= f.connectionCount ||
        f.biome != BiomeId::Stone || f.algorithm != AlgorithmId::StoneRooms)
        return {Error::Bounds};
    Scratch s;
    s.owner.fill(noRegion);
    std::array<unsigned, maxRooms> degree{};
    std::array<unsigned, 5> roles{};
    unsigned entrance = maxRooms, approach = maxRooms, objective = maxRooms;
    for (unsigned r = 0; r < f.roomCount; ++r) {
        const auto& room = f.rooms[r];
        if (room.id != r || unsigned(room.role) > 4 || unsigned(room.facing) > 3 || room.clearance > 1 ||
            room.origin.x < 1 || room.origin.y < 1 || room.origin.x >= f.width - 1 ||
            room.origin.y >= f.height - 1 || room.size.x < 1 || room.size.y < 1 ||
            room.size.x > f.width - 1 - room.origin.x || room.size.y > f.height - 1 - room.origin.y)
            return {Error::Room, int(r)};
        ++roles[unsigned(room.role)];
        if (room.role == Role::Entrance)
            entrance = r;
        if (room.role == Role::Approach)
            approach = r;
        if (room.role == Role::Objective)
            objective = r;
        if (regionAt(f, room.anchor) != r || !floorCell(f, room.anchor))
            return {Error::Anchor, int(r)};
        for (int y = -int(room.clearance); y <= int(room.clearance); ++y)
            for (int x = -int(room.clearance); x <= int(room.clearance); ++x)
                if (regionAt(f, room.anchor + Cell{x, y}) != r || !floorCell(f, room.anchor + Cell{x, y}))
                    return {Error::Anchor, int(r)};
        if ((room.role == Role::Entrance || room.role == Role::Objective || room.role == Role::Approach) &&
            room.clearance != 1)
            return {Error::Anchor, int(r)};
    }
    if (roles[unsigned(Role::Entrance)] != 1 || roles[unsigned(Role::Approach)] != 1 ||
        roles[unsigned(Role::Objective)] != 1 || roles[unsigned(Role::Optional)] < 1 ||
        roles[unsigned(Role::Optional)] > 2)
        return {Error::Room};
    unsigned offset = 0;
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto& e = f.connections[i];
        if (e.a >= f.roomCount || e.b >= f.roomCount || e.a == e.b || e.first != offset || !e.count ||
            e.first + e.count > f.pathCount || unsigned(e.facingA) > 3 || unsigned(e.facingB) > 3)
            return {Error::Connection, -1, int(i)};
        for (unsigned j = 0; j < i; ++j) {
            const auto& o = f.connections[j];
            if ((e.a == o.a && e.b == o.b) || (e.a == o.b && e.b == o.a))
                return {Error::Connection, -1, int(i)};
        }
        if (!(pathCell(f, e, 0) == e.socketA) || !(pathCell(f, e, e.count - 1) == e.socketB) ||
            regionAt(f, e.socketA + forward(e.facingA) * -1) != e.a ||
            regionAt(f, e.socketB + forward(e.facingB) * -1) != e.b)
            return {Error::Port, -1, int(i)};
        if (e.count > 1 && (!(pathCell(f, e, 1) == e.socketA + forward(e.facingA)) ||
                            !(pathCell(f, e, e.count - 2) == e.socketB + forward(e.facingB))))
            return {Error::Port, -1, int(i)};
        ++degree[e.a];
        ++degree[e.b];
        for (unsigned j = 0; j < e.count; ++j) {
            const auto c = pathCell(f, e, j);
            if (!floorCell(f, c) || regionAt(f, c) != noRegion ||
                (j && manhattan(c, pathCell(f, e, j - 1)) != 1))
                return {Error::Port, -1, int(i)};
            const auto n = unsigned(c.y * f.width + c.x);
            if (s.owner[n] != noRegion)
                return {Error::Connection, -1, int(i)};
            s.owner[n] = std::uint8_t(maxRooms + i);
            s.step[n] = std::uint16_t(j);
        }
        offset += e.count;
        if ((f.rooms[e.a].role == Role::Objective && e.b != approach) ||
            (f.rooms[e.b].role == Role::Objective && e.a != approach))
            return {Error::Connection, -1, int(i)};
    }
    if (offset != f.pathCount || degree[entrance] != 1 || degree[objective] != 1 || degree[approach] != 2)
        return {Error::Connection};
    for (unsigned r = 0; r < f.roomCount; ++r)
        if (f.rooms[r].role == Role::Optional && degree[r] != 1)
            return {Error::Branch, int(r)};
    if (!(f.spawn == Player{f.rooms[entrance].anchor, f.rooms[entrance].facing}))
        return {Error::Anchor, int(entrance)};
    unsigned floors = 0;
    for (unsigned n = 0; n < maxCells; ++n) {
        if (n >= unsigned(f.width * f.height)) {
            if (f.floor[n] || f.membership[n] != noRegion)
                return {Error::Bounds};
            continue;
        }
        const Cell c{int(n) % f.width, int(n) / f.width};
        if (f.floor[n] > 1 || (!f.floor[n] && f.membership[n] != noRegion))
            return {Error::Bounds};
        if (!f.floor[n])
            continue;
        if (c.x == 0 || c.y == 0 || c.x == f.width - 1 || c.y == f.height - 1)
            return {Error::Bounds};
        ++floors;
        const auto r = f.membership[n];
        if (r != noRegion) {
            if (r >= f.roomCount || s.owner[n] != noRegion)
                return {Error::Room};
            const auto& room = f.rooms[r];
            if (c.x < room.origin.x || c.y < room.origin.y || c.x >= room.origin.x + room.size.x ||
                c.y >= room.origin.y + room.size.y)
                return {Error::Room, int(r)};
            s.owner[n] = r;
        } else if (s.owner[n] == noRegion)
            return {Error::Port};
    }
    // Every chamber is connected internally, independent of passages outside its bounds.
    for (unsigned r = 0; r < f.roomCount; ++r) {
        unsigned head = 0, tail = 1, total = 0;
        s.distance.fill(-1);
        auto first = unsigned(f.rooms[r].anchor.y * f.width + f.rooms[r].anchor.x);
        s.queue[0] = std::uint16_t(first);
        s.distance[first] = 0;
        for (unsigned n = 0; n < unsigned(f.width * f.height); ++n)
            total += f.membership[n] == r;
        while (head < tail) {
            const auto n = s.queue[head++];
            Cell c{int(n) % f.width, int(n) / f.width};
            for (unsigned h = 0; h < 4; ++h) {
                const auto next = c + forward(Heading(h));
                if (regionAt(f, next) != r)
                    continue;
                auto j = unsigned(next.y * f.width + next.x);
                if (s.distance[j] >= 0)
                    continue;
                s.distance[j] = 0;
                s.queue[tail++] = std::uint16_t(j);
            }
        }
        if (tail != total)
            return {Error::Disconnected, int(r)};
    }
    for (int y = 1; y < f.height - 1; ++y)
        for (int x = 1; x < f.width - 1; ++x) {
            Cell c{x, y};
            if (!floorCell(f, c))
                continue;
            auto n = unsigned(y * f.width + x);
            for (unsigned h = 0; h < 4; ++h) {
                const auto next = c + forward(Heading(h));
                if (!floorCell(f, next))
                    continue;
                const auto m = unsigned(next.y * f.width + next.x);
                const auto a = s.owner[n], b = s.owner[m];
                if (a < maxRooms && b < maxRooms) {
                    if (a != b)
                        return {Error::Port};
                    continue;
                }
                if (a >= maxRooms && b >= maxRooms) {
                    if (a != b || std::abs(int(s.step[n]) - int(s.step[m])) != 1)
                        return {Error::Connection};
                    continue;
                }
                const unsigned p = a >= maxRooms ? n : m, room = a < maxRooms ? a : b;
                const auto& e = f.connections[s.owner[p] - maxRooms];
                const Cell pc{int(p) % f.width, int(p) / f.width};
                const Cell rc = a < maxRooms ? c : next;
                if (!((room == e.a && pc == e.socketA && rc == e.socketA + forward(e.facingA) * -1) ||
                      (room == e.b && pc == e.socketB && rc == e.socketB + forward(e.facingB) * -1)))
                    return {Error::Port};
            }
        }
    if (search(f, s, f.spawn.cell) != floors)
        return {Error::Disconnected};
    const auto& e = f.connections[f.loopConnection];
    const auto a = e.socketA + forward(e.facingA) * -1, b = e.socketB + forward(e.facingB) * -1;
    search(f, s, a);
    const auto direct = s.distance[unsigned(b.y * f.width + b.x)];
    if (search(f, s, a, e.socketA) != floors - 1 || s.distance[unsigned(b.y * f.width + b.x)] < direct + 8)
        return {Error::Loop, -1, int(f.loopConnection)};
    if (const auto failure = validateStone(f))
        return failure;
    return validateFixtures(f);
}
static_assert(sizeof(Scratch) + 2 * sizeof(FloorLayout) + fixtureScratchBytes < 128 * 1024,
              "Validation scratch budget");
} // namespace blobber::generation

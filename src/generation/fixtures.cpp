#include "generation/floor.hpp"
#include <algorithm>
namespace blobber::generation {
namespace {
Cell center(const Room& r) noexcept {
    return {4 + 9 * r.site.x, 4 + 9 * r.site.y};
}
std::array<unsigned, maxRooms> distances(const FloorLayout& f, unsigned start) noexcept {
    std::array<unsigned, maxRooms> d;
    d.fill(maxRooms);
    std::array<unsigned, maxRooms> q{};
    unsigned head = 0, tail = 1;
    q[0] = start;
    d[start] = 0;
    while (head < tail) {
        auto a = q[head++];
        for (unsigned i = 0; i < f.connectionCount; ++i) {
            auto e = f.connections[i];
            auto b = e.a == a ? e.b : e.b == a ? e.a : maxRooms;
            if (b < maxRooms && d[b] == maxRooms) {
                d[b] = d[a] + 1;
                q[tail++] = b;
            }
        }
    }
    return d;
}
bool floorCell(const FloorLayout& f, Cell c) noexcept {
    return c.x >= 0 && c.y >= 0 && c.x < f.width && c.y < f.height && f.floor[c.y * f.width + c.x];
}
} // namespace
void placeFixtures(FloorLayout& f) noexcept {
    auto& defs = f.fixtures;
    defs = {};
    defs.discovery = true;
    defs.depth = f.depth;
    unsigned entry = 0, approach = 0, objective = 0;
    for (unsigned i = 0; i < f.roomCount; ++i) {
        if (f.rooms[i].role == Role::Entrance)
            entry = i;
        if (f.rooms[i].role == Role::Approach)
            approach = i;
        if (f.rooms[i].role == Role::Objective)
            objective = i;
    }
    unsigned ea = 0, aa = 0;
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        auto e = f.connections[i];
        if (e.a == entry)
            ea = e.b;
        if (e.b == entry)
            ea = e.a;
        if (e.a == approach && e.b != objective)
            aa = e.b;
        if (e.b == approach && e.a != objective)
            aa = e.a;
    }
    const auto a = center(f.rooms[aa]), b = center(f.rooms[approach]);
    const Cell delta{(b.x - a.x) / 9, (b.y - a.y) / 9};
    Heading facing = Heading::North;
    for (unsigned h = 0; h < 4; ++h)
        if (forward(Heading(h)) == delta)
            facing = Heading(h);
    defs.entries[defs.count++] = {a + delta * 4, facing, FixtureKind::Door,
                                  std::uint8_t(f.depth > 1 ? f.depth : 0)};
    if (f.depth > 1) {
        const auto e = distances(f, ea), aDist = distances(f, aa);
        unsigned chosen = maxRooms, best = 0;
        for (unsigned i = 0; i < f.roomCount; ++i) {
            if (f.rooms[i].role != Role::Junction || i == ea || i == aa)
                continue;
            const auto score = std::min(e[i], aDist[i]);
            if (chosen == maxRooms || score > best || (score == best && f.rooms[i].id < f.rooms[chosen].id)) {
                chosen = i;
                best = score;
            }
        }
        if (chosen < maxRooms)
            defs.entries[defs.count++] = {f.rooms[chosen].anchor, f.rooms[chosen].facing, FixtureKind::Key,
                                          std::uint8_t(f.depth)};
    }
    defs.entries[defs.count++] = {f.rooms[entry].anchor, f.rooms[entry].facing, FixtureKind::Upstairs, 0};
    defs.entries[defs.count++] = {f.rooms[objective].anchor, f.rooms[objective].facing,
                                  f.depth == 3 ? FixtureKind::Endpoint : FixtureKind::Downstairs, 0};
}
Failure validateFixtures(const FloorLayout& f) noexcept {
    if (f.width < 3 || f.height < 3 || f.width > 40 || f.height > 40 || f.roomCount > maxRooms ||
        f.connectionCount > maxConnections || !floorCell(f, f.spawn.cell))
        return {Error::Capacity};
    for (unsigned i = 0; i < f.roomCount; ++i) {
        const auto& r = f.rooms[i];
        if (r.site.x < 0 || r.site.y < 0 || r.site.x > 3 || r.site.y > 3 || !floorCell(f, r.anchor))
            return {Error::Room};
    }
    for (unsigned i = 0; i < f.connectionCount; ++i)
        if (f.connections[i].a >= f.roomCount || f.connections[i].b >= f.roomCount)
            return {Error::Connection};
    if (f.depth < 1 || f.depth > 3 || f.fixtures.depth != f.depth || !f.fixtures.discovery ||
        f.fixtures.count != (f.depth == 1 ? 3u : 4u))
        return {Error::Fixture};
    int door = -1, key = -1, up = -1, goal = -1;
    for (unsigned i = 0; i < f.fixtures.count; ++i) {
        const auto& d = f.fixtures.entries[i];
        if (!floorCell(f, d.cell) || unsigned(d.facing) > 3 ||
            unsigned(d.kind) > unsigned(FixtureKind::Endpoint))
            return {Error::Fixture};
        for (unsigned j = 0; j < i; ++j)
            if (d.cell == f.fixtures.entries[j].cell)
                return {Error::Fixture};
        int* slot = d.kind == FixtureKind::Door       ? &door
                    : d.kind == FixtureKind::Key      ? &key
                    : d.kind == FixtureKind::Upstairs ? &up
                                                      : &goal;
        if (*slot != -1)
            return {Error::Fixture};
        *slot = int(i);
        if ((d.kind == FixtureKind::Door || d.kind == FixtureKind::Key)
                ? d.key != (f.depth == 1 ? 0 : f.depth)
                : d.key != 0)
            return {Error::Fixture};
    }
    if (door < 0 || up < 0 || goal < 0 || (f.depth > 1) != (key >= 0))
        return {Error::Fixture};
    const auto& lock = f.fixtures.entries[door];
    if (!(f.fixtures.entries[up].cell == f.spawn.cell))
        return {Error::Fixture};
    if (f.fixtures.entries[goal].kind != (f.depth == 3 ? FixtureKind::Endpoint : FixtureKind::Downstairs))
        return {Error::Fixture};
    for (unsigned i = 0; i < f.roomCount; ++i)
        if (f.rooms[i].role == Role::Objective && !(f.rooms[i].anchor == f.fixtures.entries[goal].cell))
            return {Error::Fixture};
    bool corridor = false, keyAnchor = key < 0;
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto e = f.connections[i];
        const auto& a = f.rooms[e.a];
        const auto& b = f.rooms[e.b];
        const Room* approach = a.role == Role::Approach ? &a : b.role == Role::Approach ? &b : nullptr;
        const Room* junction = a.role == Role::Junction ? &a : b.role == Role::Junction ? &b : nullptr;
        if (!approach || !junction)
            continue;
        const auto from = center(*junction), to = center(*approach);
        const Cell delta{(to.x - from.x) / 9, (to.y - from.y) / 9};
        for (int step = 3; step <= 6; ++step)
            corridor |= lock.cell == from + delta * step && forward(lock.facing) == delta;
    }
    for (unsigned i = 0; i < f.roomCount; ++i)
        if (key >= 0 && f.rooms[i].role == Role::Junction &&
            f.rooms[i].anchor == f.fixtures.entries[key].cell)
            keyAnchor = true;
    if (!corridor || !keyAnchor)
        return {Error::Fixture};
    // One generated door: four states per cell (has key, door opened).
    std::array<std::uint8_t, maxCells * 4> seen{};
    std::array<std::uint16_t, maxCells * 4> queue{};
    unsigned head = 0, tail = 0;
    const auto push = [&](Cell c, unsigned state) {
        auto n = unsigned(c.y * f.width + c.x) + state * maxCells;
        if (!seen[n]) {
            seen[n] = 1;
            queue[tail++] = std::uint16_t(n);
        }
    };
    push(f.spawn.cell, 0);
    while (head < tail) {
        auto n = queue[head++];
        unsigned state = n / maxCells;
        n %= maxCells;
        Cell c{int(n) % f.width, int(n) / f.width};
        for (unsigned h = 0; h < 4; ++h) {
            const auto next = c + forward(Heading(h));
            if (floorCell(f, next) && (!(next == lock.cell) || !fixtureBlocks(lock, state & 2)))
                push(next, state);
            if (!(state & 2) && fixtureReachable(lock, c, Heading(h)) && (!lock.key || (state & 1)))
                push(c, state | 2);
            if (key >= 0 && !(state & 1) && fixtureReachable(f.fixtures.entries[key], c, Heading(h)))
                push(c, state | 1);
        }
    }
    const auto target = f.fixtures.entries[goal].cell;
    const auto t = unsigned(target.y * f.width + target.x);
    // The locked region must be inaccessible before opening, even after key collection.
    if (seen[t] || seen[t + maxCells] || !seen[t + (f.depth > 1 ? 3 : 2) * maxCells])
        return {Error::Progression};
    if (!seen[unsigned(f.spawn.cell.y * f.width + f.spawn.cell.x) + (f.depth > 1 ? 3 : 2) * maxCells])
        return {Error::Progression};
    for (unsigned i = 0; i < f.roomCount; ++i) {
        const auto& room = f.rooms[i];
        const auto n = unsigned(room.anchor.y * f.width + room.anchor.x);
        if ((room.role == Role::Optional || room.role == Role::Junction) && !seen[n])
            return {Error::Progression};
        if (room.role == Role::Approach && (seen[n] || seen[n + maxCells]))
            return {Error::Progression};
    }
    return {};
}
} // namespace blobber::generation

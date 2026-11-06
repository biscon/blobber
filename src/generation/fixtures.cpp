#include "game/perception.hpp"
#include "generation/floor.hpp"
#include <algorithm>
namespace blobber::generation {
namespace {
std::array<unsigned, maxRooms> distances(const FloorLayout& f, unsigned start,
                                         unsigned blocked = maxRooms) noexcept {
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
            if (b < maxRooms && b != blocked && d[b] == maxRooms) {
                d[b] = d[a] + 1;
                q[tail++] = b;
            }
        }
    }
    return d;
}
bool sectorDoor(const FloorLayout& f, Cell cell, Heading facing) noexcept {
    const auto sector = sectorJunction(f);
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto& e = f.connections[i];
        const auto other = e.a == sector ? e.b : e.b == sector ? e.a : maxRooms;
        if (other < f.roomCount && f.rooms[other].role == Role::Junction && threshold(e) == cell &&
            e.facingA == facing)
            return true;
    }
    return false;
}
} // namespace
void placeFixtures(FloorLayout& f) noexcept {
    auto& defs = f.fixtures;
    defs = {};
    defs.discovery = true;
    defs.depth = f.depth;
    const auto spec = floorSpec(f.depth);
    if (!spec)
        return;
    unsigned entry = maxRooms, approach = maxRooms, objective = maxRooms;
    for (unsigned i = 0; i < f.roomCount; ++i) {
        if (f.rooms[i].role == Role::Entrance)
            entry = i;
        if (f.rooms[i].role == Role::Approach)
            approach = i;
        if (f.rooms[i].role == Role::Objective)
            objective = i;
    }
    const auto sector = sectorJunction(f);
    if (entry == maxRooms || approach == maxRooms || objective == maxRooms || sector == maxRooms)
        return;
    const auto* approachEdge = connectionBetween(f, sector, approach);
    const auto* gateEdge = connectionBetween(f, approach, objective);
    if (!approachEdge || !gateEdge || gateEdge->count < 3)
        return;
    defs.entries[defs.count++] = {threshold(*approachEdge), approachEdge->facingA, FixtureKind::Door, 0};
    if (spec->keyedSector) {
        for (unsigned i = 0; i < f.connectionCount; ++i) {
            const auto& e = f.connections[i];
            const auto other = e.a == sector ? e.b : e.b == sector ? e.a : maxRooms;
            if (other < f.roomCount && f.rooms[other].role == Role::Junction)
                defs.entries[defs.count++] = {threshold(e), e.facingA, FixtureKind::Door,
                                              std::uint8_t(f.depth)};
        }
        const auto d = distances(f, entry, sector);
        unsigned chosen = maxRooms, best = 0;
        for (unsigned i = 0; i < f.roomCount; ++i)
            if (f.rooms[i].role == Role::Junction && i != sector) {
                if (chosen == maxRooms || d[i] > best) {
                    chosen = i;
                    best = d[i];
                }
            }
        if (chosen < maxRooms)
            defs.entries[defs.count++] = {f.rooms[chosen].anchor, Heading::North, FixtureKind::Key,
                                          std::uint8_t(f.depth)};
    }
    const auto upFacing = Heading(f.rooms[entry].rotation), downFacing = Heading(f.rooms[objective].rotation);
    defs.entries[defs.count++] = {f.rooms[entry].anchor + forward(upFacing), upFacing, FixtureKind::Upstairs,
                                  0};
    defs.entries[defs.count++] = {
        f.rooms[objective].anchor + (spec->finalFloor ? Cell{} : forward(downFacing)), downFacing,
        spec->finalFloor ? FixtureKind::Endpoint : FixtureKind::Downstairs, 0};
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto& e = f.connections[i];
        if (e.a == entry || e.b == entry)
            defs.entries[defs.count++] = {threshold(e), e.facingA, FixtureKind::Door, 0};
    }
    f.guardianGate = std::uint8_t(defs.count);
    defs.entries[defs.count++] = {gateEdge->socketA, gateEdge->facingA, FixtureKind::GuardianGate, 0};
    defs.entries[defs.count++] = {gateEdge->socketB, gateEdge->facingB, FixtureKind::Door, 0};
    f.guardianReward = pathCell(f, *gateEdge, 1);
    f.guardianApproach = approachEdge->socketB + forward(approachEdge->facingB) * -1;
    for (auto room : {entry, objective}) {
        defs.landings[defs.landingCount++] = f.rooms[room].anchor;
        for (int y = 0; y < f.height; ++y)
            for (int x = 0; x < f.width; ++x)
                if (regionAt(f, {x, y}) == room)
                    defs.npcExcluded.set(unsigned(y * f.width + x));
    }
}
Failure validateFixtures(const FloorLayout& f, const CellMask* collision) noexcept {
    if (f.width < 3 || f.height < 3 || f.width > 40 || f.height > 40 || f.roomCount > maxRooms ||
        f.connectionCount > maxConnections || !floorCell(f, f.spawn.cell))
        return {Error::Capacity};
    const auto& defs = f.fixtures;
    if (f.depth < 1 || f.depth > 3 || defs.depth != f.depth || !defs.discovery ||
        defs.count < (f.depth == 1 ? 6u : 9u) || defs.count > fixtureCapacity || defs.landingCount != 2 ||
        f.approachDoor >= defs.count || f.guardianGate >= defs.count)
        return {Error::Fixture};
    int key = -1, up = -1, goal = -1;
    unsigned doors = 0, gates = 0, locks = 0;
    for (unsigned i = 0; i < defs.count; ++i) {
        const auto& d = defs.entries[i];
        if (!floorCell(f, d.cell) || unsigned(d.facing) > 3 ||
            unsigned(d.kind) > unsigned(FixtureKind::GuardianGate))
            return {Error::Fixture};
        for (unsigned j = 0; j < i; ++j)
            if (d.cell == defs.entries[j].cell)
                return {Error::Fixture};
        const auto expected = (d.kind == FixtureKind::Key ||
                               (d.kind == FixtureKind::Door && sectorDoor(f, d.cell, d.facing))) &&
                                      f.depth > 1
                                  ? f.depth
                                  : 0u;
        if (d.key != expected)
            return {Error::Fixture};
        if (d.kind == FixtureKind::Door)
            ++doors;
        if (d.kind == FixtureKind::Door && d.key)
            ++locks;
        if (d.kind == FixtureKind::GuardianGate)
            ++gates;
        if (d.kind == FixtureKind::Key) {
            if (key >= 0)
                return {Error::Fixture};
            key = int(i);
        }
        if (d.kind == FixtureKind::Upstairs) {
            if (up >= 0)
                return {Error::Fixture};
            up = int(i);
        }
        if (d.kind == FixtureKind::Downstairs || d.kind == FixtureKind::Endpoint) {
            if (goal >= 0)
                return {Error::Fixture};
            goal = int(i);
        }
    }
    if (locks != (f.depth > 1 ? 2u : 0u) || doors < 3 || gates != 1 || up < 0 || goal < 0 ||
        (key >= 0) != (f.depth > 1) || defs.entries[f.approachDoor].kind != FixtureKind::Door ||
        defs.entries[f.guardianGate].kind != FixtureKind::GuardianGate ||
        !(stairLanding(defs.entries[up]) == f.spawn.cell) ||
        defs.entries[goal].kind != (f.depth == 3 ? FixtureKind::Endpoint : FixtureKind::Downstairs))
        return {Error::Fixture};
    bool approachPort = false, gatePort = false, goalAnchor = false;
    for (unsigned i = 0; i < f.roomCount; ++i)
        if (f.rooms[i].role == Role::Objective)
            goalAnchor =
                f.rooms[i].anchor == (stairFixture(defs.entries[goal].kind) ? stairLanding(defs.entries[goal])
                                                                            : defs.entries[goal].cell);
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto& e = f.connections[i];
        if (f.rooms[e.a].role == Role::Junction && f.rooms[e.b].role == Role::Approach)
            approachPort = defs.entries[f.approachDoor].cell == threshold(e) &&
                           defs.entries[f.approachDoor].facing == e.facingA &&
                           f.guardianApproach == e.socketB + forward(e.facingB) * -1;
        if (f.rooms[e.a].role == Role::Approach && f.rooms[e.b].role == Role::Objective)
            gatePort = defs.entries[f.guardianGate].cell == e.socketA &&
                       defs.entries[f.guardianGate].facing == e.facingA &&
                       f.guardianReward == pathCell(f, e, 1);
    }
    if (!approachPort || !gatePort || !goalAnchor)
        return {Error::Fixture};
    if (!floorCell(f, f.guardianApproach) || !floorCell(f, f.guardianReward) ||
        !floorCell(f, f.optionalSupply) || !floorCell(f, f.supply))
        return {Error::Population};
    // Four monotone progression states. Ordinary doors can be opened from a faced
    // adjacent cell without costs; locked doors require key, gate requires deaths.
    std::array<std::uint8_t, maxCells * 4> seen{};
    std::array<std::uint16_t, maxCells * 4> queue{};
    unsigned head = 0, tail = 0;
    const auto push = [&](Cell c, unsigned state) {
        if (!floorCell(f, c) || (collision && (*collision)[unsigned(c.y * f.width + c.x)]))
            return;
        const auto n = unsigned(c.y * f.width + c.x) + state * maxCells;
        if (!seen[n]) {
            seen[n] = 1;
            queue[tail++] = std::uint16_t(n);
        }
    };
    push(f.spawn.cell, 0);
    while (head < tail) {
        const auto v = queue[head++];
        const auto state = unsigned(v) / maxCells, n = unsigned(v) % maxCells;
        const Cell c{int(n) % f.width, int(n) / f.width};
        if (c == f.guardianApproach)
            push(c, state | 2);
        for (unsigned h = 0; h < 4; ++h) {
            if (key >= 0 && fixtureReachable(defs.entries[key], c, Heading(h)))
                push(c, state | 1);
            const auto next = c + forward(Heading(h));
            if (!floorCell(f, next) || (collision && (*collision)[unsigned(next.y * f.width + next.x)]))
                continue;
            bool blocked = false;
            for (unsigned i = 0; i < defs.count; ++i)
                if (defs.entries[i].cell == next) {
                    const auto& d = defs.entries[i];
                    blocked |=
                        stairFixture(d.kind) || (d.kind == FixtureKind::GuardianGate
                                                     ? !(state & 2)
                                                     : d.kind == FixtureKind::Door && d.key && !(state & 1));
                }
            if (!blocked)
                push(next, state);
        }
    }
    const auto index = [&](Cell c) { return unsigned(c.y * f.width + c.x); };
    const auto goalCell =
        stairFixture(defs.entries[goal].kind) ? stairLanding(defs.entries[goal]) : defs.entries[goal].cell;
    const auto target = index(goalCell), done = f.depth > 1 ? 3u : 2u;
    if (seen[target] || seen[target + maxCells] || !seen[target + done * maxCells] ||
        (f.depth > 1 && seen[target + 2 * maxCells]) || !seen[index(f.spawn.cell) + done * maxCells])
        return {Error::Progression};
    if (f.depth > 1 && (seen[index(f.guardianApproach)] || seen[index(f.guardianApproach) + 2 * maxCells]))
        return {Error::Progression};
    if (seen[index(f.guardianReward)] || seen[index(f.guardianReward) + maxCells] ||
        !seen[index(f.guardianReward) + done * maxCells])
        return {Error::Progression};
    for (unsigned i = 0; i < f.roomCount; ++i) {
        const auto& r = f.rooms[i];
        if (!floorCell(f, r.anchor))
            return {Error::Room};
        const bool sector = f.depth > 1 && i == sectorJunction(f);
        if (sector && (seen[index(r.anchor)] || !seen[index(r.anchor) + maxCells]))
            return {Error::Progression};
        if ((r.role == Role::Optional || (r.role == Role::Junction && !sector)) && !seen[index(r.anchor)])
            return {Error::Progression};
    }
    if (const auto failure = validatePopulation(f))
        return failure;
    if (!f.populationCount || f.populationCount > f.population.size())
        return {Error::Population};
    unsigned guardians = 0, optionalActors = 0, ordinaryActors = 0;
    for (unsigned i = 0; i < f.populationCount; ++i) {
        const auto& p = f.population[i];
        if (p.id != i || !floorCell(f, p.cell) || defs.npcExcluded[index(p.cell)] || unsigned(p.facing) > 3 ||
            p.subcell.x < 0 || p.subcell.x > 1 || p.subcell.y < 0 || p.subcell.y > 1 ||
            unsigned(p.recipe) > unsigned(ActorRecipe::Bugbear))
            return {Error::Population};
        for (unsigned j = 0; j < i; ++j)
            if (p.cell == f.population[j].cell && p.subcell == f.population[j].subcell)
                return {Error::Population};
        for (unsigned j = 0; j < defs.count; ++j)
            if (p.cell == defs.entries[j].cell)
                return {Error::Population};
        const auto membership = regionAt(f, p.cell);
        const Room* room = membership < f.roomCount ? &f.rooms[membership] : nullptr;
        if (!room || (p.guardian && room->role != Role::Approach) ||
            (!p.guardian && room->role != Role::Junction && room->role != Role::Optional))
            return {Error::Population};
        if (!p.guardian) {
            optionalActors += room->role == Role::Optional;
            ordinaryActors += room->role == Role::Junction;
        }
        if (p.guardian && (!inBowRange(f.guardianApproach, p.cell) ||
                           !gridSight([&](Cell c) { return !floorCell(f, c); }, f.guardianApproach, p.cell)))
            return {Error::Population};
        if (p.guardian) {
            ++guardians;
            if (!seen[index(p.cell) + (f.depth > 1 ? 1u : 0u) * maxCells])
                return {Error::Population};
        }
    }
    if (guardians != (f.depth == 3 ? 3u : 4u) || optionalActors != (f.depth == 1 ? 3u : 2u) ||
        ordinaryActors != (f.depth == 1 ? 4u : 6u))
        return {Error::Population};
    // Allocation-free supercover on final base tiles: every door is treated open.
    const auto sight = [&](Cell from, Cell to) {
        return gridSight([&](Cell c) { return !floorCell(f, c); }, from, to);
    };
    if (!(defs.landings[0] == f.spawn.cell) || !(defs.landings[1] == goalCell))
        return {Error::Landing};
    for (auto landing : defs.landings) {
        if (!floorCell(f, landing) || !defs.npcExcluded[index(landing)])
            return {Error::Landing};
        for (int y = 0; y < f.height; ++y)
            for (int x = 0; x < f.width; ++x) {
                const Cell c{x, y};
                if (floorCell(f, c) && !defs.npcExcluded[index(c)] && sight(c, landing))
                    return {Error::Landing};
            }
    }
    if (defs.npcExcluded[index(f.optionalSupply)] || defs.npcExcluded[index(f.guardianReward)])
        return {Error::Population};
    return {};
}
} // namespace blobber::generation

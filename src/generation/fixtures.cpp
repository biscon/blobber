#include "game/perception.hpp"
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
    const auto entranceCenter = center(f.rooms[entry]), neighbor = center(f.rooms[ea]);
    const Cell entryDirection{(neighbor.x - entranceCenter.x) / 9, (neighbor.y - entranceCenter.y) / 9};
    defs.entries[defs.count++] = {entranceCenter + entryDirection * 4, f.rooms[entry].facing,
                                  FixtureKind::Door, 0};
    const auto from = center(f.rooms[approach]), to = center(f.rooms[objective]);
    const Cell exitDirection{(to.x - from.x) / 9, (to.y - from.y) / 9};
    Heading exitFacing = Heading::North;
    for (unsigned h = 0; h < 4; ++h)
        if (forward(Heading(h)) == exitDirection)
            exitFacing = Heading(h);
    f.guardianGate = std::uint8_t(defs.count);
    defs.entries[defs.count++] = {from + exitDirection * 3, exitFacing, FixtureKind::GuardianGate, 0};
    defs.entries[defs.count++] = {from + exitDirection * 6, exitFacing, FixtureKind::Door, 0};
    f.guardianReward = from + exitDirection * 5;
    f.guardianApproach = b + delta * -2;
    for (auto room : {entry, objective}) {
        defs.landings[defs.landingCount++] = f.rooms[room].anchor;
        const auto c = center(f.rooms[room]);
        for (int y = -2; y <= 2; ++y)
            for (int x = -2; x <= 2; ++x) {
                const auto p = c + Cell{x, y};
                if (floorCell(f, p))
                    defs.npcExcluded.set(unsigned(p.y * f.width + p.x));
            }
    }
}
void placePopulation(FloorLayout& f, GenerationRequest request, unsigned attempt, bool fallback) noexcept {
    unsigned approach = 0, optional = maxRooms, ordinary = maxRooms;
    const auto start =
        fallback ? 0u : unsigned(seedFor(request, Domain::Encounter, 0, attempt) % f.roomCount);
    for (unsigned n = 0; n < f.roomCount; ++n) {
        const auto i = (start + n) % f.roomCount;
        const auto& r = f.rooms[i];
        if (r.role == Role::Approach)
            approach = i;
        if (r.role == Role::Optional && optional == maxRooms)
            optional = i;
        bool key = false;
        for (unsigned j = 0; j < f.fixtures.count; ++j)
            key |= f.fixtures.entries[j].kind == FixtureKind::Key && f.fixtures.entries[j].cell == r.anchor;
        if (r.role == Role::Junction && !key && ordinary == maxRooms)
            ordinary = i;
    }
    const auto add = [&](Cell c, Cell sub, Heading h, ActorRecipe recipe, bool guardian) {
        const auto i = f.populationCount++;
        f.population[i] = {c, sub, h, recipe, std::uint8_t(i), guardian};
    };
    if (ordinary < maxRooms)
        add(f.rooms[ordinary].anchor, {0, 0}, f.rooms[ordinary].facing, ActorRecipe::Goblin, false);
    if (optional < maxRooms) {
        add(f.rooms[optional].anchor, {0, 0}, f.rooms[optional].facing, ActorRecipe::Goblin, false);
        f.optionalSupply = f.rooms[optional].anchor;
    }
    const auto facing = Heading((unsigned(f.fixtures.entries[f.approachDoor].facing) + 2) % 4);
    add(f.rooms[approach].anchor, {0, 0}, facing, ActorRecipe::Guardian, true);
    add(f.rooms[approach].anchor, {1, 1}, facing, ActorRecipe::Archer, true);
}
Failure validateFixtures(const FloorLayout& f) noexcept {
    if (f.width < 3 || f.height < 3 || f.width > 40 || f.height > 40 || f.roomCount > maxRooms ||
        f.connectionCount > maxConnections || !floorCell(f, f.spawn.cell))
        return {Error::Capacity};
    const auto& defs = f.fixtures;
    if (f.depth < 1 || f.depth > 3 || defs.depth != f.depth || !defs.discovery ||
        defs.count != (f.depth == 1 ? 6u : 7u) || defs.landingCount != 2 || f.approachDoor >= defs.count ||
        f.guardianGate >= defs.count)
        return {Error::Fixture};
    int key = -1, up = -1, goal = -1;
    unsigned doors = 0, gates = 0;
    for (unsigned i = 0; i < defs.count; ++i) {
        const auto& d = defs.entries[i];
        if (!floorCell(f, d.cell) || unsigned(d.facing) > 3 ||
            unsigned(d.kind) > unsigned(FixtureKind::GuardianGate))
            return {Error::Fixture};
        for (unsigned j = 0; j < i; ++j)
            if (d.cell == defs.entries[j].cell)
                return {Error::Fixture};
        const auto expected =
            (i == f.approachDoor || d.kind == FixtureKind::Key) && f.depth > 1 ? f.depth : 0u;
        if (d.key != expected)
            return {Error::Fixture};
        if (d.kind == FixtureKind::Door)
            ++doors;
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
    if (doors != 3 || gates != 1 || up < 0 || goal < 0 || (key >= 0) != (f.depth > 1) ||
        defs.entries[f.approachDoor].kind != FixtureKind::Door ||
        defs.entries[f.guardianGate].kind != FixtureKind::GuardianGate ||
        !(defs.entries[up].cell == f.spawn.cell) ||
        defs.entries[goal].kind != (f.depth == 3 ? FixtureKind::Endpoint : FixtureKind::Downstairs))
        return {Error::Fixture};
    bool approachPort = false, gatePort = false, goalAnchor = false;
    for (unsigned i = 0; i < f.roomCount; ++i) {
        const auto& r = f.rooms[i];
        if (r.role == Role::Objective)
            goalAnchor |= r.anchor == defs.entries[goal].cell;
    }
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto e = f.connections[i];
        if (e.a >= f.roomCount || e.b >= f.roomCount)
            return {Error::Connection};
        const auto& a = f.rooms[e.a];
        const auto& b = f.rooms[e.b];
        const Room* approach = a.role == Role::Approach ? &a : b.role == Role::Approach ? &b : nullptr;
        const Room* junction = a.role == Role::Junction ? &a : b.role == Role::Junction ? &b : nullptr;
        const Room* objective = a.role == Role::Objective ? &a : b.role == Role::Objective ? &b : nullptr;
        if (approach && junction) {
            const auto c = center(*junction), t = center(*approach);
            const Cell d{(t.x - c.x) / 9, (t.y - c.y) / 9};
            approachPort |= defs.entries[f.approachDoor].cell == c + d * 4 &&
                            forward(defs.entries[f.approachDoor].facing) == d &&
                            f.guardianApproach == t + d * -2;
        }
        if (approach && objective) {
            const auto c = center(*approach), t = center(*objective);
            const Cell d{(t.x - c.x) / 9, (t.y - c.y) / 9};
            gatePort |= defs.entries[f.guardianGate].cell == c + d * 3 && f.guardianReward == c + d * 5;
        }
    }
    if (!approachPort || !gatePort || !goalAnchor)
        return {Error::Fixture};
    if (!floorCell(f, f.guardianApproach) || !floorCell(f, f.guardianReward) ||
        !floorCell(f, f.optionalSupply))
        return {Error::Population};
    // Four monotone progression states. Ordinary doors can be opened from a faced
    // adjacent cell without costs; locked doors require key, gate requires deaths.
    std::array<std::uint8_t, maxCells * 4> seen{};
    std::array<std::uint16_t, maxCells * 4> queue{};
    unsigned head = 0, tail = 0;
    const auto push = [&](Cell c, unsigned state) {
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
            if (!floorCell(f, next))
                continue;
            bool blocked = false;
            for (unsigned i = 0; i < defs.count; ++i)
                if (defs.entries[i].cell == next) {
                    const auto& d = defs.entries[i];
                    blocked |= d.kind == FixtureKind::GuardianGate
                                   ? !(state & 2)
                                   : d.kind == FixtureKind::Door && d.key && !(state & 1);
                }
            if (!blocked)
                push(next, state);
        }
    }
    const auto index = [&](Cell c) { return unsigned(c.y * f.width + c.x); };
    const auto target = index(defs.entries[goal].cell), done = f.depth > 1 ? 3u : 2u;
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
        if ((r.role == Role::Optional || r.role == Role::Junction) && !seen[index(r.anchor)])
            return {Error::Progression};
    }
    if (f.populationCount != 4)
        return {Error::Population};
    unsigned guardians = 0, optionalActors = 0, ordinaryActors = 0;
    for (unsigned i = 0; i < f.populationCount; ++i) {
        const auto& p = f.population[i];
        if (p.id != i || !floorCell(f, p.cell) || defs.npcExcluded[index(p.cell)] || unsigned(p.facing) > 3 ||
            p.subcell.x < 0 || p.subcell.x > 1 || p.subcell.y < 0 || p.subcell.y > 1 ||
            unsigned(p.recipe) > 2)
            return {Error::Population};
        for (unsigned j = 0; j < i; ++j)
            if (p.cell == f.population[j].cell && p.subcell == f.population[j].subcell)
                return {Error::Population};
        for (unsigned j = 0; j < defs.count; ++j)
            if (p.cell == defs.entries[j].cell)
                return {Error::Population};
        const Room* room = nullptr;
        for (unsigned j = 0; j < f.roomCount; ++j) {
            const auto c = center(f.rooms[j]);
            if (std::abs(p.cell.x - c.x) <= 2 && std::abs(p.cell.y - c.y) <= 2)
                room = &f.rooms[j];
        }
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
    if (guardians != 2 || optionalActors != 1 || ordinaryActors != 1)
        return {Error::Population};
    // Allocation-free supercover on final base tiles: every door is treated open.
    const auto sight = [&](Cell from, Cell to) {
        return gridSight([&](Cell c) { return !floorCell(f, c); }, from, to);
    };
    if (!(defs.landings[0] == f.spawn.cell) || !(defs.landings[1] == defs.entries[goal].cell))
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

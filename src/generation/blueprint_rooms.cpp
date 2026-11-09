#include "generation/blueprint_rooms.hpp"
#include <algorithm>
#include <cstring>
namespace blobber::generation {
const BlueprintInstance* blueprintAt(const FloorLayout& f, unsigned room) noexcept {
    for (unsigned i = 0; i < std::min(f.blueprintCount, 4u); ++i)
        if (f.blueprints[i].room == room)
            return &f.blueprints[i];
    return nullptr;
}
void validateBlueprintRoster(const blueprints::BlueprintCatalog& c) {
    blueprints::validate(c);
    unsigned purposes = 0;
    for (unsigned i = 0; i < c.roomCount; ++i) {
        unsigned p = unsigned(c.rooms[i].purpose);
        if (p > 3 || (purposes & (1u << p)))
            throw blueprints::Failure(blueprints::Error::Content, "Duplicate room purpose");
        purposes |= 1u << p;
    }
    if (purposes != 15 || c.roomCount != 4)
        throw blueprints::Failure(blueprints::Error::Content, "Campaign requires exactly four room purposes");
}
unsigned roomDistance(const FloorLayout& f, unsigned a, unsigned b) noexcept {
    if (a >= f.roomCount || b >= f.roomCount)
        return maxRooms;
    std::array<unsigned, maxRooms> d{}, q{};
    d.fill(maxRooms);
    d[a] = 0;
    unsigned head = 0, tail = 1;
    q[0] = a;
    while (head < tail) {
        auto r = q[head++];
        for (unsigned i = 0; i < f.connectionCount; ++i) {
            const auto& e = f.connections[i];
            auto n = e.a == r ? e.b : e.b == r ? e.a : maxRooms;
            if (n < f.roomCount && d[n] == maxRooms) {
                d[n] = d[r] + 1;
                q[tail++] = n;
            }
        }
    }
    return d[b];
}
Failure prepareBlueprintRooms(const FloorLayout& f, const blueprints::BlueprintCatalog& c,
                              BlueprintReservations& out, WorldDefinition* world) {
    out = {};
    if (f.blueprintCount != 4 || f.blueprintFingerprint != blueprints::fingerprint(c) ||
        f.roomCount > maxRooms || f.width * f.height > int(maxCells))
        return {Error::Blueprint};
    unsigned purposes = 0;
    unsigned kitchen = maxRooms, hall = maxRooms;
    for (unsigned n = 0; n < 4; ++n) {
        const auto& instance = f.blueprints[n];
        if (instance.room >= f.roomCount || out.authored[instance.room] ||
            std::find(instance.id.begin(), instance.id.end(), '\0') == instance.id.end())
            return {Error::Blueprint};
        const auto& source = c.rooms[c.room(instance.id.data()).index];
        if (instance.id != source.id || instance.purpose != source.purpose || instance.rotation > 3 ||
            !(source.rotations & (1u << instance.rotation)) ||
            (purposes & (1u << unsigned(source.purpose))) || instance.ceiling != source.ceiling)
            return {Error::Blueprint, int(instance.room)};
        purposes |= 1u << unsigned(source.purpose);
        if (source.purpose == blueprints::Purpose::Kitchen)
            kitchen = instance.room;
        if (source.purpose == blueprints::Purpose::MeadHall)
            hall = instance.room;
        auto r = blueprints::transform(source, instance.rotation);
        const auto& room = f.rooms[instance.room];
        if (!(room.size == r.size) || room.rotation != instance.rotation || room.role != Role::Junction ||
            room.shape != 5)
            return {Error::Blueprint, int(instance.room)};
        unsigned mask = 0;
        for (unsigned e = 0; e < f.connectionCount; ++e) {
            const auto& edge = f.connections[e];
            if (edge.a != instance.room && edge.b != instance.room)
                continue;
            const auto socket = edge.a == instance.room ? edge.socketA : edge.socketB;
            const auto face = edge.a == instance.room ? edge.facingA : edge.facingB;
            unsigned found = r.socketCount;
            for (unsigned s = 0; s < r.socketCount; ++s)
                if (socket == room.origin + r.sockets[s].cell && face == r.sockets[s].outward)
                    found = s;
            if (found == r.socketCount || (mask & (1u << found)))
                return {Error::Port, int(instance.room), int(e)};
            mask |= 1u << found;
        }
        if (mask != instance.sockets || !mask)
            return {Error::Port, int(instance.room)};
        blueprints::selectSockets(c, r, mask);
        out.authored[instance.room] = true;
        auto mark = [&](Cell local, CellMask& m) {
            const auto p = room.origin + local;
            m[unsigned(p.y * f.width + p.x)] = true;
        };
        for (int y = 0; y < r.size.y; ++y)
            for (int x = 0; x < r.size.x; ++x) {
                auto p = room.origin + Cell{x, y};
                if (!inside(f, p) || floorCell(f, p) == r.solid[blueprints::index({x, y})] ||
                    (!r.solid[blueprints::index({x, y})] && regionAt(f, p) != instance.room))
                    return {Error::Blueprint, int(instance.room)};
            }
        for (unsigned i = 0; i < r.placementCount; ++i) {
            const auto& p = r.placements[i];
            const auto& k = c.kits[p.kit.index];
            if (k.clearance > 0)
                continue;
            for (int y = 0; y < k.size.y; ++y)
                for (int x = 0; x < k.size.x; ++x) {
                    const auto local = blueprints::footprint(k, p, unsigned(x), unsigned(y));
                    mark(local, out.furnished);
                    if (k.blocking)
                        mark(local, out.blocked);
                }
        }
        auto rect = [&](const blueprints::Rect& a, CellMask& m) {
            for (int y = 0; y < a.size.y; ++y)
                for (int x = 0; x < a.size.x; ++x)
                    mark(a.origin + Cell{x, y}, m);
        };
        for (unsigned i = 0; i < r.laneCount; ++i)
            rect(r.lanes[i], out.lanes);
        for (unsigned i = 0; i < r.encounterCount; ++i)
            rect(r.encounters[i], out.encounters);
        for (unsigned i = 0; i < r.rewardCount; ++i) {
            mark(r.rewards[i].cell, out.rewards);
            mark(r.rewards[i].access, out.rewards);
        }
        out.reward[instance.room] = {room.origin + r.rewards[0].cell, room.origin + r.rewards[0].access};
        if (out.blocked[unsigned(room.anchor.y * f.width + room.anchor.x)])
            return {Error::Anchor, int(instance.room)};
        if (world)
            blueprints::appendFurnishings(c, r, room.origin, *world);
    }
    const auto* pair = connectionBetween(f, kitchen, hall);
    if (purposes != 15 || !pair || pair->count > 3)
        return {Error::Blueprint};
    return {};
}
Failure validateBlueprintWorld(const FloorLayout& f, const blueprints::BlueprintCatalog& c, const Map& map) {
    BlueprintReservations reservations;
    WorldDefinition expected;
    if (auto e = prepareBlueprintRooms(f, c, reservations, &expected))
        return e;
    const auto& actual = map.definition();
    // Compare only blueprint-owned fields, using their normal canonical serialization hash.
    WorldDefinition furnished;
    furnished.hearth = actual.hearth;
    furnished.scenery = actual.scenery;
    furnished.ambientFires = actual.ambientFires;
    if (furnished.fingerprint() != expected.fingerprint())
        return {Error::Blueprint};
    for (unsigned i = 0; i < f.populationCount; ++i) {
        auto p = f.population[i].cell;
        const auto idx = unsigned(p.y * f.width + p.x), r = regionAt(f, p);
        if (reservations.blocked[idx] ||
            (r < maxRooms && reservations.authored[r] &&
             (!reservations.encounters[idx] || reservations.furnished[idx] || reservations.rewards[idx])))
            return {Error::Population, int(r)};
    }
    for (unsigned i = 0; i < f.blueprintCount; ++i) {
        const auto& b = f.blueprints[i];
        for (int y = 0; y < f.height; ++y)
            for (int x = 0; x < f.width; ++x)
                if (regionAt(f, {x, y}) == b.room && map.surface({x, y}).ceiling != b.ceiling)
                    return {Error::Blueprint, int(b.room)};
    }
    for (const auto& d : actual.decorations) {
        const auto r = regionAt(f, d.cell);
        if (r >= maxRooms || !reservations.authored[r])
            continue;
        const auto site = reservations.reward[r];
        if (d.kind != DecorationKind::Chest || d.container >= f.cacheCount || !(d.cell == site.cell) ||
            !(d.cell + forward(d.face) * -1 == site.access) || !(f.caches[d.container].cell == d.cell) ||
            f.caches[d.container].facing != d.face)
            return {Error::Dressing, int(r)};
    }
    for (int y = 0; y < f.height; ++y)
        for (int x = 0; x < f.width; ++x)
            reservations.blocked[unsigned(y * f.width + x)] = map.movementBlocked({x, y});
    return validateFixtures(f, &reservations.blocked);
}
} // namespace blobber::generation

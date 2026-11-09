#include "world/blueprints.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace blobber::blueprints {
namespace {
void require(bool ok, Error error, const char* message) {
    if (!ok)
        throw Failure(error, message);
}
bool validName(const Name& n) {
    const auto end = std::find(n.begin(), n.end(), '\0');
    return end != n.begin() && end != n.end() && std::all_of(n.begin(), end, [](char c) {
               return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
           });
}
void rect(const Rect& r, Cell size) {
    require(r.size.x > 0 && r.size.y > 0 && r.size.x <= int(maxSide) && r.size.y <= int(maxSide) &&
                inside(r.origin, size) && inside(r.origin + r.size + Cell{-1, -1}, size),
            Error::Bounds, "Region outside room");
}
template <class F> void each(const Rect& r, F f) {
    for (int y = 0; y < r.size.y; ++y)
        for (int x = 0; x < r.size.x; ++x)
            f(r.origin + Cell{x, y});
}
Rect turnRect(Rect r, Cell size, unsigned turns) {
    const auto a = rotate(r.origin, size, turns);
    const auto b = rotate(r.origin + r.size + Cell{-1, -1}, size, turns);
    return {{std::min(a.x, b.x), std::min(a.y, b.y)}, rotatedSize(r.size, turns)};
}
unsigned defaultSockets(const RoomBlueprint& r) {
    for (unsigned i = 0; i < r.socketCount; ++i)
        if (r.sockets[i].outward == Heading::South)
            return 1u << i;
    return 1;
}
} // namespace
bool inside(Cell c, Cell size) noexcept {
    return c.x >= 0 && c.y >= 0 && c.x < size.x && c.y < size.y;
}
Cell rotatedSize(Cell s, unsigned turns) noexcept {
    return turns % 2 ? Cell{s.y, s.x} : s;
}
Cell rotate(Cell c, Cell s, unsigned turns) noexcept {
    switch (turns % 4) {
    case 1:
        return {s.y - 1 - c.y, c.x};
    case 2:
        return {s.x - 1 - c.x, s.y - 1 - c.y};
    case 3:
        return {c.y, s.x - 1 - c.x};
    default:
        return c;
    }
}
Heading rotate(Heading h, unsigned turns) noexcept {
    return Heading((unsigned(h) + turns) % 4);
}
Cell footprint(const Kit&, const Placement& p, unsigned x, unsigned y) noexcept {
    return p.anchor + right(p.facing) * int(x) + forward(p.facing) * -int(y);
}
KitId BlueprintCatalog::kit(std::string_view id) const {
    require(kitCount <= maxKits, Error::Capacity, "Invalid kit catalog size");
    for (unsigned i = 0; i < kitCount; ++i)
        if (id == kits[i].id.data())
            return {i};
    throw Failure(Error::Reference, "Unknown blueprint kit: " + std::string(id));
}
RoomId BlueprintCatalog::room(std::string_view id) const {
    require(roomCount <= maxRooms, Error::Capacity, "Invalid room catalog size");
    for (unsigned i = 0; i < roomCount; ++i)
        if (id == rooms[i].id.data())
            return {i};
    throw Failure(Error::Reference, "Unknown room blueprint: " + std::string(id));
}
RoomBlueprint transform(const RoomBlueprint& r, unsigned turns) {
    require(turns < 4 && (r.rotations & (1u << turns)), Error::Content, "Unsupported room rotation");
    RoomBlueprint out = r;
    out.size = rotatedSize(r.size, turns);
    out.solid.fill(false);
    for (int y = 0; y < r.size.y; ++y)
        for (int x = 0; x < r.size.x; ++x)
            out.solid[index(rotate({x, y}, r.size, turns))] = r.solid[index({x, y})];
    for (unsigned i = 0; i < r.placementCount; ++i) {
        out.placements[i].anchor = rotate(r.placements[i].anchor, r.size, turns);
        out.placements[i].facing = rotate(r.placements[i].facing, turns);
    }
    for (unsigned i = 0; i < r.socketCount; ++i) {
        out.sockets[i].cell = rotate(r.sockets[i].cell, r.size, turns);
        out.sockets[i].outward = rotate(r.sockets[i].outward, turns);
    }
    for (unsigned i = 0; i < r.laneCount; ++i)
        out.lanes[i] = turnRect(r.lanes[i], r.size, turns);
    for (unsigned i = 0; i < r.encounterCount; ++i)
        out.encounters[i] = turnRect(r.encounters[i], r.size, turns);
    for (unsigned i = 0; i < r.rewardCount; ++i) {
        out.rewards[i].cell = rotate(r.rewards[i].cell, r.size, turns);
        out.rewards[i].access = rotate(r.rewards[i].access, r.size, turns);
    }
    return out;
}
Mask occupied(const BlueprintCatalog& c, const RoomBlueprint& r) {
    Mask out = r.solid;
    require(r.placementCount <= maxPlacements, Error::Capacity, "Too many placements");
    for (unsigned i = 0; i < r.placementCount; ++i) {
        const auto& p = r.placements[i];
        require(p.kit.index < c.kitCount && unsigned(p.facing) < 4 && inside(p.anchor, r.size),
                Error::Reference, "Invalid placement kit, anchor or facing");
        const auto& k = c.kits[p.kit.index];
        for (unsigned y = 0; y < unsigned(k.size.y); ++y)
            for (unsigned x = 0; x < unsigned(k.size.x); ++x) {
                const auto cell = footprint(k, p, x, y);
                require(inside(cell, r.size), Error::Bounds, "Furniture footprint outside room");
                require(!out[index(cell)], Error::Overlap,
                        "Furniture footprints overlap terrain or furniture");
                out[index(cell)] = true;
            }
    }
    return out;
}
void validateRoom(const BlueprintCatalog& c, const RoomBlueprint& r) {
    require(validName(r.id) && unsigned(r.purpose) <= unsigned(Purpose::Armory), Error::Content,
            "Invalid room identity");
    require(r.size.x > 0 && r.size.y > 0 && r.size.x <= int(maxSide) && r.size.y <= int(maxSide),
            Error::Bounds, "Invalid room size");
    require(r.ceiling >= 4 && r.ceiling <= 8 && r.rotations && r.rotations <= 15, Error::Content,
            "Invalid ceiling or rotations");
    require(r.placementCount <= maxPlacements && r.socketCount > 0 && r.socketCount <= maxSockets &&
                r.laneCount > 0 && r.laneCount <= maxLanes && r.encounterCount > 0 &&
                r.encounterCount <= maxRegions && r.rewardCount > 0 && r.rewardCount <= maxRegions,
            Error::Capacity, "Room record capacity exceeded");
    for (unsigned y = 0; y < maxSide; ++y)
        for (unsigned x = 0; x < maxSide; ++x)
            require(!r.solid[y * maxSide + x] || inside({int(x), int(y)}, r.size), Error::Bounds,
                    "Solid cell outside room");
    const auto blocked = occupied(c, r);
    Mask reserved{};
    for (unsigned i = 0; i < r.placementCount; ++i) {
        const auto& p = r.placements[i];
        const auto& k = c.kits[p.kit.index];
        require(k.height <= float(r.ceiling) * .25f, Error::Clearance, "Furniture exceeds ceiling");
        // Concrete wall-backed kit contracts; tables, beds and lockers are free-standing.
        const std::string_view id = k.id.data();
        if (id == "hearth" || id == "work-surface" || id == "provisions" || id == "weapon-storage" ||
            id == "armor-display")
            for (unsigned x = 0; x < unsigned(k.size.x); ++x) {
                const auto back = footprint(k, p, x, unsigned(k.size.y - 1)) + forward(p.facing) * -1;
                require(!inside(back, r.size) || r.solid[index(back)], Error::Clearance,
                        "Wall-backed kit lacks backing wall");
            }
    }
    for (unsigned i = 0; i < r.laneCount; ++i) {
        rect(r.lanes[i], r.size);
        each(r.lanes[i], [&](Cell p) {
            require(!blocked[index(p)], Error::Clearance, "Blocked reserved lane");
            reserved[index(p)] = true;
        });
    }
    Mask encounters{};
    for (unsigned i = 0; i < r.encounterCount; ++i) {
        rect(r.encounters[i], r.size);
        require(r.encounters[i].size.x >= 2 && r.encounters[i].size.y >= 2, Error::Clearance,
                "Encounter requires a 2x2 area");
        each(r.encounters[i], [&](Cell p) {
            require(!blocked[index(p)] && !encounters[index(p)], Error::Clearance,
                    "Blocked or overlapping encounter region");
            encounters[index(p)] = true;
        });
    }
    Mask rewards{};
    for (unsigned i = 0; i < r.rewardCount; ++i) {
        const auto& a = r.rewards[i];
        require(inside(a.cell, r.size) && inside(a.access, r.size), Error::Bounds, "Reward outside room");
        require(std::abs(a.cell.x - a.access.x) + std::abs(a.cell.y - a.access.y) == 1 &&
                    !blocked[index(a.cell)] && !blocked[index(a.access)] && !rewards[index(a.cell)] &&
                    !reserved[index(a.cell)] && !encounters[index(a.cell)],
                Error::Clearance, "Reward or access is obstructed");
        rewards[index(a.cell)] = true;
    }
    Cell start{};
    for (unsigned i = 0; i < r.socketCount; ++i) {
        const auto& s = r.sockets[i];
        require(validName(s.id) && unsigned(s.outward) < 4, Error::Socket, "Invalid socket identity/facing");
        require(s.cell.x >= -1 && s.cell.y >= -1 && s.cell.x <= r.size.x && s.cell.y <= r.size.y,
                Error::Socket, "Socket outside wall ring");
        const auto a = s.cell + forward(s.outward) * -1;
        require(!inside(s.cell, r.size) && inside(a, r.size) && !blocked[index(a)] && !rewards[index(a)],
                Error::Socket, "Socket must face out through an unblocked room boundary");
        for (unsigned j = 0; j < i; ++j)
            require(s.id != r.sockets[j].id && !(s.cell == r.sockets[j].cell), Error::Socket,
                    "Duplicate socket");
        start = a;
    }
    Mask visited{};
    std::array<Cell, maxCells> queue{};
    unsigned head = 0, tail = 0;
    queue[tail++] = start;
    visited[index(start)] = true;
    while (head < tail) {
        const auto p = queue[head++];
        for (unsigned h = 0; h < 4; ++h) {
            const auto n = p + forward(Heading(h));
            if (inside(n, r.size) && !blocked[index(n)] && !visited[index(n)]) {
                visited[index(n)] = true;
                queue[tail++] = n;
            }
        }
    }
    for (int y = 0; y < r.size.y; ++y)
        for (int x = 0; x < r.size.x; ++x)
            require(blocked[index({x, y})] || visited[index({x, y})], Error::Disconnected,
                    "Room-local circulation disconnected");
}
void validate(const BlueprintCatalog& c) {
    require(c.kitCount > 0 && c.kitCount <= maxKits && c.roomCount > 0 && c.roomCount <= maxRooms,
            Error::Capacity, "Catalog capacity exceeded");
    for (unsigned i = 0; i < c.kitCount; ++i) {
        const auto& k = c.kits[i];
        require(validName(k.id) && k.size.x > 0 && k.size.y > 0 && k.size.x <= 4 && k.size.y <= 4 &&
                    std::isfinite(k.height) && k.height > 0 && k.height <= 2,
                Error::Content, "Invalid kit contract");
        for (unsigned j = 0; j < i; ++j)
            require(k.id != c.kits[j].id, Error::Content, "Duplicate kit ID");
    }
    for (unsigned i = 0; i < c.roomCount; ++i) {
        const auto& r = c.rooms[i];
        for (unsigned j = 0; j < i; ++j)
            require(r.id != c.rooms[j].id, Error::Content, "Duplicate room ID");
        try {
            validateRoom(c, r);
            for (unsigned t = 1; t < 4; ++t)
                if (r.rotations & (1u << t))
                    validateRoom(c, transform(r, t));
        } catch (const Failure& e) {
            throw Failure(e.code, std::string(r.id.begin(), std::find(r.id.begin(), r.id.end(), '\0')) +
                                      ": " + e.what());
        }
    }
}
ReviewScene assemble(const BlueprintCatalog& catalog, std::string_view selection, unsigned turns,
                     unsigned sockets, std::string_view assetPack) {
    validate(catalog);
    require(turns < 4, Error::Content, "Invalid assembly rotation");
    const bool pair = selection == "kitchen-hall";
    require(!pair || !sockets, Error::Socket, "Pair owns its shared/south socket selection");
    std::array<ReviewRoom, 2> rooms{};
    unsigned count = pair ? 2 : 1;
    if (pair) {
        rooms[0] = {catalog.rooms[catalog.room("kitchen").index], {1, 3}};
        rooms[1] = {catalog.rooms[catalog.room("mead-hall").index], {7, 1}};
        require(rooms[0].blueprint.size == Cell{5, 5} && rooms[1].blueprint.size == Cell{7, 7}, Error::Bounds,
                "Kitchen/hall pair requires the reviewed 5x5 and 7x7 footprints");
    } else
        rooms[0] = {catalog.rooms[catalog.room(selection).index], {1, 1}};
    // Three-cell padding holds a wall threshold, a vestibule and a solid outer border.
    const Cell baseSize = pair ? Cell{15, 9} : rooms[0].blueprint.size + Cell{2, 2};
    const Cell canvas = baseSize + Cell{6, 6}, finalSize = rotatedSize(canvas, turns);
    std::vector<std::string> rows(std::size_t(finalSize.y), std::string(std::size_t(finalSize.x), '#'));
    WorldDefinition world;
    world.assetPack = assetPack;
    world.surfaces.resize(std::size_t(finalSize.x * finalSize.y));
    FloorFixtures fixtures;
    fixtures.discovery = true;
    std::array<Cell, 8> thresholds{};
    unsigned thresholdCount = 0, sharedCount = 0;
    Player spawn{};
    bool spawned = false;
    std::array<Cell, 2> combat{};
    auto global = [&](Cell p) { return rotate(p, canvas, turns); };
    auto carve = [&](Cell p, unsigned ceiling) {
        require(inside(p, canvas) && p.x > 0 && p.y > 0 && p.x < canvas.x - 1 && p.y < canvas.y - 1,
                Error::Bounds, "Review assembly exceeds enclosed canvas");
        p = global(p);
        rows[std::size_t(p.y)][std::size_t(p.x)] = '.';
        world.surfaces[std::size_t(p.y * finalSize.x + p.x)].ceiling = std::int16_t(ceiling);
    };
    for (unsigned n = 0; n < count; ++n) {
        auto& rr = rooms[n];
        auto& r = rr.blueprint;
        rr.origin = rr.origin + Cell{3, 3};
        require((r.rotations & (1u << turns)) != 0, Error::Content, "Room disallows assembly rotation");
        for (int y = 0; y < r.size.y; ++y)
            for (int x = 0; x < r.size.x; ++x)
                if (!r.solid[index({x, y})])
                    carve(rr.origin + Cell{x, y}, r.ceiling);
        for (unsigned i = 0; i < r.placementCount; ++i) {
            const auto& p = r.placements[i];
            const auto& k = catalog.kits[p.kit.index];
            world.scenery.push_back({k.id.data(),
                                     global(rr.origin + p.anchor),
                                     {0, 0},
                                     rotate(p.facing, turns),
                                     unsigned(k.size.x),
                                     unsigned(k.size.y)});
        }
        // All authored reward candidates reserve space; the preview instantiates the first.
        const auto& reward = r.rewards[0];
        Decoration chest;
        chest.cell = global(rr.origin + reward.cell);
        chest.kind = DecorationKind::Chest;
        chest.face = rotate(Heading::North, turns);
        chest.container = std::uint8_t(n);
        chest.elevation = 0;
        chest.width = .76f;
        chest.height = .36f;
        chest.depth = .58f;
        world.decorations.push_back(chest);
        unsigned selected = sockets ? sockets : defaultSockets(r);
        require(selected < (1u << r.socketCount), Error::Socket, "Unknown selected socket bits");
        unsigned joined = 0;
        for (unsigned i = 0; i < r.socketCount; ++i) {
            const auto& s = r.sockets[i];
            const bool shared = pair && s.outward == (n ? Heading::West : Heading::East);
            const bool active = pair ? (shared || s.outward == Heading::South) : bool(selected & (1u << i));
            if (!active)
                continue;
            const auto p = rr.origin + s.cell;
            if (shared) {
                require(p == Cell{9, 8}, Error::Socket, "Kitchen/hall sockets do not meet");
                ++joined;
                ++sharedCount;
            }
            carve(p, 4);
            const auto exterior = p + forward(s.outward);
            if (!shared)
                carve(exterior, 4);
            const auto g = global(p);
            bool duplicate = false;
            for (unsigned j = 0; j < thresholdCount; ++j)
                duplicate |= thresholds[j] == g;
            if (!duplicate) {
                thresholds[thresholdCount++] = g;
                fixtures.entries[fixtures.count++] = {g, rotate(s.outward, turns), FixtureKind::Door};
            }
            if (!shared && (!spawned || (!n && s.outward == Heading::South))) {
                spawn = {global(exterior), rotate(s.outward, turns + 2)};
                spawned = true;
            }
        }
        require(!pair || joined == 1, Error::Socket, "Missing kitchen/hall join socket");
        if (n == count - 1) {
            const auto e = r.encounters[0];
            combat = {global(rr.origin + e.origin), global(rr.origin + e.origin + Cell{e.size.x - 1, 0})};
        }
    }
    require(spawned && (!pair || sharedCount == 2), Error::Socket, "Missing entrance or shared door");
    for (unsigned i = 0; i < count; ++i) {
        const auto old = rooms[i];
        rooms[i].blueprint = transform(old.blueprint, turns);
        const auto a = global(old.origin), b = global(old.origin + old.blueprint.size + Cell{-1, -1});
        rooms[i].origin = {std::min(a.x, b.x), std::min(a.y, b.y)};
    }
    Map map(rows, std::move(world));
    require(map.connected(spawn.cell), Error::Disconnected, "Assembled review map disconnected");
    for (unsigned i = 0; i < fixtures.count; ++i) {
        const auto& f = fixtures.entries[i];
        require(!map.movementBlocked(f.cell) && !map.movementBlocked(f.cell + forward(f.facing)) &&
                    !map.movementBlocked(f.cell + forward(f.facing) * -1),
                Error::Clearance, "Door threshold/approach blocked");
    }
    return {std::move(map), fixtures, spawn, rooms, count, combat, rotate(Heading::South, turns)};
}
} // namespace blobber::blueprints

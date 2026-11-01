#include "generation/floor.hpp"
#include <algorithm>
namespace blobber::generation {
const Recipe& encounterRecipe(EncounterRecipe id) noexcept {
    using A = ActorRecipe;
    static constexpr Recipe recipes[]{{"goblin_pair", {A::Goblin, A::Goblin}, 2},
                                      {"rat_pack", {A::Rat, A::Rat, A::Rat}, 3},
                                      {"undead_pair", {A::Skeleton, A::Zombie}, 2},
                                      {"goblin_support", {A::Goblin, A::Goblin, A::Archer, A::Shaman}, 4},
                                      {"bugbear_support", {A::Bugbear, A::Goblin, A::Shaman}, 3}};
    return recipes[unsigned(id) < 5 ? unsigned(id) : 0];
}
const char* speciesName(ActorRecipe id) noexcept {
    constexpr const char* names[]{"goblin",    "goblin.archer", "skeleton", "zombie",
                                  "giant.rat", "goblin_shaman", "bugbear"};
    return unsigned(id) < 7 ? names[unsigned(id)] : "";
}
namespace {
struct DepthBand {
    std::array<EncounterRecipe, 3> ordinary;
    unsigned count;
    EncounterRecipe optional, guardian;
};
const DepthBand& band(unsigned depth) noexcept {
    using R = EncounterRecipe;
    static constexpr DepthBand bands[]{
        {{R::GoblinPair, R::GoblinPair}, 2, R::RatPack, R::GoblinSupport},
        {{R::UndeadPair, R::GoblinSupport}, 2, R::GoblinPair, R::GoblinSupport},
        {{R::GoblinPair, R::UndeadPair, R::GoblinPair}, 3, R::UndeadPair, R::BugbearSupport}};
    return bands[depth >= 1 && depth <= 3 ? depth - 1 : 0];
}
Cell center(const Room& r) noexcept {
    return {4 + 9 * r.site.x, 4 + 9 * r.site.y};
}
Heading toward(Cell from, Cell to) noexcept {
    const auto d = to + from * -1;
    return d.x ? (d.x > 0 ? Heading::East : Heading::West) : (d.y > 0 ? Heading::South : Heading::North);
}
bool path(FloorLayout& f, unsigned from, unsigned to) noexcept {
    std::array<unsigned, maxRooms> parents{}, queue{};
    parents.fill(maxRooms);
    unsigned head = 0, tail = 1;
    queue[0] = from;
    parents[from] = from;
    while (head < tail) {
        const auto here = queue[head++];
        // Stable room-ID order, independent of connection iteration order.
        for (unsigned next = 0; next < f.roomCount; ++next) {
            bool edge = false;
            for (unsigned i = 0; i < f.connectionCount; ++i) {
                const auto e = f.connections[i];
                edge |= (e.a == here && e.b == next) || (e.b == here && e.a == next);
            }
            if (edge && parents[next] == maxRooms) {
                parents[next] = here;
                queue[tail++] = next;
            }
        }
    }
    if (parents[to] == maxRooms)
        return false;
    std::array<unsigned, maxRooms> reverse{};
    unsigned count = 0;
    for (auto n = to; n != from; n = parents[n])
        reverse[count++] = n;
    if (f.routeCount + count > f.route.size())
        return false;
    while (count)
        f.route[f.routeCount++] = std::uint8_t(reverse[--count]);
    return true;
}
} // namespace
void placePopulation(FloorLayout& f, GenerationRequest, unsigned, bool) noexcept {
    f.populationCount = f.encounterCount = f.routeCount = 0;
    unsigned entry = maxRooms, approach = maxRooms, optional = maxRooms, key = maxRooms;
    for (unsigned i = 0; i < f.roomCount; ++i) {
        const auto& r = f.rooms[i];
        if (r.role == Role::Entrance)
            entry = i;
        if (r.role == Role::Approach)
            approach = i;
        if (r.role == Role::Optional && optional == maxRooms)
            optional = i;
        for (unsigned j = 0; j < f.fixtures.count; ++j)
            if (f.fixtures.entries[j].kind == FixtureKind::Key && f.fixtures.entries[j].cell == r.anchor)
                key = i;
    }
    if (entry == maxRooms || approach == maxRooms || optional == maxRooms)
        return;
    f.route[f.routeCount++] = std::uint8_t(entry);
    if (key < maxRooms && !path(f, entry, key))
        return;
    if (!path(f, key < maxRooms ? key : entry, approach))
        return;
    const auto add = [&](unsigned room, EncounterRecipe recipe, EncounterRole role, Heading facing) {
        if (f.encounterCount == f.encounters.size())
            return;
        const auto& r = encounterRecipe(recipe);
        if (f.populationCount + r.count > f.population.size())
            return;
        f.encounters[f.encounterCount++] = {recipe, role, std::uint8_t(room), std::uint8_t(f.populationCount),
                                            std::uint8_t(r.count)};
        for (unsigned n = 0; n < r.count; ++n) {
            const auto i = f.populationCount++;
            f.population[i] = {f.rooms[room].anchor,
                               rotate({int(n % 2), int(n / 2)}, 2, unsigned(facing)),
                               facing,
                               r.members[n],
                               std::uint8_t(i),
                               role == EncounterRole::Guardian};
        }
    };
    std::array<bool, maxRooms> used{};
    const auto& depth = band(f.depth);
    const unsigned wanted = depth.count;
    for (unsigned i = 1; i < f.routeCount && f.encounterCount < wanted; ++i) {
        const unsigned room = f.route[i];
        if (f.rooms[room].role != Role::Junction || room == key || used[room])
            continue;
        used[room] = true;
        add(room, depth.ordinary[f.encounterCount], EncounterRole::Direct,
            toward(f.rooms[room].site, f.rooms[f.route[i - 1]].site));
    }
    if (f.encounterCount != wanted)
        return;
    unsigned neighbor = optional;
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto e = f.connections[i];
        if (e.a == optional)
            neighbor = e.b;
        if (e.b == optional)
            neighbor = e.a;
    }
    add(optional, depth.optional, EncounterRole::Optional,
        toward(f.rooms[optional].site, f.rooms[neighbor].site));
    f.optionalSupply = f.rooms[optional].anchor;
    const auto& door = f.fixtures.entries[f.approachDoor];
    f.supply = door.cell + forward(door.facing) * -1;
    add(approach, depth.guardian, EncounterRole::Guardian, Heading((unsigned(door.facing) + 2) % 4));
    // One ordinary door per populated-room connection; existing special doors win.
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto e = f.connections[i];
        bool populated = false;
        for (unsigned j = 0; j < f.encounterCount; ++j)
            populated |= f.encounters[j].role != EncounterRole::Guardian &&
                         (f.encounters[j].room == e.a || f.encounters[j].room == e.b);
        if (!populated)
            continue;
        const auto a = center(f.rooms[e.a]), b = center(f.rooms[e.b]);
        const auto h = toward(a, b);
        // Adjacent rims sit at steps 3/6. Either orientation's corridor center is safe.
        const auto c = a + forward(h) * 4;
        bool existing = false;
        for (unsigned j = 0; j < f.fixtures.count; ++j) {
            const auto p = f.fixtures.entries[j].cell;
            existing |= p == a + forward(h) * 4 || p == a + forward(h) * 5;
        }
        if (!existing && f.fixtures.count < fixtureCapacity)
            f.fixtures.entries[f.fixtures.count++] = {c, h, FixtureKind::Door, 0};
    }
}
Failure validatePopulation(const FloorLayout& f) noexcept {
    const auto& depth = band(f.depth);
    if (f.depth < 1 || f.depth > 3 || f.encounterCount != depth.count + 2 || !f.routeCount ||
        f.routeCount > f.route.size() || f.populationCount > f.population.size())
        return {Error::Population};
    for (unsigned i = 0; i < f.routeCount; ++i) {
        if (f.route[i] >= f.roomCount)
            return {Error::Population};
        if (i) {
            bool edge = false;
            for (unsigned j = 0; j < f.connectionCount; ++j) {
                const auto e = f.connections[j];
                edge |= (e.a == f.route[i - 1] && e.b == f.route[i]) ||
                        (e.b == f.route[i - 1] && e.a == f.route[i]);
            }
            if (!edge)
                return {Error::Population};
        }
    }
    if (f.rooms[f.route[0]].role != Role::Entrance ||
        f.rooms[f.route[f.routeCount - 1]].role != Role::Approach)
        return {Error::Population};
    unsigned member = 0;
    for (unsigned i = 0; i < f.encounterCount; ++i) {
        const auto& e = f.encounters[i];
        if (unsigned(e.recipe) > 4 || unsigned(e.role) > 2 || e.room >= f.roomCount || e.first != member ||
            e.count != encounterRecipe(e.recipe).count || member + e.count > f.populationCount)
            return {Error::Population};
        const auto expectedRole = i + 1 == f.encounterCount   ? EncounterRole::Guardian
                                  : i + 2 == f.encounterCount ? EncounterRole::Optional
                                                              : EncounterRole::Direct;
        const auto expectedRecipe = expectedRole == EncounterRole::Guardian   ? depth.guardian
                                    : expectedRole == EncounterRole::Optional ? depth.optional
                                                                              : depth.ordinary[i];
        if (e.role != expectedRole || e.recipe != expectedRecipe)
            return {Error::Population};
        for (unsigned j = 0; j < i; ++j)
            if (e.room == f.encounters[j].room)
                return {Error::Population};
        const auto roomRole = f.rooms[e.room].role;
        if ((e.role == EncounterRole::Direct &&
             (roomRole != Role::Junction || std::find(f.route.begin(), f.route.begin() + f.routeCount,
                                                      e.room) == f.route.begin() + f.routeCount)) ||
            (e.role == EncounterRole::Optional && roomRole != Role::Optional) ||
            (e.role == EncounterRole::Guardian && roomRole != Role::Approach))
            return {Error::Population};
        for (unsigned j = 0; j < f.fixtures.count; ++j)
            if (f.fixtures.entries[j].kind == FixtureKind::Key &&
                f.fixtures.entries[j].cell == f.rooms[e.room].anchor)
                return {Error::Population};
        for (unsigned n = 0; n < e.count; ++n) {
            const auto& p = f.population[member++];
            if (p.recipe != encounterRecipe(e.recipe).members[n] || !(p.cell == f.rooms[e.room].anchor) ||
                unsigned(p.facing) > 3 || p.facing != f.population[e.first].facing ||
                !(p.subcell == rotate({int(n % 2), int(n / 2)}, 2, unsigned(p.facing))) ||
                p.guardian != (e.role == EncounterRole::Guardian))
                return {Error::Population};
        }
    }
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto edge = f.connections[i];
        bool populated = false;
        for (unsigned j = 0; j < f.encounterCount; ++j)
            populated |= f.encounters[j].role != EncounterRole::Guardian &&
                         (f.encounters[j].room == edge.a || f.encounters[j].room == edge.b);
        if (!populated)
            continue;
        const auto a = center(f.rooms[edge.a]), b = center(f.rooms[edge.b]);
        const auto d = forward(toward(a, b));
        bool door = false;
        for (unsigned j = 0; j < f.fixtures.count; ++j)
            door |= f.fixtures.entries[j].kind == FixtureKind::Door &&
                    (f.fixtures.entries[j].cell == a + d * 4 || f.fixtures.entries[j].cell == a + d * 5);
        if (!door)
            return {Error::Population};
    }
    for (unsigned i = 0; i < f.fixtures.count; ++i) {
        const auto& fixture = f.fixtures.entries[i];
        if (fixture.kind == FixtureKind::Key &&
            std::none_of(f.route.begin(), f.route.begin() + f.routeCount,
                         [&](auto room) { return f.rooms[room].anchor == fixture.cell; }))
            return {Error::Population};
        if (fixture.cell == f.supply || fixture.cell == f.optionalSupply)
            return {Error::Population};
    }
    const auto& reserve = f.encounters[f.encounterCount - 2];
    if (!(f.optionalSupply == f.rooms[reserve.room].anchor))
        return {Error::Population};
    if (member != f.populationCount ||
        !(f.supply ==
          f.fixtures.entries[f.approachDoor].cell + forward(f.fixtures.entries[f.approachDoor].facing) * -1))
        return {Error::Population};
    return {};
}
} // namespace blobber::generation

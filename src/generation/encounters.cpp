#include "game/perception.hpp"
#include "generation/encounter_planning.hpp"
#include <algorithm>
namespace blobber::generation {
const Recipe& encounterRecipe(EncounterRecipe id) noexcept {
    using A = ActorRecipe;
    static constexpr Recipe recipes[]{
        {"goblin_pair", {A::Goblin, A::Goblin}, 2},
        {"rat_pack", {A::Rat, A::Rat, A::Rat}, 3},
        {"undead_pair", {A::Skeleton, A::Zombie}, 2},
        {"goblin_support", {A::Goblin, A::Goblin, A::Archer, A::Shaman}, 4},
        {"bugbear_support", {A::Bugbear, A::Goblin, A::Shaman}, 3},
        {"mixed_patrol", {A::Goblin, A::Goblin, A::Archer}, 3},
        {"goblin_guardians", {A::GoblinCaptain, A::Goblin, A::Archer, A::Archer, A::Shaman}, 5},
        {"bugbear_guardians", {A::BugbearCaptain, A::Goblin, A::Goblin, A::Archer, A::Shaman}, 5},
        {"final_guardians", {A::BugbearCommander, A::Goblin, A::Goblin, A::Goblin, A::Archer, A::Shaman}, 6}};
    return recipes[unsigned(id) <= unsigned(EncounterRecipe::FinalGuardians) ? unsigned(id) : 0];
}
const char* speciesName(ActorRecipe id) noexcept {
    constexpr const char* names[]{"goblin",          "goblin.archer",    "skeleton", "zombie",
                                  "giant.rat",       "goblin_shaman",    "bugbear",  "goblin.captain",
                                  "bugbear.captain", "bugbear.commander"};
    return unsigned(id) <= unsigned(ActorRecipe::BugbearCommander) ? names[unsigned(id)] : "";
}
const DepthBand& encounterBand(unsigned depth) noexcept {
    using R = EncounterRecipe;
    static constexpr DepthBand bands[]{
        {{R::GoblinPair, R::MixedPatrol, R::GoblinPair, R::MixedPatrol},
         4,
         1,
         R::RatPack,
         R::GoblinGuardians},
        {{R::UndeadPair, R::MixedPatrol, R::UndeadPair, R::MixedPatrol},
         4,
         2,
         R::GoblinPair,
         R::BugbearGuardians},
        {{R::UndeadPair, R::GoblinSupport, R::UndeadPair, R::GoblinPair, R::UndeadPair},
         5,
         2,
         R::UndeadPair,
         R::FinalGuardians}};
    const auto spec = floorSpec(depth);
    return bands[spec ? spec->populationBand - 1 : 0];
}
void placePopulation(FloorLayout& f, GenerationRequest, unsigned, bool) noexcept {
    f.populationCount = f.encounterCount = f.routeCount = 0;
    const auto& depth = encounterBand(f.depth);
    std::array<unsigned, 7> sites{};
    if (!planEncounterSites(f, depth, sites))
        return;
    for (unsigned i = 0; i < depth.count + depth.optionalCount; ++i) {
        const auto room = sites[i];
        unsigned previous = maxRooms;
        if (i < depth.count) {
            for (unsigned j = 1; j < f.routeCount; ++j)
                if (f.route[j] == room) {
                    previous = f.route[j - 1];
                    break;
                }
        } else {
            for (unsigned j = 0; j < f.connectionCount; ++j) {
                const auto& e = f.connections[j];
                const auto next = e.a == room ? e.b : e.b == room ? e.a : maxRooms;
                if (next < previous)
                    previous = next;
            }
        }
        const auto* edge = connectionBetween(f, room, previous);
        if (!edge ||
            !placeEncounterFormation(f, room, i < depth.count ? depth.ordinary[i] : depth.optional,
                                     i < depth.count ? EncounterRole::Direct : EncounterRole::Optional,
                                     incomingFacing(*edge, room)))
            return;
    }
    f.optionalSupply = f.rooms[sites[depth.count]].anchor;
    const auto& door = f.fixtures.entries[f.approachDoor];
    f.supply = door.cell + forward(door.facing) * -1;
    const auto room = f.route[f.routeCount - 1];
    const auto* edge = connectionBetween(f, sectorJunction(f), room);
    if (!edge || !placeEncounterFormation(f, room, depth.guardian, EncounterRole::Guardian,
                                          incomingFacing(*edge, room)))
        return;
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto e = f.connections[i];
        bool populated = false;
        for (unsigned j = 0; j < f.encounterCount; ++j)
            populated |= f.encounters[j].role != EncounterRole::Guardian &&
                         (f.encounters[j].room == e.a || f.encounters[j].room == e.b);
        if (!populated)
            continue;
        const auto cell = threshold(e);
        bool existing = false;
        for (unsigned j = 0; j < f.fixtures.count; ++j)
            existing |= f.fixtures.entries[j].cell == cell;
        if (!existing) {
            if (f.fixtures.count == fixtureCapacity)
                return;
            f.fixtures.entries[f.fixtures.count++] = {cell, e.facingA, FixtureKind::Door, 0};
        }
    }
}
Failure validatePopulation(const FloorLayout& f) noexcept {
    const auto& depth = encounterBand(f.depth);
    if (f.depth < 1 || f.depth > 3 || f.encounterCount != depth.count + depth.optionalCount + 1 ||
        !f.routeCount || f.routeCount > f.route.size() || f.populationCount > f.population.size())
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
    bool keyHeld = f.depth == 1;
    unsigned visited = 0, nextDirect = 0;
    for (unsigned i = 0; i < f.routeCount; ++i) {
        const auto room = f.route[i];
        for (unsigned j = 0; j < f.fixtures.count; ++j)
            if (f.fixtures.entries[j].kind == FixtureKind::Key &&
                f.fixtures.entries[j].cell == f.rooms[room].anchor)
                keyHeld = true;
        if (room == sectorJunction(f) && !keyHeld)
            return {Error::Population};
        for (unsigned j = 0; j < depth.count; ++j)
            if (f.encounters[j].room == room && !(visited & (1u << j))) {
                if (j != nextDirect++)
                    return {Error::Population};
                visited |= 1u << j;
            }
    }
    if (nextDirect != depth.count)
        return {Error::Population};
    unsigned member = 0;
    for (unsigned i = 0; i < f.encounterCount; ++i) {
        const auto& e = f.encounters[i];
        if (unsigned(e.recipe) > unsigned(EncounterRecipe::FinalGuardians) || unsigned(e.role) > 2 ||
            e.room >= f.roomCount || e.first != member || e.count != encounterRecipe(e.recipe).count ||
            member + e.count > f.populationCount)
            return {Error::Population};
        const auto expectedRole = i < depth.count                         ? EncounterRole::Direct
                                  : i < depth.count + depth.optionalCount ? EncounterRole::Optional
                                                                          : EncounterRole::Guardian;
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
            (e.role == EncounterRole::Optional &&
             (roomRole != Role::Optional && roomRole != Role::Junction)) ||
            (e.role == EncounterRole::Guardian && roomRole != Role::Approach))
            return {Error::Population};
        if (e.role == EncounterRole::Optional && std::find(f.route.begin(), f.route.begin() + f.routeCount,
                                                           e.room) != f.route.begin() + f.routeCount)
            return {Error::Population};
        for (unsigned n = 0; n < e.count; ++n) {
            const auto& p = f.population[member++];
            if (p.recipe != encounterRecipe(e.recipe).members[n] || regionAt(f, p.cell) != e.room ||
                unsigned(p.facing) > 3 || p.facing != f.population[e.first].facing ||
                std::abs(p.cell.x - f.rooms[e.room].anchor.x) > 1 ||
                std::abs(p.cell.y - f.rooms[e.room].anchor.y) > 1 ||
                p.guardian != (e.role == EncounterRole::Guardian))
                return {Error::Population};
            for (unsigned k = 0; k < e.count; ++k) {
                const auto other = f.population[e.first + k].cell;
                if (std::abs(p.cell.x - other.x) + std::abs(p.cell.y - other.y) > 3)
                    return {Error::Population};
            }
            unsigned sameCell = 0;
            for (unsigned k = 0; k < e.count; ++k)
                sameCell += f.population[e.first + k].cell == p.cell;
            if (sameCell > 2 ||
                (sameCell > 1 && (p.recipe == ActorRecipe::Archer || p.recipe == ActorRecipe::Shaman)))
                return {Error::Population};
            for (unsigned k = 0; k < f.connectionCount; ++k) {
                const auto& edge = f.connections[k];
                if ((edge.a == e.room && p.cell == edge.socketA + forward(edge.facingA) * -1) ||
                    (edge.b == e.room && p.cell == edge.socketB + forward(edge.facingB) * -1))
                    return {Error::Population};
            }
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
        bool door = false;
        for (unsigned j = 0; j < f.fixtures.count; ++j)
            door |= f.fixtures.entries[j].kind == FixtureKind::Door &&
                    f.fixtures.entries[j].cell == threshold(edge);
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
    const auto& reserve = f.encounters[depth.count];
    if (!(f.optionalSupply == f.rooms[reserve.room].anchor))
        return {Error::Population};
    if (member != f.populationCount ||
        !(f.supply ==
          f.fixtures.entries[f.approachDoor].cell + forward(f.fixtures.entries[f.approachDoor].facing) * -1))
        return {Error::Population};
    return {};
}
} // namespace blobber::generation

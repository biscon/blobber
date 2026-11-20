#include "generation/cave_population.hpp"
#include "generation/blueprint_rooms.hpp"
#include "generation/encounter_planning.hpp"
#include <algorithm>
#include <stdexcept>

namespace blobber::generation {
namespace {
using R = EncounterRecipe;
using S = CaveSite;
constexpr unsigned networkRoom = noRegion;
struct Group {
    R recipe;
    unsigned room;
    EncounterRole role = EncounterRole::Direct;
};
constexpr auto optional = EncounterRole::Optional, guardian = EncounterRole::Guardian;
constexpr Group seep[]{{R::SpiderSolo, 2},
                       {R::MixedPatrol, networkRoom},
                       {R::SpiderTrio, networkRoom},
                       {R::CaptainPatrol, networkRoom},
                       {R::SpiderPair, 5, optional},
                       {R::MixedPatrol, 15, optional},
                       {R::SpiderGuard, 4, guardian}};
constexpr Group galleries[]{{R::NewtSolo, 1},
                            {R::Banks, 2},
                            {R::MixedPatrol, networkRoom},
                            {R::GalleryNest, networkRoom},
                            {R::NewtPatrol, networkRoom},
                            {R::NewtPatrol, 5, optional},
                            {R::MixedPatrol, 15, optional},
                            {R::GalleryGuard, 4, guardian}};
constexpr Group vault[]{{R::SpiderTrio, 2},
                        {R::MixedPatrol, networkRoom},
                        {R::SpiderNewtPack, networkRoom},
                        {R::GoblinSentries, networkRoom},
                        {R::VaultApproach, 3},
                        {R::SpiderTrio, 5, optional},
                        {R::WebLookout, 15, optional},
                        {R::BroodVault, 4, guardian}};
struct Groups {
    const Group* data;
    unsigned count;
};
Groups groups(unsigned depth) {
    if (depth == 4)
        return {seep, std::size(seep)};
    if (depth == 5)
        return {galleries, std::size(galleries)};
    return {vault, std::size(vault)};
}
// Walk the decorated map from the teaching rooms to the objective, keeping both
// optional pockets closed. The middle encounter belongs on the gallery a party
// actually crosses. Equal-length routes use the same N/E/S/W order as review.
bool networkTour(CaveFloor& cave, std::array<unsigned, 3>& fights) {
    auto& f = cave.layout;
    const auto start = f.spawn.cell, end = f.rooms[4].anchor;
    auto index = [&](Cell c) { return unsigned(c.y * f.width + c.x); };
    auto cell = [&](unsigned k) { return Cell{int(k) % f.width, int(k) / f.width}; };
    std::array<std::uint16_t, maxCells> parent{}, queue{};
    parent.fill(UINT16_MAX);
    parent[index(start)] = std::uint16_t(index(start));
    unsigned head = 0, tail = 0;
    queue[tail++] = std::uint16_t(index(start));
    while (head < tail && parent[index(end)] == UINT16_MAX) {
        const auto previous = queue[head++];
        const auto here = cell(previous);
        for (unsigned h = 0; h < 4; ++h) {
            const auto next = here + forward(Heading(h));
            if (!cave.map.canStep(here, next))
                continue;
            const auto room = regionAt(f, next);
            const auto k = index(next);
            if (room == 5 || room == 15 || parent[k] != UINT16_MAX || next == f.fixtures.entries[0].cell ||
                next == f.fixtures.entries[3].cell)
                continue;
            parent[k] = previous;
            queue[tail++] = std::uint16_t(k);
        }
    }
    if (parent[index(end)] == UINT16_MAX)
        return false;
    unsigned count = 0;
    for (unsigned k = index(end); k != index(start); k = parent[k])
        queue[count++] = std::uint16_t(k);
    f.route[f.routeCount++] = 0;
    std::array<unsigned, maxRooms> network{}, distance{};
    unsigned visited = 0;
    for (unsigned step = 0; step < count; ++step) {
        const auto room = regionAt(f, cell(queue[count - 1 - step]));
        if (room == noRegion || room == f.route[f.routeCount - 1])
            continue;
        if (f.routeCount == f.route.size())
            return false;
        f.route[f.routeCount++] = std::uint8_t(room);
        if (room >= 6 && room < 15 &&
            std::find(network.begin(), network.begin() + visited, room) == network.begin() + visited) {
            network[visited] = room;
            distance[visited++] = step;
        }
    }
    if (visited < 3)
        return false;
    unsigned middle = 1;
    const auto half = int(distance[0] + distance[visited - 1]);
    for (unsigned n = 2; n + 1 < visited; ++n)
        if (std::abs(2 * int(distance[n]) - half) < std::abs(2 * int(distance[middle]) - half))
            middle = n;
    fights = {network[0], network[middle], network[visited - 1]};
    return true;
}
bool specialPlacement(const CaveFloor& floor, R recipe, unsigned n, Cell& cell, Cell& subcell,
                      Heading& face) {
    const auto& s = floor.sites;
    if (recipe == R::SpiderSolo) {
        cell = s[S::SpiderArch];
        subcell = {0, 1};
        face = Heading::West;
    } else if (recipe == R::NewtSolo) {
        cell = s[S::SoloNewt];
        subcell = {0, 1};
        face = Heading::South;
    } else if (recipe == R::Banks) {
        cell = n ? s[S::SouthContact] + Cell{2, -1} : s[S::NorthContact] + Cell{2, 1};
        subcell = {0, int(n)};
        face = n ? Heading::West : Heading::East;
    } else if (recipe == R::SpiderGuard || recipe == R::GalleryGuard) {
        // Two webbed flanks in the Seep; newts cover the Galleries' shoulders
        // while the forward spider can make contact before the heavy bites.
        constexpr Cell flanks[]{{-1, -1}, {1, -1}, {-1, 0}, {1, 0}};
        constexpr Cell shoulders[]{{-1, 0}, {1, 0}, {0, -1}, {0, 0}};
        cell = floor.layout.rooms[4].anchor + (recipe == R::SpiderGuard ? flanks[n] : shoulders[n]);
        subcell = {int(n % 2), 0};
        face = Heading::North;
    } else if (recipe == R::BroodVault) {
        constexpr S parts[]{S::Mother, S::WestClutch, S::EastClutch, S::WestHatch, S::EastHatch};
        cell = s[parts[n]];
        subcell = {int(n == 1 || n == 3), 0};
        face = n ? Heading::South : Heading::North;
    } else
        return false;
    return true;
}
} // namespace

Failure planCavePopulation(CaveFloor& floor) {
    auto& f = floor.layout;
    const auto band = groups(f.depth);
    std::array<unsigned, 3> fightRooms{};
    if (!networkTour(floor, fightRooms))
        return {Error::Progression};
    unsigned nextNetwork = 0;
    std::array<unsigned, 8> rooms{};
    for (unsigned i = 0; i < band.count; ++i) {
        const auto r = band.data[i].room;
        rooms[i] = r == networkRoom ? fightRooms.at(nextNetwork++) : r;
    }
    auto& defs = f.fixtures;
    f.supply = f.depth == 4 ? floor.sites[S::Camp] : f.rooms[0].anchor;
    if (f.depth < 6)
        f.guardianReward = f.rooms[4].anchor;
    f.guardianGate = std::uint8_t(defs.count);
    defs.entries.at(defs.count++) = {floor.sites[S::Goal], Heading::South, FixtureKind::GuardianGate};
    // Close the individual galleries at their exterior sockets. The two approaches
    // within the pool room remain free; hearing and sight use normal door geometry.
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto& e = f.connections[i];
        bool occupiedRoom = false;
        for (unsigned j = 0; j < band.count; ++j)
            occupiedRoom |= e.a == rooms[j] || e.b == rooms[j];
        if (!occupiedRoom || i == 0 || i == 3 || (f.depth == 5 && i == 1))
            continue;
        defs.entries.at(defs.count++) = {e.socketA, e.facingA, FixtureKind::Door};
    }
    FloorGeometry geometry(floor.map, defs); // Check all dressed thresholds before publishing the candidate.
    BlueprintReservations reservations;
    for (int y = 0; y < f.height; ++y)
        for (int x = 0; x < f.width; ++x)
            reservations.furnished[y * f.width + x] = floor.map.npcBlocked({x, y});
    for (unsigned i = 0; i < band.count; ++i) {
        const auto& group = band.data[i];
        const auto room = rooms[i];
        const auto recipe = group.recipe;
        const auto& r = encounterRecipe(recipe);
        const auto first = f.populationCount;
        if (first + r.count > f.population.size() || f.encounterCount == f.encounters.size())
            return {Error::Capacity};
        Cell cell, subcell;
        Heading face;
        if (specialPlacement(floor, recipe, 0, cell, subcell, face)) {
            f.encounters[f.encounterCount++] = {recipe, group.role, std::uint8_t(room), std::uint8_t(first),
                                                std::uint8_t(r.count)};
            for (unsigned n = 0; n < r.count; ++n) {
                specialPlacement(floor, recipe, n, cell, subcell, face);
                if (floor.map.npcBlocked(cell) || defs.npcExcluded[cell.y * f.width + cell.x])
                    return {Error::Placement, int(room)};
                f.population[f.populationCount++] = {
                    cell, subcell, face, r.members[n], std::uint8_t(first + n), group.role == guardian};
            }
        } else {
            bool placed = false;
            for (auto h : {Heading::West, Heading::North, Heading::South, Heading::East}) {
                if (placeEncounterFormation(f, room, recipe, group.role, h, &reservations)) {
                    placed = true;
                    break;
                }
            }
            if (!placed)
                return {Error::Placement, int(room)};
        }
    }
    return validateCavePopulation(floor);
}

Failure validateCavePopulation(const CaveFloor& floor) {
    const auto& f = floor.layout;
    if (f.depth < 4 || f.depth > 6)
        return {Error::Request};
    const auto band = groups(f.depth);
    if (f.encounterCount != band.count || f.populationCount > f.population.size() ||
        f.guardianGate >= f.fixtures.count || f.fixtures.count > fixtureCapacity ||
        (f.depth == 6 && !floor.hangingClutches))
        return {Error::Population};
    const auto& gate = f.fixtures.entries[f.guardianGate];
    if (gate.kind != FixtureKind::GuardianGate || !(gate.cell == floor.sites[S::Goal]) ||
        !floor.map.solid(gate.cell + Cell{-1, 0}) || !floor.map.solid(gate.cell + Cell{1, 0}))
        return {Error::Progression};
    unsigned member = 0;
    std::bitset<maxRooms> occupiedRooms;
    for (unsigned i = 0; i < band.count; ++i) {
        const auto& e = f.encounters[i];
        const auto& g = band.data[i];
        const auto& recipe = encounterRecipe(g.recipe);
        if (e.recipe != g.recipe || e.role != g.role || e.first != member || e.count != recipe.count ||
            member + e.count > f.populationCount || e.room >= f.roomCount || occupiedRooms[e.room] ||
            (g.room != networkRoom && e.room != g.room))
            return {Error::Population};
        occupiedRooms.set(e.room);
        for (unsigned n = 0; n < e.count; ++n, ++member) {
            const auto& p = f.population[member];
            if (p.id != member || p.recipe != recipe.members[n] || p.guardian != (e.role == guardian) ||
                regionAt(f, p.cell) != e.room || floor.map.npcBlocked(p.cell) ||
                f.fixtures.npcExcluded[p.cell.y * f.width + p.cell.x] || unsigned(p.subcell.x) > 1 ||
                unsigned(p.subcell.y) > 1 || unsigned(p.facing) > 3)
                return {Error::Placement, int(e.room)};
            for (unsigned j = 0; j < member; ++j)
                if (f.population[j].cell == p.cell && f.population[j].subcell == p.subcell)
                    return {Error::Placement, int(e.room)};
            Cell cell, subcell;
            Heading heading;
            if (specialPlacement(floor, e.recipe, n, cell, subcell, heading) &&
                (!(cell == p.cell) || !(subcell == p.subcell) || heading != p.facing))
                return {Error::Placement, int(e.room)};
        }
    }
    return member == f.populationCount ? Failure{} : Failure{Error::Population};
}

PreparedPopulation prepareCavePopulation(const CaveFloor& floor, std::uint32_t seed,
                                         const rules::Catalog& catalog,
                                         const std::vector<std::string>& types) {
    const auto& f = floor.layout;
    const auto band = groups(f.depth);
    if (validateCavePopulation(floor))
        throw std::invalid_argument("Cave is missing its prepared population");
    RecoveryLevel recovery;
    FloorObjectiveDefinition objective;
    objective.approachDoor = f.approachDoor;
    objective.gate = f.guardianGate;
    objective.lootSeed = std::uint32_t(seedFor({seed, f.depth}, Domain::Reward));
    std::vector<NpcInstance> actors;
    actors.reserve(f.populationCount);
    for (unsigned i = 0; i < band.count; ++i) {
        const auto& e = f.encounters[i];
        if (e.recipe == R::BroodVault) {
            placeBroodVault(floor.sites, types, catalog, actors);
            objective.gatedVault = true;
        } else {
            for (unsigned n = e.first; n < e.first + e.count; ++n) {
                actors.push_back(bindPopulationActor(f.population[n], catalog, types));
                if (e.role == guardian)
                    objective.members[objective.count++] = std::uint16_t(n);
            }
        }
    }
    auto supply = [&](Cell cell, const char* name, unsigned amount) {
        auto item = catalog.findItem(name);
        if (!catalog.item(item) || floor.map.movementBlocked(cell))
            throw std::invalid_argument("Invalid cave supply");
        recovery.ground.at(recovery.groundCount++) = {cell, item, std::uint16_t(amount)};
    };
    const auto camp = f.supply;
    if (f.depth == 4)
        supply(camp, "antitoxin", 1);
    supply(camp, "healing_potion", 1);
    supply(camp, "mana_potion", 1);
    supply(camp, "arrow", 10);
    supply(floor.sites[S::Optional], "antitoxin", 1);
    supply(floor.sites[S::Lookout], "healing_potion", 1);
    if (f.depth != 6)
        objective.rewards[objective.rewardCount++] = {f.guardianReward, catalog.findItem("camp_kit"), 1};
    return {NpcWorld(floor.map, f.spawn.cell, types, std::move(actors)), recovery, objective};
}
} // namespace blobber::generation

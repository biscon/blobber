#include "generation/dressing.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
namespace blobber::generation {
namespace {
struct Candidate {
    Cell cell;
    Heading face;
    int score;
};
struct Dressing {
    FloorLayout& f;
    WorldDefinition& world;
    std::array<bool, maxCells> protectedCells{}, occupied{};
    std::uint64_t rng;
    unsigned next(unsigned n) {
        rng ^= rng >> 12;
        rng ^= rng << 25;
        rng ^= rng >> 27;
        return unsigned((rng * 2685821657736338717ull) >> 32) % n;
    }
    unsigned index(Cell c) const { return unsigned(c.y * f.width + c.x); }
    void protect(Cell c) {
        if (inside(f, c))
            protectedCells[index(c)] = true;
    }
    bool free(Cell c) const { return floorCell(f, c) && !occupied[index(c)]; }
    bool connected() const {
        std::array<bool, maxCells> seen{};
        std::array<Cell, maxCells> queue{};
        unsigned head = 0, tail = 0;
        queue[tail++] = f.spawn.cell;
        seen[index(f.spawn.cell)] = true;
        while (head < tail) {
            const auto c = queue[head++];
            for (unsigned h = 0; h < 4; ++h) {
                const auto n = c + forward(Heading(h));
                if (free(n) && !seen[index(n)]) {
                    seen[index(n)] = true;
                    queue[tail++] = n;
                }
            }
        }
        for (int y = 0; y < f.height; ++y)
            for (int x = 0; x < f.width; ++x)
                if (free({x, y}) && !seen[index({x, y})])
                    return false;
        return true;
    }
    bool admit(const Decoration& d) {
        for (unsigned i = 0; i < d.footprintCount(); ++i) {
            const auto c = d.footprint(i);
            if (!free(c) || protectedCells[index(c)])
                return false;
        }
        for (unsigned i = 0; i < d.footprintCount(); ++i)
            occupied[index(d.footprint(i))] = true;
        const bool ok = connected();
        if (!ok)
            for (unsigned i = 0; i < d.footprintCount(); ++i)
                occupied[index(d.footprint(i))] = false;
        return ok;
    }
    bool chest(unsigned id, Cell old, bool sealed) {
        unsigned region = regionAt(f, old);
        if (sealed)
            region = regionAt(f, f.guardianApproach);
        if (region == noRegion)
            return false;
        std::array<Candidate, maxCells> candidates{};
        unsigned count = 0;
        for (int y = 1; y < f.height - 1; ++y)
            for (int x = 1; x < f.width - 1; ++x) {
                const Cell c{x, y};
                if (regionAt(f, c) != region || protectedCells[index(c)])
                    continue;
                for (unsigned h = 0; h < 4; ++h)
                    if (!floorCell(f, c + forward(Heading(h)))) {
                        candidates[count++] = {c, Heading(h),
                                               (std::abs(c.x - old.x) + std::abs(c.y - old.y)) * 16 +
                                                   int(next(16))};
                        break;
                    }
            }
        std::sort(candidates.begin(), candidates.begin() + count, [](const auto& a, const auto& b) {
            return a.score != b.score     ? a.score < b.score
                   : a.cell.y != b.cell.y ? a.cell.y < b.cell.y
                                          : a.cell.x < b.cell.x;
        });
        for (unsigned n = 0; n < std::min(count, 32u); ++n) {
            const auto c = candidates[n];
            Decoration d{c.cell, c.face, DecorationKind::Chest, 0, .5f, 0, .76f, .36f, .58f};
            d.container = std::uint8_t(id);
            d.sealed = sealed;
            if (!admit(d))
                continue;
            const auto access = c.cell + forward(c.face) * -1;
            if (!free(access)) {
                occupied[index(c.cell)] = false;
                continue;
            }
            protect(access);
            f.caches[id] = {c.cell, c.face};
            world.decorations.push_back(d);
            return true;
        }
        return false;
    }
};
static_assert(sizeof(Dressing) + maxCells * (sizeof(Candidate) + sizeof(bool) + sizeof(Cell)) +
                      2 * sizeof(FloorLayout) <
                  128 * 1024,
              "Bounded dressing workspace");
} // namespace
bool dressUnderkeep(FloorLayout& f, GenerationRequest request, WorldDefinition& world) {
    if (validate(f))
        return false;
    world.assetPack = "underkeep";
    world.decorations.clear();
    world.decorations.reserve(192);
    Dressing d{f, world, {}, {}, seedFor(request, Domain::Dressing)};
    // Preserve doors, passage lanes, stairs, anchors, actors and existing rewards.
    for (unsigned i = 0; i < f.pathCount; ++i)
        d.protect({int(f.paths[i] % f.width), int(f.paths[i] / f.width)});
    for (unsigned i = 0; i < f.roomCount; ++i)
        d.protect(f.rooms[i].anchor);
    for (unsigned i = 0; i < f.fixtures.count; ++i) {
        const auto c = f.fixtures.entries[i].cell;
        d.protect(c);
        for (unsigned h = 0; h < 4; ++h)
            d.protect(c + forward(Heading(h)));
    }
    for (unsigned i = 0; i < maxCells; ++i)
        if (f.fixtures.npcExcluded[i])
            d.protectedCells[i] = true;
    for (unsigned i = 0; i < f.populationCount; ++i)
        d.protect(f.population[i].cell);
    for (auto c : {f.spawn.cell, f.supply, f.optionalSupply, f.guardianReward, f.guardianApproach})
        d.protect(c);
    f.cacheCount = f.depth == 3 ? 2 : 3;
    if (!d.chest(0, f.supply, false) || !d.chest(1, f.optionalSupply, false) ||
        (f.cacheCount == 3 && !d.chest(2, f.guardianReward, true)))
        return false;
    // One contextual furniture arrangement; racks remain restricted to deepest floor.
    bool furnished = false;
    for (unsigned trial = 0; trial < 32 && !furnished; ++trial) {
        const auto& room = f.rooms[d.next(f.roomCount)];
        if (room.role == Role::Entrance || room.role == Role::Objective)
            continue;
        const Cell c{room.origin.x + int(d.next(unsigned(room.size.x))),
                     room.origin.y + int(d.next(unsigned(room.size.y)))};
        const auto h = Heading(d.next(4));
        if (floorCell(f, c + forward(h) * -1))
            continue;
        const auto kind = f.depth == 3 ? DecorationKind::Rack : DecorationKind::Cage;
        Decoration prop{c,
                        kind == DecorationKind::Cage ? Heading((unsigned(h) + 2) % 4) : h,
                        kind,
                        0,
                        .5f,
                        0,
                        .68f,
                        kind == DecorationKind::Rack ? .36f : .84f,
                        kind == DecorationKind::Rack ? 1.7f : .72f};
        if (d.admit(prop)) {
            world.decorations.push_back(prop);
            furnished = true;
        }
    }
    unsigned relief = 0, details = 0, webs = 0, bones = 0;
    for (unsigned i = 0; i < f.fixtures.count; ++i) {
        const auto& fixture = f.fixtures.entries[i];
        if (fixture.kind == FixtureKind::Door || fixture.kind == FixtureKind::GuardianGate) {
            world.decorations.push_back(
                {fixture.cell, fixture.facing, DecorationKind::DoorSurround, 0, .5f, 0, 1.f, 1.f, .08f});
            ++relief;
        }
    }
    if (furnished) {
        Cell furnitureCell{};
        Heading furnitureFace = Heading::North;
        bool found = false;
        for (const auto& prop : world.decorations)
            if (prop.kind == DecorationKind::Cage || prop.kind == DecorationKind::Rack) {
                furnitureCell = prop.cell;
                furnitureFace = prop.face;
                found = true;
                break;
            }
        if (found)
            for (unsigned h = 0; h < 4; ++h) {
                const auto cell = furnitureCell + forward(Heading((unsigned(furnitureFace) + h + 2) % 4));
                if (!d.free(cell) || f.fixtures.npcExcluded[d.index(cell)])
                    continue;
                world.decorations.push_back(
                    {cell, Heading::North, DecorationKind::BonesA, 0, .5f, .003f, .55f, .55f, .02f});
                ++bones;
                break;
            }
    }
    for (int y = 1; y < f.height - 1; ++y)
        for (int x = 1; x < f.width - 1; ++x) {
            const Cell c{x, y};
            if (!d.free(c) || f.fixtures.npcExcluded[d.index(c)])
                continue;
            bool fixture = false;
            for (unsigned i = 0; i < f.fixtures.count; ++i) {
                const auto p = f.fixtures.entries[i].cell;
                fixture |= std::abs(p.x - x) + std::abs(p.y - y) <= 1;
            }
            if (fixture)
                continue;
            if (regionAt(f, c) != noRegion && bones < 5 && d.next(45) == 0) {
                bool bare = true;
                for (const auto& other : world.decorations)
                    if (other.cell == c && other.floorCutout())
                        bare = false;
                if (bare) {
                    world.decorations.push_back({c, Heading::North,
                                                 bones % 2 ? DecorationKind::BonesB : DecorationKind::BonesA,
                                                 0, .5f, .003f, .65f, .65f, .02f});
                    ++bones;
                }
            }
            for (unsigned h = 0; h < 4; ++h) {
                const auto face = Heading(h), side = Heading((h + 1) % 4);
                if (floorCell(f, c + forward(face)))
                    continue;
                // Rhythmic relief follows room perimeter, outside passage/fixture areas.
                if (regionAt(f, c) != noRegion && relief < 64 && ((h % 2 ? y : x) % 3 == 0)) {
                    world.decorations.push_back(
                        {c, face, DecorationKind::Pilaster, 0, .5f, 0, .19f, 1.f, .10f});
                    ++relief;
                } else if (regionAt(f, c) != noRegion && relief < 64 && d.next(4) == 0) {
                    world.decorations.push_back(
                        {c, face, DecorationKind::WallBase, 0, .5f, 0, 1.f, .09f, .065f});
                    ++relief;
                }
                if (details < 28 && !((h % 2 ? y : x) % 3 == 0) && d.next(4) == 0) {
                    const unsigned choice = d.next(12);
                    auto kind = choice < 4     ? DecorationKind::DampA
                                : choice < 7   ? DecorationKind::DampB
                                : choice < 9   ? DecorationKind::Cracks
                                : choice == 9  ? DecorationKind::Soot
                                : choice == 10 ? DecorationKind::BannerA
                                               : DecorationKind::BannerB;
                    const auto region = regionAt(f, c);
                    if ((kind == DecorationKind::BannerA || kind == DecorationKind::BannerB) &&
                        (region == noRegion || (region != 0 && f.rooms[region].role != Role::Approach &&
                                                f.rooms[region].role != Role::Optional)))
                        kind = DecorationKind::Cracks;
                    const bool banner = kind == DecorationKind::BannerA || kind == DecorationKind::BannerB;
                    world.decorations.push_back({c, face, kind, 0, .5f, banner ? .57f : .25f,
                                                 banner ? .44f : .75f, banner ? .73f : .45f, .01f});
                    ++details;
                } else if (webs < 5 && !floorCell(f, c + forward(side)) && d.next(3) == 0) {
                    world.decorations.push_back({c, face,
                                                 webs % 2 ? DecorationKind::WebB : DecorationKind::WebA, 0,
                                                 .5f, .82f, .70f, .34f, .01f});
                    ++webs;
                }
            }
        }
    // One very subdued old stain, only near the rare imprisonment arrangement.
    if (furnished && f.depth == 3)
        for (const auto& prop : world.decorations)
            if (prop.kind == DecorationKind::Rack || prop.kind == DecorationKind::Cage) {
                const auto c = prop.cell + right(prop.face);
                bool bare = true;
                for (const auto& other : world.decorations)
                    if (other.cell == c && other.floorCutout())
                        bare = false;
                if (d.free(c) && bare)
                    world.decorations.push_back(
                        {c, Heading::North, DecorationKind::Blood, 0, .5f, .004f, .35f, .35f, .01f});
                break;
            }
    return d.connected();
}
} // namespace blobber::generation

#include "game/perception.hpp"
#include "generation/caves.hpp"
#include <algorithm>

namespace blobber::generation {
namespace {
using Site = CaveSite;
using BoundsMask = std::bitset<maxCells>;
unsigned index(const Map& m, Cell c) {
    return unsigned(c.y * m.width() + c.x);
}
// Doors are interactable throughout this standalone floor. The caller's cut mask and
// stair footprints model impassable cells. Every edge uses the published Map.
BoundsMask reachable(const Map& m, Cell start, BoundsMask blocked) {
    BoundsMask seen;
    if (m.movementBlocked(start) || blocked[index(m, start)])
        return seen;
    std::array<std::uint16_t, maxCells> queue;
    unsigned head = 0, tail = 0;
    auto add = [&](unsigned i) {
        seen.set(i);
        queue[tail++] = std::uint16_t(i);
    };
    add(index(m, start));
    while (head < tail) {
        const int n = queue[head++];
        const Cell c{n % m.width(), n / m.width()};
        for (unsigned h = 0; h < 4; ++h) {
            const Cell next = c + forward(Heading(h));
            if (!m.canStep(c, next))
                continue;
            const unsigned i = index(m, next);
            if (!seen[i] && !blocked[i])
                add(i);
        }
    }
    return seen;
}
} // namespace
Failure validateCave(const CaveFloor& cave) {
    const auto& f = cave.layout;
    const auto& m = cave.map;
    const auto& sites = cave.sites;
    if (f.width != m.width() || f.height != m.height() || f.depth < 4 || f.depth > 6 || f.roomCount != 16 ||
        f.connectionCount < f.roomCount + 1 || f.biome != BiomeId::DampCaves ||
        f.algorithm != AlgorithmId::CaveChambers)
        return {Error::Bounds};
    if (auto failure = validateGeometry(f))
        return failure;
    const auto& defs = f.fixtures;
    if (defs.depth != f.depth || !defs.discovery || defs.count != 4 || defs.landingCount != 2 ||
        !(defs.landings[0] == f.spawn.cell) || !(defs.landings[1] == sites[Site::Goal]))
        return {Error::Landing};
    BoundsMask fixed;
    for (unsigned i = 0; i < defs.count; ++i) {
        const auto& d = defs.entries[i];
        if (unsigned(d.facing) > 3 || m.movementBlocked(d.cell))
            return {Error::Fixture};
        if (i == 0 || i == 3) {
            const auto kind = i == 0         ? FixtureKind::Upstairs
                              : f.depth == 6 ? FixtureKind::Endpoint
                                             : FixtureKind::Downstairs;
            if (d.kind != kind || !(stairLanding(d) == defs.landings[i == 0 ? 0 : 1]))
                return {Error::Landing};
            fixed.set(index(m, d.cell));
        } else {
            const auto& link = f.connections[i == 1 ? 0 : 3];
            if (d.kind != FixtureKind::Door || d.key || !(d.cell == link.socketA) || d.facing != link.facingA)
                return {Error::Fixture};
            for (auto delta : {-1, 1})
                if (!m.canStep(d.cell, d.cell + forward(d.facing) * delta))
                    return {Error::Fixture};
            if (!m.solid(d.cell + right(d.facing)) || !m.solid(d.cell + right(d.facing) * -1))
                return {Error::Fixture};
        }
    }
    for (auto c : defs.landings) {
        if (m.movementBlocked(c))
            return {Error::Landing};
        for (int y = -1; y <= 1; ++y)
            for (int x = -1; x <= 1; ++x) {
                const Cell near = c + Cell{x, y};
                if (!m.solid(near) && (m.movementBlocked(near) || !defs.npcExcluded[index(m, near)]))
                    return {Error::Landing};
            }
    }
    if (regionAt(f, f.guardianApproach) != 3 || m.movementBlocked(f.guardianApproach) || f.approachDoor != 2)
        return {Error::Progression};
    if (lineOfSight(m, f.spawn.cell, f.rooms[1].anchor))
        return {Error::Landing};
    const auto all = reachable(m, f.spawn.cell, fixed);
    for (int y = 0; y < m.height(); ++y)
        for (int x = 0; x < m.width(); ++x) {
            Cell c{x, y};
            const unsigned i = index(m, c);
            if (m.solid(c) == floorCell(f, c))
                return {Error::Bounds};
            if (!m.movementBlocked(c) && !fixed[i] && !all[i])
                return {Error::Disconnected};
        }
    auto accessible = [&](Site s) {
        const auto c = sites[s];
        return inside(f, c) && all[index(m, c)];
    };
    if (!accessible(Site::Goal) || !accessible(Site::Optional) || regionAt(f, sites[Site::Optional]) != 5)
        return {Error::Branch};
    auto cut = fixed;
    for (unsigned i = 0; i < f.connectionCount; ++i)
        if (f.connections[i].a == 5 || f.connections[i].b == 5)
            cut.set(index(m, f.connections[i].socketA));
    const auto direct = reachable(m, f.spawn.cell, cut);
    if (!direct[index(m, sites[Site::Goal])] || direct[index(m, sites[Site::Optional])])
        return {Error::Branch};
    if (!accessible(Site::Lookout) || regionAt(f, sites[Site::Lookout]) != 15 ||
        !accessible(Site::HighGallery) || !accessible(Site::LowGallery))
        return {Error::Branch};
    cut.set(index(m, f.connections[6].socketA));
    const auto bypass = reachable(m, f.spawn.cell, cut);
    if (!bypass[index(m, sites[Site::Goal])] || bypass[index(m, sites[Site::Lookout])])
        return {Error::Branch};
    // Every network passage can close in isolation while the objective remains
    // accessible. This rejects loops confined to a side branch off a linear spine.
    for (unsigned edge = 7; edge < f.connectionCount; ++edge) {
        if (f.connections[edge].a < 6 || f.connections[edge].b < 6)
            continue;
        cut = fixed;
        cut.set(index(m, f.connections[edge].socketA));
        if (!reachable(m, f.spawn.cell, cut)[index(m, sites[Site::Goal])])
            return {Error::Loop};
    }
    for (unsigned edge = 0; edge < 4; ++edge) {
        if ((edge == 2 && f.depth != 5) || (edge == 1 && f.depth == 6))
            continue;
        cut = fixed;
        cut.set(index(m, f.connections[edge].socketA));
        if (reachable(m, f.spawn.cell, cut)[index(m, sites[Site::Goal])])
            return {Error::Progression};
    }
    if (f.depth == 4) {
        if (!accessible(Site::Camp) || !accessible(Site::SpiderArch) || !accessible(Site::ArchWeb) ||
            !accessible(Site::PocketWeb) || regionAt(f, sites[Site::Camp]) != 1 ||
            regionAt(f, sites[Site::SpiderArch]) != 2)
            return {Error::Anchor};
        cut = fixed;
        cut.set(index(m, f.connections[1].socketA));
        const auto camp = reachable(m, f.spawn.cell, cut);
        if (!camp[index(m, sites[Site::Camp])] || camp[index(m, sites[Site::SpiderArch])] ||
            camp[index(m, sites[Site::Optional])])
            return {Error::Progression};
    }
    if (f.depth == 5) {
        for (auto s :
             {Site::SoloNewt, Site::Fork, Site::FarBank, Site::NorthContact, Site::SouthContact, Site::Rope})
            if (!accessible(s))
                return {Error::Anchor};
        const Cell center = sites[Site::Fork] + Cell{3, 0};
        if (!(sites[Site::FarBank] == center + Cell{3, 0}) || regionAt(f, sites[Site::SoloNewt]) != 1 ||
            !(sites[Site::NorthContact] == center + Cell{1, -2}) ||
            !(sites[Site::SouthContact] == center + Cell{1, 2}))
            return {Error::Anchor};
        for (int y = -1; y <= 1; ++y)
            for (int x = -2; x <= 2; ++x) {
                const Cell c = center + Cell{x, y};
                const bool pillar = y == 0 && std::abs(x) <= 1;
                if (m.solid(c) != pillar || !m.movementBlocked(c) || !m.npcBlocked(c))
                    return {Error::Room};
            }
        if (!lineOfSight(m, center + Cell{-2, -2}, center + Cell{-2, 2}) ||
            lineOfSight(m, sites[Site::Fork], sites[Site::FarBank]))
            return {Error::Room};
        for (int side : {-1, 1}) {
            cut = fixed;
            cut.set(index(m, center + Cell{0, 2 * side}));
            const auto bank = reachable(m, sites[Site::Fork], cut);
            if (!bank[index(m, sites[Site::FarBank])])
                return {Error::Progression};
            cut.set(index(m, center + Cell{0, -2 * side}));
            if (reachable(m, sites[Site::Fork], cut)[index(m, sites[Site::FarBank])])
                return {Error::Progression};
        }
    }
    if (f.depth == 6) {
        if (!accessible(Site::Mother) || regionAt(f, sites[Site::Mother]) != 4 ||
            !(sites[Site::WestClutch] == sites[Site::Mother] + Cell{-2, 0}) ||
            !(sites[Site::EastClutch] == sites[Site::Mother] + Cell{2, 0}))
            return {Error::Anchor};
        for (unsigned i = 0; i < 2; ++i) {
            const auto c = sites[i ? Site::EastClutch : Site::WestClutch];
            const auto melee = i ? Site::EastMelee : Site::WestMelee,
                       hatch = i ? Site::EastHatch : Site::WestHatch;
            const int dropSide = cave.hangingClutches ? -1 : 1;
            if (!inside(f, c) || m.solid(c) || m.movementBlocked(c) != !cave.hangingClutches ||
                !accessible(melee) || !accessible(hatch) || !(sites[melee] == c + Cell{0, -dropSide}) ||
                !(sites[hatch] == c + Cell{0, dropSide}) || defs.npcExcluded[index(m, sites[hatch])])
                return {Error::Anchor};
        }
    }
    return {};
}
} // namespace blobber::generation

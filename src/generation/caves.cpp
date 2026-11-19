#include "generation/caves.hpp"
#include "generation/cave_pool.hpp"
#include <algorithm>
#include <stdexcept>

namespace blobber::generation {
using Site = CaveSite;
const char* siteName(Site s) {
    static constexpr std::array<const char*, caveSiteCount> names{
        "Survey camp", "Teaching spider", "Arch web",       "Pocket web",        "Solo newt",
        "Pool fork",   "Far bank",        "North contact",  "South contact",     "Rope",
        "Broodmother", "West clutch",     "East clutch",    "West melee",        "East melee",
        "West hatch",  "East hatch",      "Optional cache", "Objective landing", "Pillar gallery",
        "Low gallery", "Survey lookout"};
    return names.at(unsigned(s));
}
namespace {
struct Builder {
    FloorLayout layout;
    CaveSites sites;
    WorldDefinition world;
    std::uint64_t random;
    bool fallback;
    unsigned pick(unsigned count) {
        random = random * 6364136223846793005ull + 1442695040888963407ull;
        return fallback ? 0 : unsigned(random >> 32) % count;
    }
    void tile(Cell c, unsigned owner) {
        if (!inside(layout, c))
            throw Failure{Error::Bounds};
        auto i = unsigned(c.y * layout.width + c.x);
        if (layout.floor[i])
            throw Failure{Error::Placement};
        layout.floor[i] = 1;
        layout.membership[i] = std::uint8_t(owner);
    }
    unsigned chamber(Cell center, Cell radius, Role role, int ceiling, Cell anchorOffset = {}) {
        auto id = layout.roomCount++;
        auto& room = layout.rooms.at(id);
        room.id = std::uint8_t(id);
        room.role = role;
        room.origin = center + radius * -1;
        room.size = radius * 2 + Cell{1, 1};
        room.anchor = center + anchorOffset;
        room.clearance = role == Role::Junction || role == Role::Optional ? 0 : 1;
        std::array<int, 4> cuts{};
        for (auto& cut : cuts)
            cut = 1 + int(pick(2));
        // Keep three socket candidates on narrow faces of staggered galleries.
        if (id >= 6 && std::min(radius.x, radius.y) == 2)
            cuts.fill(1);
        // Both outer pool banks need the shoulder cell at each end.
        if (layout.depth == 5 && radius.x == 4 && radius.y == 2)
            cuts.fill(1);
        for (int y = -radius.y; y <= radius.y; ++y)
            for (int x = -radius.x; x <= radius.x; ++x) {
                const auto corner = unsigned(x > 0) + 2 * unsigned(y > 0);
                if (radius.x - std::abs(x) + radius.y - std::abs(y) < cuts[corner])
                    continue;
                Cell c = center + Cell{x, y};
                tile(c, id);
                world.surfaces[unsigned(c.y * layout.width + c.x)].ceiling = std::int16_t(ceiling);
            }
        return id;
    }
    // Inclusive exterior sockets. The two straight segments meet exactly once at bend.
    void link(unsigned a, unsigned b, Cell start, Heading outA, Cell bend, Cell end, Heading outB) {
        auto& e = layout.connections.at(layout.connectionCount++);
        e = {std::uint8_t(a), std::uint8_t(b), start, end, outA, outB, std::uint16_t(layout.pathCount), 0};
        Cell c = start;
        for (;;) {
            tile(c, noRegion);
            layout.paths.at(layout.pathCount++) = std::uint16_t(c.y * layout.width + c.x);
            ++e.count;
            if (c == end)
                break;
            if (c == bend)
                outA = Heading((unsigned(outB) + 2) % 4);
            c = c + forward(outA);
        }
    }
    void prop(const char* id, Cell cell, bool blocking) {
        world.scenery.push_back({id, cell, {}, Heading::North, 1, 1, blocking});
    }
    void web(unsigned room, Site site) {
        const auto& r = layout.rooms[room];
        // Find a supported corner on the incoming half, starting at the north wall.
        for (int y = r.origin.y; y < r.origin.y + r.size.y; ++y)
            for (int x = r.origin.x; x < r.origin.x + r.size.x; ++x) {
                Cell c{x, y};
                if (regionAt(layout, c) != room)
                    continue;
                for (unsigned h = 0; h < 4; ++h) {
                    auto face = Heading(h);
                    if (floorCell(layout, c + forward(face)) || floorCell(layout, c + right(face)))
                        continue;
                    world.decorations.push_back(
                        {c, face, DecorationKind::WebA, 0, .5f, .84f, .95f, .55f, .01f});
                    sites[site] = c;
                    return;
                }
            }
        throw Failure{Error::Anchor, int(room)};
    }
    void rock(Cell c) {
        auto n = unsigned(c.y * layout.width + c.x);
        layout.floor[n] = 0;
        layout.membership[n] = noRegion;
    }
    // Pick a common open boundary row/column. Chambers may be staggered or
    // asymmetrically clipped, so use actual membership on both faces.
    void connect(unsigned a, unsigned b, Heading heading) {
        const auto step = forward(heading);
        const auto& ra = layout.rooms[a];
        const auto& rb = layout.rooms[b];
        std::array<Cell, 9> starts{}, ends{};
        unsigned choices = 0;
        const bool horizontal = step.x != 0;
        const int delta = horizontal ? step.x : step.y;
        const int extentA = horizontal ? ra.size.x : ra.size.y;
        const int extentB = horizontal ? rb.size.x : rb.size.y;
        const int crossExtent = horizontal ? ra.size.y : ra.size.x;
        for (int n = 0; n < crossExtent; ++n) {
            Cell ca = ra.origin, cb = rb.origin;
            if (horizontal) {
                ca.y += n;
                cb.y = ca.y;
                if (delta > 0)
                    ca.x += extentA - 1;
                else
                    cb.x += extentB - 1;
            } else {
                ca.x += n;
                cb.x = ca.x;
                if (delta > 0)
                    ca.y += extentA - 1;
                else
                    cb.y += extentB - 1;
            }
            if (regionAt(layout, ca) != a || regionAt(layout, cb) != b)
                continue;
            starts.at(choices) = ca + step;
            ends.at(choices) = cb + step * -1;
            ++choices;
        }
        if (!choices)
            throw Failure{Error::Port, int(a)};
        unsigned selected = pick(choices);
        link(a, b, starts[selected], heading, ends[selected], ends[selected],
             Heading((unsigned(heading) + 2) % 4));
    }
    void carveCircuits(unsigned base, unsigned outlet, unsigned depth) {
        std::array<std::array<unsigned, 2>, 12> edges{};
        unsigned count = 0;
        for (unsigned n = 0; n < 9; ++n) {
            if (n % 3 < 2)
                edges[count++] = {n, n + 1};
            if (n < 6)
                edges[count++] = {n, n + 3};
        }
        for (unsigned n = count; n > 1; --n)
            std::swap(edges[n - 1], edges[pick(n)]);
        // Remove passages only if all galleries remain reachable and the exit
        // still has a second approach. Check the tiny graph before carving.
        std::array<bool, 12> open;
        open.fill(true);
        auto reached = [&](unsigned closed) {
            std::bitset<9> seen;
            seen.set(3); // west mouth
            for (unsigned pass = 0; pass < 9; ++pass) {
                const auto before = seen;
                for (unsigned i = 0; i < count; ++i)
                    if (open[i] && i != closed) {
                        auto [a, b] = edges[i];
                        if (seen[a] || seen[b]) {
                            seen.set(a);
                            seen.set(b);
                        }
                    }
                if (seen == before)
                    break;
            }
            return seen;
        };
        unsigned remove = depth == 5 ? 1 : 2;
        for (unsigned i = 0; i < count && remove; ++i) {
            open[i] = false;
            bool good = reached(count).all();
            for (unsigned j = 0; good && j < count; ++j)
                if (open[j])
                    good = reached(j)[outlet];
            if (good)
                --remove;
            else
                open[i] = true;
        }
        for (unsigned i = 0; i < count; ++i)
            if (open[i]) {
                auto [a, b] = edges[i];
                connect(base + a, base + b, b == a + 1 ? Heading::East : Heading::South);
            }
    }
    void galleries(unsigned depth, int row, int east, int approachRow, int lookoutRow) {
        // Stagger a belt of high and low chambers around the three-way junction.
        constexpr unsigned base = 6;
        for (unsigned y = 0; y < 3; ++y)
            for (unsigned x = 0; x < 3; ++x) {
                unsigned n = y * 3 + x;
                const int cy = y == 0 ? 5 : y == 1 ? row : 24;
                const int cx = x == 2 ? east - 8 : int(x) * 8 + 23 + int(pick(unsigned(layout.width - 51)));
                Cell radius{2 + int(pick(2)), 2 + int(pick(2))};
                if (x == 0 && cx == 24)
                    radius.x = 2;
                if (n == 1 || n == 7)
                    radius = {3, n == 1 ? 4 : 2};
                if (x == 2)
                    radius.x = 2;
                const int jitter = int(pick(3)) - 1;
                const int stagger = n == 1 || n == 3 ? 0 : jitter;
                const Cell center{
                    cx, std::clamp(cy + stagger, y == 2 ? 24 : radius.y + 1, layout.height - radius.y - 2)};
                chamber(center, radius, Role::Junction, n == 1 ? 9 : n == 7 ? 5 : 6 + int(pick(3)));
                if (n == 1) {
                    sites[Site::HighGallery] = center;
                    rock(center + Cell{-1, -1});
                    rock(center + Cell{1, 1});
                }
                if (n == 7)
                    sites[Site::LowGallery] = center;
            }
        const Cell lookout{east, lookoutRow};
        chamber(lookout, {2, 3}, Role::Optional, 7);
        sites[Site::Lookout] = lookout;
        prop("brazier", lookout, false);
        // The west/east mouths of the teaching rooms remain fixed for the pool.
        const auto middle = layout.rooms[2].origin + Cell{4, 2};
        const auto& inlet = layout.rooms[base + 3];
        const Cell inletEnd{inlet.origin.x - 1, row};
        link(2, base + 3, middle + Cell{5, 0}, Heading::East, inletEnd, inletEnd, Heading::West);
        const auto& goal = layout.rooms[4];
        const Cell door{east, approachRow + 3}, socket{east, goal.origin.y - 1};
        link(3, 4, door, Heading::South, socket, socket, Heading::North);
        connect(base, 5, Heading::West);
        const unsigned outlet = base + (approachRow == 5 ? 2 : 5);
        connect(outlet, 3, Heading::East);
        connect(base + (lookoutRow == 5 ? 2 : 8), 15, Heading::East);

        carveCircuits(base, outlet - base, depth);
        if (depth == 4)
            connect(2, 5, Heading::North);
        if (depth == 6) {
            const Cell first = layout.rooms[1].anchor;
            const auto& cache = layout.rooms[5];
            const Cell end{cache.origin.x - 1, cache.anchor.y};
            link(1, 5, first + Cell{0, -4}, Heading::North, {first.x, end.y}, end, Heading::West);
        }
    }
    void build(unsigned depth) {
        auto& f = layout;
        f.depth = depth;
        f.biome = BiomeId::DampCaves;
        f.algorithm = AlgorithmId::CaveChambers;
        f.profile = depth == 4   ? LayoutProfile::Seep
                    : depth == 5 ? LayoutProfile::DrownedGalleries
                                 : LayoutProfile::BroodVault;
        world.assetPack = "cave-blockout";
        f.width = 52 + int(pick(2));
        f.height = 30;
        world.surfaces.resize(unsigned(f.width * f.height));
        const int row = 14 + int(pick(2)), east = f.width - 6;
        const int approachRow = depth == 4 ? 5 : depth == 5 ? row : (pick(2) ? row : 5);
        const int goalRow = approachRow == 5 ? row : 24;
        const int lookoutRow = approachRow == 5 ? 24 : 5;
        const Cell first{5, row}, middle{14, row}, entry{12, 24}, approach{east, approachRow},
            goal{east, goalRow};
        const Cell pocket{11 + int(pick(3)), 5};
        const int objectiveRadius = depth == 6 ? 4 : 3;
        chamber(entry, {3, 2}, Role::Entrance, 4);
        chamber(first, {2, 3}, Role::Junction, 5);
        chamber(middle, {4, 2}, Role::Junction, depth == 5 ? 8 : 6, {-3, 0});
        chamber(approach, {2, 2}, Role::Approach, 6);
        chamber(goal, {objectiveRadius, objectiveRadius}, Role::Objective, depth == 6 ? 8 : 6, {0, 1});
        chamber(pocket, {3, 2}, depth == 5 ? Role::Optional : Role::Junction, 5);
        link(0, 1, entry + Cell{-4, 0}, Heading::West, {first.x, entry.y}, first + Cell{0, 4},
             Heading::South);
        link(1, 2, first + Cell{3, 0}, Heading::East, middle + Cell{-5, 0}, middle + Cell{-5, 0},
             Heading::West);
        galleries(depth, row, east, approachRow, lookoutRow);
        f.spawn = {entry, Heading::West};
        f.rooms[0].facing = f.spawn.heading;
        f.guardianApproach = approach;
        f.optionalSupply = pocket;
        f.guardianGate = UINT8_MAX; // The review door opens normally, without a guardian objective.
        sites[Site::Optional] = pocket;
        sites[Site::Goal] = goal + Cell{0, objectiveRadius - 1};
        auto& fixtures = f.fixtures;
        fixtures.depth = depth;
        fixtures.discovery = true;
        auto fixture = [&](Cell c, Heading h, FixtureKind kind) {
            fixtures.entries.at(fixtures.count++) = {c, h, kind, 0};
        };
        fixture(entry + Cell{1, 0}, Heading::East, FixtureKind::Upstairs);
        fixture(f.connections[0].socketA, Heading::West, FixtureKind::Door);
        f.approachDoor = std::uint8_t(fixtures.count);
        fixture(f.connections[3].socketA, Heading::South, FixtureKind::Door);
        fixture(sites[Site::Goal] + Cell{0, 1}, Heading::South,
                depth == 6 ? FixtureKind::Endpoint : FixtureKind::Downstairs);
        fixtures.landings = {entry, sites[Site::Goal]};
        fixtures.landingCount = 2;
        for (auto c : fixtures.landings)
            for (int y = -1; y <= 1; ++y)
                for (int x = -1; x <= 1; ++x)
                    fixtures.npcExcluded.set(unsigned((c.y + y) * f.width + c.x + x));
        prop("crate-small", pocket, false);
        if (depth == 4) {
            sites[Site::Camp] = first + Cell{0, -1};
            prop("crate-large", sites[Site::Camp], false);
            prop("sleeping-bay", first + Cell{-1, 1}, true);
            sites[Site::SpiderArch] = middle;
            prop("footlocker", middle, false);
            web(2, Site::ArchWeb);
            web(5, Site::PocketWeb);
        } else if (depth == 5) {
            sites[Site::SoloNewt] = first;
            prop("barrel", first, false);
            sites[Site::Fork] = middle + Cell{-3, 0};
            sites[Site::FarBank] = middle + Cell{3, 0};
            sites[Site::NorthContact] = middle + Cell{1, -2};
            sites[Site::SouthContact] = middle + Cell{1, 2};
            prop("footlocker", sites[Site::NorthContact], false);
            prop("barrel", sites[Site::SouthContact], false);
            sites[Site::Rope] = middle + Cell{-1, -2};
            prop("survey-rope", sites[Site::Rope], false);
            for (int x = -1; x <= 1; ++x)
                rock(middle + Cell{x, 0});
            for (const auto& patch : cavePool) {
                Cell anchor = middle + patch.center;
                Cell offset{-int(patch.width / 2), 0};
                world.scenery.push_back({patch.model, anchor, offset, Heading::North, patch.width, 1, true});
                for (unsigned x = 0; x < patch.width; ++x) {
                    const Cell c = anchor + offset + Cell{int(x), 0};
                    world.surfaces[unsigned(c.y * f.width + c.x)].floor = -1;
                }
            }
        } else {
            sites[Site::Mother] = goal;
            prop("footlocker", goal, false);
            for (unsigned i = 0; i < 2; ++i) {
                const Cell c = goal + Cell{i ? 2 : -2, 0};
                sites[i ? Site::EastClutch : Site::WestClutch] = c;
                sites[i ? Site::EastMelee : Site::WestMelee] = c + Cell{0, -1};
                sites[i ? Site::EastHatch : Site::WestHatch] = c + Cell{0, 1};
                prop("clutch-blockout", c, true);
            }
        }
    }
};
} // namespace
CaveResult generateCave(std::uint32_t seed, unsigned depth, bool forceFallback) {
    CaveResult result;
    auto& d = result.diagnostics;
    if (depth < 4 || depth > 6) {
        d.failure = {Error::Request};
        return result;
    }
    for (unsigned attempt = 0; attempt <= maxAttempts; ++attempt) {
        d.fallback = attempt == maxAttempts;
        if (!d.fallback)
            ++d.attempts;
        if (forceFallback && !d.fallback) {
            d.rejected[attempt] = {Error::ForcedFallback};
            continue;
        }
        Failure failure;
        try {
            Builder b{{}, {}, {}, seedFor({seed, depth}, Domain::Topology, 0, attempt), d.fallback};
            b.build(depth);
            std::vector<std::string> rows(b.layout.height, std::string(b.layout.width, '#'));
            for (int y = 0; y < b.layout.height; ++y)
                for (int x = 0; x < b.layout.width; ++x)
                    if (floorCell(b.layout, {x, y}))
                        rows[y][x] = '.';
            CaveFloor floor{std::move(b.layout), b.sites, Map(rows, std::move(b.world))};
            failure = validateCave(floor);
            if (!failure) {
                if (d.fallback)
                    d.fallbackId = depth;
                result.floor.emplace(std::move(floor));
                return result;
            }
        } catch (const std::out_of_range&) {
            failure = {Error::Capacity};
        } catch (const std::invalid_argument&) {
            failure = {Error::Placement}; // The final WorldDefinition failed Map admission.
        } catch (Failure badMotif) {
            failure = badMotif;
        }
        if (d.fallback)
            d.failure = failure;
        else
            d.rejected[attempt] = failure;
    }
    return result;
}
} // namespace blobber::generation

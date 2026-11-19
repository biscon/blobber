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
    void prop(const char* id, Cell cell, bool blocking, Heading facing = Heading::North) {
        world.scenery.push_back({id, cell, {}, facing, 1, 1, blocking});
    }
    void overhead(const char* model, Cell cell, float clearance) {
        world.scenery.push_back({model, cell, {}, Heading::North, 1, 1, false, clearance});
    }
    void trail(Cell cell, Heading facing, bool bend = false) {
        world.scenery.push_back({bend ? "trail-bend" : "trail-straight", cell, {}, facing, 1, 1, false});
    }
    // Scan actual room members for corners supported on both sides. The first
    // match is the DC01 reservation. Extra vault silk uses other available corners.
    void web(unsigned room, Site site = Site::Count, unsigned limit = 1) {
        const auto& r = layout.rooms[room];
        unsigned placed = 0;
        for (int y = 0; y < r.size.y; ++y)
            for (int x = 0; x < r.size.x; ++x) {
                Cell c = r.origin + Cell{x, y};
                if (regionAt(layout, c) != room)
                    continue;
                for (unsigned h = 0; h < 4; ++h) {
                    auto face = Heading(h);
                    if (!naturalWall(c + forward(face)) || !naturalWall(c + right(face)))
                        continue;
                    const auto variant = site == Site::ArchWeb     ? 0u
                                         : site == Site::PocketWeb ? 1u
                                                                   : (room + layout.depth + placed) % 3;
                    const auto kind = variant == 0   ? DecorationKind::WebA
                                      : variant == 1 ? DecorationKind::WebB
                                                     : DecorationKind::Vine;
                    float elevation = room == 4 && placed % 2 ? 1.35f : .75f;
                    world.decorations.push_back({c, face, kind, 0, .5f, elevation, variant == 2 ? .67f : .95f,
                                                 variant == 1 ? .55f : .92f, .01f});
                    if (!placed && site != Site::Count)
                        sites[site] = c;
                    if (++placed == limit)
                        return;
                    break;
                }
            }
        if (!placed && site != Site::Count)
            throw Failure{Error::Anchor, int(room)};
    }
    bool groundSceneryAt(Cell cell) const {
        for (const auto& s : world.scenery) {
            if (s.clearance > 0)
                continue;
            for (unsigned z = 0; z < s.depth; ++z)
                for (unsigned x = 0; x < s.width; ++x)
                    if (s.cell(x, z) == cell)
                        return true;
        }
        return false;
    }

    bool naturalWall(Cell cell) const {
        if (floorCell(layout, cell))
            return false;
        for (const auto& b : world.buildings)
            for (unsigned x = 0; x < b.width; ++x)
                if (b.cell(x, 0) == cell)
                    return false;
        return true;
    }

    void wallStore(const char* model, unsigned room) {
        const auto& r = layout.rooms[room];
        for (int y = 0; y < r.size.y; ++y)
            for (int x = 0; x < r.size.x; ++x) {
                const Cell c = r.origin + Cell{x, y};
                if (regionAt(layout, c) != room || groundSceneryAt(c))
                    continue;
                for (auto h : {Heading::North, Heading::East, Heading::South, Heading::West})
                    if (naturalWall(c + forward(h))) {
                        prop(model, c, false, h);
                        return;
                    }
            }
        throw Failure{Error::Placement, int(room)};
    }

    bool reservedCell(Cell c) const {
        return layout.fixtures.npcExcluded[c.y * layout.width + c.x] ||
               std::find(sites.cells.begin(), sites.cells.end(), c) != sites.cells.end();
    }

    // A pocket hugs a rock face and leaves a parallel dry shoulder. Keep its
    // entire neighbourhood clear of passage sockets, even at clipped corners.
    void shorePool(unsigned room, bool pair) {
        const auto& r = layout.rooms[room];
        auto eligible = [&](Cell c, Heading wall) {
            if (regionAt(layout, c) != room || c == r.anchor || reservedCell(c) || groundSceneryAt(c) ||
                !naturalWall(c + forward(wall)) || regionAt(layout, c + forward(wall) * -1) != room)
                return false;
            for (int y = -1; y <= 1; ++y)
                for (int x = -1; x <= 1; ++x) {
                    Cell neighbour = c + Cell{x, y};
                    if (floorCell(layout, neighbour) && regionAt(layout, neighbour) != room)
                        return false;
                }
            return true;
        };
        const int area = r.size.x * r.size.y;
        Cell first{-1, -1}, second{-1, -1};
        for (int k = 0; k < area && second.x < 0; ++k) {
            const int n = (k + room * 11 + layout.depth * 7) % area;
            Cell c = r.origin + Cell{n % r.size.x, n / r.size.x};
            for (unsigned h = 0; h < 4; ++h) {
                const auto wall = Heading(h);
                if (!eligible(c, wall))
                    continue;
                if (first.x < 0)
                    first = c;
                if (!pair)
                    break;
                Cell next = c + right(wall);
                if (eligible(next, wall)) {
                    first = c;
                    second = next;
                    break;
                }
            }
            if (!pair && first.x >= 0)
                break;
        }
        if (first.x < 0)
            throw Failure{Error::Placement, int(room)};
        for (auto c : {first, second})
            if (c.x >= 0) {
                prop("pool-east", c, true);
                world.surfaces[c.y * layout.width + c.x].floor = -1;
            }
    }

    void dressRoof(unsigned room) {
        constexpr std::array<const char*, 5> clusters{"camp-teeth", "arch-teeth", "lookout-teeth",
                                                      "vault-teeth", "gallery-teeth"};
        const auto& r = layout.rooms[room];
        const int area = r.size.x * r.size.y;
        const unsigned target = std::clamp(2 + area / 10, 5, 7);
        std::array<Cell, 7> groups{};
        unsigned count = 0;
        for (const auto& prop : world.scenery)
            if (prop.clearance && regionAt(layout, prop.anchor) == room)
                groups.at(count++) = prop.anchor;
        auto distance = [](Cell a, Cell b) {
            const int dx = a.x - b.x, dy = a.y - b.y;
            return dx * dx + dy * dy;
        };
        // Pick near the centre first, then the candidate farthest from existing
        // roots. A room-specific scan order breaks ties without consuming RNG.
        while (count < target) {
            Cell best{};
            int bestScore = -10000;
            for (int k = 0; k < area; ++k) {
                const int n = (k + 11 * room + r.origin.x) % area;
                const Cell c = r.origin + Cell{n % r.size.x, n / r.size.x};
                if (regionAt(layout, c) != room || reservedCell(c))
                    continue;
                const auto surface = world.surfaces[c.y * layout.width + c.x];
                if (surface.floor || surface.ceiling < 5)
                    continue;
                int score = count ? 10000 : -distance(c, r.origin + Cell{r.size.x / 2, r.size.y / 2});
                for (unsigned i = 0; i < count; ++i)
                    score = std::min(score, distance(c, groups[i]));
                if (count && score == 0)
                    continue;
                // Favor clear spans for the first few groups so the canopy is
                // visible from the door. Later roots can run into the wall seams.
                if (count < 3) {
                    for (unsigned h = 0; h < 4; ++h)
                        if (regionAt(layout, c + forward(Heading(h))) != room)
                            score -= area;
                }
                if (score > bestScore) {
                    bestScore = score;
                    best = c;
                }
            }
            if (bestScore == -10000)
                throw Failure{Error::Placement, int(room)};
            const auto ceiling = world.surfaces[best.y * layout.width + best.x].ceiling;
            world.scenery.push_back({clusters.at(unsigned(ceiling - 5)),
                                     best,
                                     {},
                                     Heading((best.x + best.y + count) % 4),
                                     1,
                                     1,
                                     false,
                                     std::max(1.02f, elevation(ceiling) - .72f)});
            groups[count++] = best;
        }
    }

    void dressChambers() {
        for (unsigned room : {5u, 6u, 8u, 9u, 11u, 14u})
            shorePool(room, (room + layout.depth) % 3 != 0);
        for (unsigned room : {1u, 6u, 8u, 9u, 11u, 13u, 15u})
            web(room, Site::Count, 1 + (room + layout.depth) % 2);
        for (unsigned room = 0; room < layout.roomCount; ++room) {
            const auto& r = layout.rooms[room];
            const int count = r.size.x * r.size.y;
            const unsigned ledgeLimit = room >= 6 && room < 15 ? 1 : 2;
            dressRoof(room);
            unsigned ledges = 0;
            Heading lastWall = Heading::North;
            for (int k = 0; k < count; ++k) {
                const int n = (k + 11 * int(room) + r.origin.x) % count;
                const Cell c = r.origin + Cell{n % r.size.x, n / r.size.x};
                if (regionAt(layout, c) != room || layout.fixtures.npcExcluded[c.y * layout.width + c.x])
                    continue;
                const auto surface = world.surfaces[c.y * layout.width + c.x];
                if (surface.floor != 0)
                    continue;
                const unsigned pattern = unsigned(c.x * 71 + c.y * 37 + room * 13);
                const bool reserved =
                    std::find(sites.cells.begin(), sites.cells.end(), c) != sites.cells.end();
                for (unsigned h = 0; h < 4; ++h) {
                    const auto facing = Heading((pattern + h) % 4);
                    if (!naturalWall(c + forward(facing)))
                        continue;
                    if (std::any_of(world.decorations.begin(), world.decorations.end(),
                                    [c](const auto& d) { return d.cell == c; }))
                        break;
                    if (!reserved && !groundSceneryAt(c) && ledges < ledgeLimit &&
                        (!ledges || facing != lastWall)) {
                        prop((room + ledges) % 2 ? "fungus-ledge" : "flowstone", c, false, facing);
                        lastWall = facing;
                        ++ledges;
                    }
                    if ((pattern / 4) % 3 != 0) {
                        const bool hanging = (pattern / 7) % 4 == 0;
                        const float width = .68f + .07f * (pattern % 5);
                        const float height =
                            hanging ? .40f + .08f * ((pattern / 5) % 5) : .24f + .055f * ((pattern / 5) % 5);
                        const float center =
                            hanging ? elevation(surface.ceiling) - height / 2 - .09f * (pattern % 3)
                                    : height / 2;
                        const auto kind = hanging ? DecorationKind::Crest : DecorationKind::DampA;
                        world.decorations.push_back({c, facing, kind, 0, .5f, center, width, height, .01f});
                    }
                    break;
                }
            }
        }
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
        const Cell hearth = lookout + Cell{0, 3};
        prop("brazier", hearth, false);
        world.ambientFires.push_back({hearth.x + .5f, .36f, hearth.y + .5f, 1.f, true});
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
        world.assetPack = "damp-caves";
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
        chamber(entry, {3, 2}, Role::Entrance, 5);
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
        for (int y = -1; y <= 1; ++y)
            for (int x = -1; x <= 1; ++x)
                world.surfaces[(entry.y + y) * f.width + entry.x + x].ceiling = 4;
        const auto high = sites[Site::HighGallery];
        for (Cell offset : {Cell{-1, -1}, Cell{1, 1}})
            world.buildings.push_back({"high-column", high + offset, {}, Heading::North, 1, 1, true});
        overhead("gallery-teeth", high + Cell{0, 1}, 1.53f);
        wallStore("crate-small", 5);
        if (depth == 4) {
            sites[Site::Camp] = first + Cell{0, -1};
            wallStore("crate-large", 1);
            prop("bedroll", first + Cell{-2, 0}, true);
            sites[Site::SpiderArch] = middle;
            overhead("arch-teeth", middle + Cell{-2, 0}, 1.02f);

            web(2, Site::ArchWeb);
            web(5, Site::PocketWeb);
        } else if (depth == 5) {
            sites[Site::SoloNewt] = first;
            sites[Site::Fork] = middle + Cell{-3, 0};
            sites[Site::FarBank] = middle + Cell{3, 0};
            sites[Site::NorthContact] = middle + Cell{1, -2};
            sites[Site::SouthContact] = middle + Cell{1, 2};
            sites[Site::Rope] = middle + Cell{-1, -2};
            trail(sites[Site::Fork], Heading::East, true);
            trail(middle + Cell{-3, -1}, Heading::East);
            trail(middle + Cell{-3, -2}, Heading::West, true);
            for (int x = -2; x <= 2; ++x)
                trail(middle + Cell{x, -2}, Heading::North);
            trail(middle + Cell{3, -2}, Heading::North, true);
            trail(middle + Cell{3, -1}, Heading::East);
            trail(sites[Site::FarBank], Heading::South, true);
            world.buildings.push_back({"limestone-rib", middle, {-1, 0}, Heading::North, 3, 1, true});
            overhead("vault-teeth", middle + Cell{-2, -2}, 1.28f);
            for (int x = -1; x <= 1; ++x) {
                const auto cell = middle + Cell{x, 0};
                rock(cell);
                world.surfaces[unsigned(cell.y * f.width + cell.x)].floor = -1;
            }
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
            web(3);
            web(4, Site::Count, 5);
            overhead("vault-teeth", goal + Cell{1, -2}, 1.28f);
            trail(goal + Cell{0, -4}, Heading::East);
            trail(goal + Cell{0, -3}, Heading::East, true);
            trail(goal + Cell{-1, -3}, Heading::North);
            trail(goal + Cell{-2, -3}, Heading::West, true);
            for (int y : {-2, -1})
                trail(goal + Cell{-2, y}, Heading::East);
            for (unsigned i = 0; i < 2; ++i) {
                const Cell c = goal + Cell{i ? 2 : -2, 0};
                sites[i ? Site::EastClutch : Site::WestClutch] = c;
                sites[i ? Site::EastMelee : Site::WestMelee] = c + Cell{0, -1};
                sites[i ? Site::EastHatch : Site::WestHatch] = c + Cell{0, 1};
                prop(i ? "sealed-clutch" : "tethered-clutch", c, true);
            }
        }
        dressChambers();
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

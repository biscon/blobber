#include "generation/floor.hpp"
#include <algorithm>
namespace blobber::generation {
namespace {
std::uint64_t mix(std::uint64_t z) noexcept {
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}
struct Random {
    std::uint64_t state;
    unsigned next(unsigned bound) noexcept {
        state += 0x9e3779b97f4a7c15ull;
        const auto word = std::uint32_t(mix(state) >> 32);
        return unsigned((std::uint64_t(word) * bound) >> 32);
    }
};
Cell origin(Cell site) noexcept {
    return {1 + site.x * 9, 1 + site.y * 9};
}
void edge(FloorLayout& f, unsigned a, unsigned b) {
    f.connections[f.connectionCount++] = {std::uint8_t(a), std::uint8_t(b)};
}
void room(FloorLayout& f, Cell site, Role role) {
    auto& r = f.rooms[f.roomCount];
    r.site = site;
    r.role = role;
    r.id = std::uint8_t(f.roomCount++);
}
FloorLayout candidate(GenerationRequest request, unsigned attempt, const TemplateCatalog& catalog,
                      bool fallback) {
    FloorLayout f;
    f.depth = request.depth;
    Random rng{seedFor(request, Domain::Topology, 0, attempt)};
    f.family = fallback ? 0 : rng.next(2);
    f.rotation = fallback ? request.depth - 1 : rng.next(4);
    const Cell six[]{{0, 1}, {1, 1}, {2, 1}, {2, 2}, {1, 2}, {0, 2}};
    const Cell eight[]{{0, 0}, {1, 0}, {2, 0}, {2, 1}, {2, 2}, {1, 2}, {0, 2}, {0, 1}};
    const unsigned count = f.family ? 8 : 6;
    for (unsigned i = 0; i < count; ++i)
        room(f, f.family ? eight[i] : six[i], Role::Junction);
    for (unsigned i = 0; i < count; ++i)
        edge(f, i, (i + 1) % count);
    if (!f.family) {
        const unsigned entrance = fallback ? 0 : rng.next(2);
        room(f, {int(entrance), 0}, Role::Entrance);
        edge(f, entrance, 6);
        room(f, {3, 1}, Role::Approach);
        edge(f, 2, 7);
        room(f, {3, fallback ? 0 : int(rng.next(2) * 2)}, Role::Objective);
        edge(f, 7, 8);
        const unsigned branch = fallback ? 0 : rng.next(2);
        room(f, {int(branch), 3}, Role::Optional);
        edge(f, branch ? 4 : 5, 9);
        if (!fallback && rng.next(2)) {
            room(f, {2, 3}, Role::Optional);
            edge(f, 3, 10);
        }
    } else {
        room(f, {0, 3}, Role::Entrance);
        edge(f, 6, 8);
        room(f, {3, 1}, Role::Approach);
        edge(f, 3, 9);
        room(f, {3, int(rng.next(2) * 2)}, Role::Objective);
        edge(f, 9, 10);
        room(f, {2, 3}, Role::Optional);
        edge(f, 4, 11);
    }
    for (unsigned i = 0; i < f.roomCount; ++i)
        f.rooms[i].site = rotate(f.rooms[i].site, 4, f.rotation);
    for (unsigned i = 0; i < f.roomCount; ++i) {
        auto& r = f.rooms[i];
        Random interior{seedFor(request, Domain::Interior, r.id, attempt)};
        r.shape = fallback || (r.role != Role::Junction && r.role != Role::Optional)
                      ? 0
                      : std::uint8_t(interior.next(4));
        r.rotation = fallback ? std::uint8_t(f.rotation) : std::uint8_t(interior.next(4));
        if (r.role == Role::Entrance || r.role == Role::Objective) {
            r.shape = 4;
            for (unsigned e = 0; e < f.connectionCount; ++e) {
                const auto edge = f.connections[e];
                const auto other = edge.a == i ? edge.b : edge.b == i ? edge.a : maxRooms;
                if (other == maxRooms)
                    continue;
                const auto site = f.rooms[other].site;
                for (unsigned d = 0; d < 4; ++d)
                    if (r.site + forward(Heading(d)) == site)
                        r.rotation = std::uint8_t((d + 2) % 4);
            }
        }
        const auto base = origin(r.site);
        r.anchor = base + rotate(catalog[r.shape].anchor, 7, r.rotation);
        r.facing = static_cast<Heading>(f.rotation);
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 7; ++x) {
                const auto p = base + rotate({x, y}, 7, r.rotation);
                f.floor[p.y * f.width + p.x] = catalog[r.shape].floor[y * 7 + x];
            }
    }
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto e = f.connections[i];
        auto& a = f.rooms[e.a];
        auto& b = f.rooms[e.b];
        for (unsigned d = 0; d < 4; ++d) {
            const auto direction = forward(static_cast<Heading>(d));
            if (!(a.site + direction == b.site))
                continue;
            a.ports |= std::uint8_t(1u << d);
            b.ports |= std::uint8_t(1u << ((d + 2) % 4));
            const auto center = origin(a.site) + Cell{3, 3};
            for (int step = 3; step <= 6; ++step) {
                const auto p = center + direction * step;
                f.floor[p.y * f.width + p.x] = 1;
            }
            // Entrance faces inward along its only connection.
            if (a.role == Role::Entrance)
                a.facing = static_cast<Heading>(d);
            if (b.role == Role::Entrance)
                b.facing = static_cast<Heading>((d + 2) % 4);
        }
    }
    for (unsigned i = 0; i < f.roomCount; ++i)
        if (f.rooms[i].role == Role::Entrance)
            f.spawn = {f.rooms[i].anchor, f.rooms[i].facing};
    placeFixtures(f);
    placePopulation(f, request, attempt, fallback);
    return f;
}
} // namespace
std::uint64_t seedFor(GenerationRequest r, Domain domain, unsigned id, unsigned attempt) noexcept {
    auto h = mix(0x424c4f4242455255ull ^ r.seed);
    for (const auto value : {r.depth, r.generator, r.templates, unsigned(domain), id, attempt})
        h = mix(h ^ (std::uint64_t(value) + 0x9e3779b97f4a7c15ull));
    return h;
}
std::uint64_t fingerprint(const FloorLayout& f) noexcept {
    std::uint64_t h = 14695981039346656037ull;
    const auto number = [&](std::uint32_t n) {
        for (unsigned i = 0; i < 4; ++i) {
            h = (h ^ (n & 255)) * 1099511628211ull;
            n >>= 8;
        }
    };
    number(generatorVersion);
    number(templateVersion);
    number(unsigned(f.width));
    number(unsigned(f.height));
    for (int i = 0; i < f.width * f.height; ++i)
        number(f.floor[std::size_t(i)]);
    number(f.depth);
    number(f.fixtures.count);
    number(f.fixtures.discovery);
    number(f.fixtures.depth);
    for (unsigned i = 0; i < f.fixtures.count; ++i) {
        const auto& v = f.fixtures.entries[i];
        number(unsigned(v.cell.x));
        number(unsigned(v.cell.y));
        number(unsigned(v.kind));
        number(unsigned(v.facing));
        number(v.key);
    }
    number(f.fixtures.landingCount);
    for (unsigned i = 0; i < f.fixtures.landingCount; ++i) {
        number(unsigned(f.fixtures.landings[i].x));
        number(unsigned(f.fixtures.landings[i].y));
    }
    for (unsigned i = 0; i < maxCells; ++i)
        number(f.fixtures.npcExcluded[i]);
    number(f.populationCount);
    for (unsigned i = 0; i < f.populationCount; ++i) {
        const auto& p = f.population[i];
        number(p.id);
        number(unsigned(p.recipe));
        number(p.guardian);
        number(unsigned(p.cell.x));
        number(unsigned(p.cell.y));
        number(unsigned(p.subcell.x));
        number(unsigned(p.subcell.y));
        number(unsigned(p.facing));
    }
    for (auto c : {f.optionalSupply, f.guardianReward, f.guardianApproach}) {
        number(unsigned(c.x));
        number(unsigned(c.y));
    }
    number(f.approachDoor);
    number(f.guardianGate);
    number(f.family);
    number(f.rotation);
    number(f.roomCount);
    number(f.connectionCount);
    number(f.loopConnection);
    for (unsigned i = 0; i < f.roomCount; ++i) {
        const auto& r = f.rooms[i];
        number(r.id);
        number(unsigned(r.site.x));
        number(unsigned(r.site.y));
        number(unsigned(r.anchor.x));
        number(unsigned(r.anchor.y));
        number(unsigned(r.role));
        number(r.shape);
        number(r.rotation);
        number(r.ports);
        number(unsigned(r.facing));
    }
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        number(f.connections[i].a);
        number(f.connections[i].b);
    }
    number(unsigned(f.spawn.cell.x));
    number(unsigned(f.spawn.cell.y));
    number(unsigned(f.spawn.heading));
    return h;
}
GenerationResult generate(GenerationRequest request, const TemplateCatalog& catalog, bool forceFallback) {
    GenerationResult out;
    if (request.depth < 1 || request.depth > 3 || request.generator != generatorVersion ||
        request.templates != templateVersion) {
        out.diagnostics.failure = {Error::Request};
        return out;
    }
    if (const auto failure = validateTemplates(catalog)) {
        out.diagnostics.failure = failure;
        return out;
    }
    FloorLayout layout;
    bool valid = false;
    for (unsigned i = 0; i < maxAttempts; ++i) {
        ++out.diagnostics.attempts;
        if (forceFallback) {
            out.diagnostics.rejected[i] = {Error::ForcedFallback};
            continue;
        }
        layout = candidate(request, i, catalog, false);
        const auto failure = validate(layout);
        if (!failure) {
            valid = true;
            break;
        }
        out.diagnostics.rejected[i] = failure;
    }
    if (!valid) {
        out.diagnostics.fallback = true;
        out.diagnostics.fallbackId = request.depth;
        layout = candidate(request, 0, catalog, true);
        if (const auto failure = validate(layout)) {
            out.diagnostics.failure = failure;
            return out;
        }
    }
    std::vector<std::string> rows(std::size_t(layout.height), std::string(std::size_t(layout.width), '#'));
    for (int y = 0; y < layout.height; ++y)
        for (int x = 0; x < layout.width; ++x)
            if (layout.floor[y * layout.width + x])
                rows[y][x] = '.';
    out.floor.emplace(GeneratedFloor{request, layout, Map(rows), out.diagnostics, fingerprint(layout),
                                     std::uint32_t(seedFor(request, Domain::Runtime))});
    return out;
}
std::string ascii(const FloorLayout& f) {
    std::string s;
    for (int y = 0; y < f.height; ++y) {
        for (int x = 0; x < f.width; ++x) {
            char c = f.floor[y * f.width + x] ? '.' : '#';
            for (unsigned i = 0; i < f.roomCount; ++i)
                if (f.rooms[i].anchor == Cell{x, y}) {
                    constexpr char markers[]{'+', 'E', 'A', 'O', 'B'};
                    c = markers[unsigned(f.rooms[i].role)];
                }
            s.push_back(c);
        }
        s.push_back('\n');
    }
    return s;
}
static_assert(sizeof(GeneratedFloor) + maxCells < 16 * 1024, "Retained generation budget");
} // namespace blobber::generation

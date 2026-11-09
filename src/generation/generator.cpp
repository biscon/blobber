#include "generation/architecture.hpp"
#include "generation/dressing.hpp"
#include "generation/floor.hpp"
#include "generation/stone_layout.hpp"
#include <algorithm>
namespace blobber::generation {
namespace {
std::uint64_t mix(std::uint64_t z) noexcept {
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}
FloorLayout candidate(GenerationRequest request, unsigned attempt, bool fallback) {
    FloorLayout f;
    const auto spec = floorSpec(request.depth);
    if (!spec)
        return f;
    f.depth = spec->depth;
    f.width = spec->width;
    f.height = spec->height;
    f.biome = spec->biome;
    f.algorithm = underkeepBiome().algorithm;
    switch (f.algorithm) {
    case AlgorithmId::StoneRooms:
        if (!buildStoneLayout(f, request, attempt, fallback))
            return f;
        break;
    }
    placeFixtures(f);
    placePopulation(f, request, attempt, fallback);
    return f;
}
} // namespace
std::uint64_t seedFor(GenerationRequest r, Domain domain, unsigned id, unsigned attempt) noexcept {
    auto h = mix(0x424c4f4242455255ull ^ r.seed);
    // Preserve accepted placement streams independently of generation identity.
    const unsigned randomGenerator = domain == Domain::Dressing ? 12 : 8;
    const unsigned randomCatalog = domain == Domain::Dressing ? 9 : 6;
    for (const auto value : {r.depth, randomGenerator, randomCatalog, unsigned(domain), id, attempt})
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
    number(std::uint32_t(f.dressingFingerprint));
    number(std::uint32_t(f.dressingFingerprint >> 32));
    number(f.cacheCount);
    for (unsigned i = 0; i < f.cacheCount; ++i) {
        number(f.caches[i].cell.x);
        number(f.caches[i].cell.y);
        number(unsigned(f.caches[i].facing));
    }
    number(unsigned(f.biome));
    number(unsigned(f.algorithm));
    number(unsigned(f.profile));
    number(f.pathCount);
    for (int i = 0; i < f.width * f.height; ++i)
        number(f.membership[unsigned(i)]);
    for (unsigned i = 0; i < f.pathCount; ++i)
        number(f.paths[i]);
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
    number(f.encounterCount);
    for (unsigned i = 0; i < f.encounterCount; ++i) {
        const auto& e = f.encounters[i];
        number(unsigned(e.recipe));
        number(unsigned(e.role));
        number(e.room);
        number(e.first);
        number(e.count);
    }
    number(f.routeCount);
    for (unsigned i = 0; i < f.routeCount; ++i)
        number(f.route[i]);
    for (auto c : {f.optionalSupply, f.guardianReward, f.guardianApproach, f.supply}) {
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
        number(unsigned(r.origin.x));
        number(unsigned(r.origin.y));
        number(unsigned(r.size.x));
        number(unsigned(r.size.y));
        number(unsigned(r.anchor.x));
        number(unsigned(r.anchor.y));
        number(unsigned(r.role));
        number(r.shape);
        number(r.rotation);
        number(r.clearance);
        number(unsigned(r.facing));
    }
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        number(f.connections[i].a);
        number(f.connections[i].b);
        const auto& e = f.connections[i];
        for (auto c : {e.socketA, e.socketB}) {
            number(unsigned(c.x));
            number(unsigned(c.y));
        }
        number(unsigned(e.facingA));
        number(unsigned(e.facingB));
        number(e.first);
        number(e.count);
    }
    number(unsigned(f.spawn.cell.x));
    number(unsigned(f.spawn.cell.y));
    number(unsigned(f.spawn.heading));
    return h;
}
GenerationResult generate(GenerationRequest request, bool forceFallback) {
    GenerationResult out;
    if (request.depth < 1 || request.depth > 3 || request.generator != generatorVersion ||
        request.templates != templateVersion) {
        out.diagnostics.failure = {Error::Request};
        return out;
    }
    FloorLayout layout;
    bool valid = false;
    WorldDefinition dressing;
    for (unsigned i = 0; i < maxAttempts; ++i) {
        ++out.diagnostics.attempts;
        if (forceFallback) {
            out.diagnostics.rejected[i] = {Error::ForcedFallback};
            continue;
        }
        layout = candidate(request, i, false);
        const auto failure = validate(layout);
        if (!failure) {
            if (!dressUnderkeep(layout, request, dressing))
                continue;
            valid = true;
            break;
        }
        out.diagnostics.rejected[i] = failure;
    }
    if (!valid) {
        out.diagnostics.fallback = true;
        out.diagnostics.fallbackId = request.depth;
        layout = candidate(request, 0, true);
        if (const auto failure = validate(layout)) {
            out.diagnostics.failure = failure;
            return out;
        }
    }
    if (!valid && !dressUnderkeep(layout, request, dressing)) {
        out.diagnostics.failure = {Error::Fixture};
        return out;
    }
    std::vector<std::string> rows(std::size_t(layout.height), std::string(std::size_t(layout.width), '#'));
    for (int y = 0; y < layout.height; ++y)
        for (int x = 0; x < layout.width; ++x)
            if (layout.floor[y * layout.width + x])
                rows[y][x] = '.';
    prepareArchitecture(layout, dressing);
    Map map(rows, std::move(dressing));
    layout.dressingFingerprint = map.definition().fingerprint();
    out.floor.emplace(GeneratedFloor{request, layout, std::move(map), out.diagnostics, fingerprint(layout),
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

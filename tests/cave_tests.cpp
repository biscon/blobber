#include "game/perception.hpp"
#include "generation/caves.hpp"
#include "support/check.hpp"
#include <iostream>
using namespace blobber;
using namespace blobber::generation;
void checkCaveWater(const CaveFloor&);
namespace {
CaveFloor sample(unsigned seed, unsigned depth, bool forced = false) {
    auto r = generateCave(seed, depth, forced);
    if (!r.floor)
        throw std::runtime_error(std::string("Cave rejected: ") + errorName(r.diagnostics.failure.code));
    if (r.diagnostics.fallback != forced) {
        std::cerr << "depth " << depth << " seed " << seed << " rejected";
        for (auto f : r.diagnostics.rejected)
            std::cerr << " " << errorName(f.code) << "/" << f.room << "/" << f.connection;
        std::cerr << "\n";
    }
    check(r.diagnostics.fallback == forced, "Ordinary seed unexpectedly exhausted its proposals");
    if (forced)
        check(r.diagnostics.attempts == maxAttempts && r.diagnostics.fallbackId == depth,
              "Fallback diagnostics");
    return std::move(*r.floor);
}
std::bitset<maxRooms * maxRooms> passages(const FloorLayout& layout) {
    std::bitset<maxRooms * maxRooms> result;
    for (unsigned i = 0; i < layout.connectionCount; ++i) {
        const auto& e = layout.connections[i];
        result.set(std::min(e.a, e.b) * maxRooms + std::max(e.a, e.b));
    }
    return result;
}
void run() {
    check(!generateCave(5, 3).floor && !generateCave(5, 7).floor, "Invalid depth admitted");
    check(!floorSpec(4) && floorSpec(3)->finalFloor, "Cave preview leaked into campaign");
    for (unsigned d = 4; d <= 6; ++d) {
        auto a = sample(5, d), b = sample(5, d), other = sample(6, d);
        check(fingerprint(a.layout) == fingerprint(b.layout) && a.sites.cells == b.sites.cells &&
                  a.map.definition().fingerprint() == b.map.definition().fingerprint(),
              "Unstable floor");
        check(a.layout.floor != other.layout.floor, "Seed must change the actual footprint");
        auto frozen = sample(0, d, true), otherFrozen = sample(UINT32_MAX, d, true);
        check(fingerprint(frozen.layout) == fingerprint(otherFrozen.layout) &&
                  frozen.sites.cells == otherFrozen.sites.cells &&
                  frozen.map.definition().fingerprint() == otherFrozen.map.definition().fingerprint(),
              "Fallback depends on seed");
        check(passages(a.layout) != passages(other.layout), "Seeds only moved rooms without changing routes");
        for (const auto* f : {&a, &frozen}) {
            check(f->layout.roomCount - 2 >= 12 && f->layout.connectionCount >= f->layout.roomCount + 1,
                  "Exploration collapsed to a short spine");
            check(!(f->sites[CaveSite::Lookout] == f->sites[CaveSite::Optional]),
                  "Both detours share a destination");
        }
        auto oversized = a.layout;
        oversized.height = int(maxCells);
        check(validateGeometry(oversized).code == Error::Capacity, "Oversized rectangular map admitted");
        FloorGeometry geometry(a.map, a.layout.fixtures);
        auto door = a.layout.fixtures.entries[1];
        auto from = door.cell + forward(door.facing) * -1;
        check(!geometry.view().canStep(from, door.cell) &&
                  !lineOfSight(geometry.view(), from, door.cell + forward(door.facing)),
              "Closed entrance admits traversal or sight");
        check(!geometry.view().npcAllowed(a.layout.spawn.cell), "Unprotected arrival");
        auto bad = a;
        bad.layout.fixtures.npcExcluded.reset();
        check(validateCave(bad).code == Error::Landing, "Lost landing protection");
        bad = a;
        bad.layout.connections[0].socketA = bad.layout.spawn.cell;
        check(validateCave(bad).code == Error::Port, "Malformed socket accepted");
        bad = a;
        bad.sites[CaveSite::Optional] = bad.sites[CaveSite::Goal];
        check(validateCave(bad).code == Error::Branch, "Optional pocket can become mandatory");
        if (d == 4) {
            bad = a;
            std::swap(bad.sites[CaveSite::Camp], bad.sites[CaveSite::SpiderArch]);
            check(bool(validateCave(bad)), "Camp can follow first spider");
        }
        if (d == 5) {
            checkCaveWater(a);
            const Cell center = a.sites[CaveSite::Fork] + Cell{3, 0};
            const auto bank = center + Cell{0, -2}, pool = bank + Cell{0, 1};
            Player player{bank, Heading::South};
            check(!tryAction(geometry.view(), player, Action::Forward) && !geometry.view().npcAllowed(pool),
                  "Water admits a walker");
            check(lineOfSight(a.map, center + Cell{-2, -2}, center + Cell{-2, 2}), "Pool became opaque");
            check(!lineOfSight(a.map, center + Cell{-2, 0}, center + Cell{-1, -1}),
                  "Diagonal sight escaped rib corner");
            // Obstruct just one bank in the final published world. The other still connects the floor.
            for (int side : {-1, 1}) {
                auto world = a.map.definition();
                world.scenery.push_back(
                    {"test-rock", center + Cell{0, side * 2}, {}, Heading::North, 1, 1, true});
                std::vector<std::string> rows(a.map.height(), std::string(a.map.width(), '#'));
                for (int y = 0; y < a.map.height(); ++y)
                    for (int x = 0; x < a.map.width(); ++x)
                        if (!a.map.solid({x, y}))
                            rows[y][x] = '.';
                bad = a;
                bad.map = Map(rows, std::move(world));
                check(validateCave(bad).code == Error::Progression,
                      "One usable bank passed the two-route contract");
            }
        }
        if (d == 6) {
            bad = a;
            bad.sites[CaveSite::EastHatch] = bad.sites[CaveSite::WestHatch];
            check(bool(validateCave(bad)), "Shared hatch space accepted");
        }
    }
}
} // namespace
int main() try {
    run();
    std::cout << "Cave geometry, routes, LOS and fallback checks passed\n";
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}

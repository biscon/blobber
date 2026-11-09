#include "generation/architecture.hpp"
#include "generation/blueprint_rooms.hpp"
#include "generation/diagnostic_view.hpp"
#include "generation/dressing.hpp"
#include "generation/floor.hpp"
#include "support/check.hpp"
#include "support/generation_blueprints.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
using namespace blobber;
using namespace blobber::generation;
namespace {
FloorLayout rotated(const FloorLayout& source, unsigned turn) {
    auto f = source;
    f.floor.fill(0);
    f.membership.fill(noRegion);
    f.fixtures.npcExcluded.reset();
    auto heading = [&](Heading h) { return Heading((unsigned(h) + turn) % 4); };
    for (int y = 0; y < f.height; ++y)
        for (int x = 0; x < f.width; ++x) {
            const auto p = rotate({x, y}, f.width, turn);
            const auto a = unsigned(y * f.width + x), b = unsigned(p.y * f.width + p.x);
            f.floor[b] = source.floor[a];
            f.membership[b] = source.membership[a];
            if (source.fixtures.npcExcluded[a])
                f.fixtures.npcExcluded.set(b);
        }
    f.rotation = (f.rotation + turn) % 4;
    for (unsigned i = 0; i < f.blueprintCount; ++i)
        f.blueprints[i].rotation = std::uint8_t((f.blueprints[i].rotation + turn) % 4);
    for (unsigned i = 0; i < f.roomCount; ++i) {
        auto& r = f.rooms[i];
        const auto p = rotate(r.origin, f.width, turn);
        const auto q = rotate(r.origin + r.size + Cell{-1, -1}, f.width, turn);
        r.origin = {std::min(p.x, q.x), std::min(p.y, q.y)};
        if (turn % 2)
            std::swap(r.size.x, r.size.y);
        r.anchor = rotate(r.anchor, f.width, turn);
        r.rotation = std::uint8_t((r.rotation + turn) % 4);
        r.facing = heading(r.facing);
    }
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        auto& e = f.connections[i];
        e.socketA = rotate(e.socketA, f.width, turn);
        e.socketB = rotate(e.socketB, f.width, turn);
        e.facingA = heading(e.facingA);
        e.facingB = heading(e.facingB);
    }
    for (unsigned i = 0; i < f.pathCount; ++i) {
        const auto p =
            rotate({int(source.paths[i]) % f.width, int(source.paths[i]) / f.width}, f.width, turn);
        f.paths[i] = std::uint16_t(p.y * f.width + p.x);
    }
    for (unsigned i = 0; i < f.fixtures.count; ++i) {
        auto& v = f.fixtures.entries[i];
        v.cell = rotate(v.cell, f.width, turn);
        v.facing = heading(v.facing);
    }
    for (unsigned i = 0; i < f.fixtures.landingCount; ++i)
        f.fixtures.landings[i] = rotate(f.fixtures.landings[i], f.width, turn);
    for (unsigned i = 0; i < f.populationCount; ++i) {
        auto& p = f.population[i];
        p.cell = rotate(p.cell, f.width, turn);
        p.subcell = rotate(p.subcell, 2, turn);
        p.facing = heading(p.facing);
    }
    for (auto* c : {&f.guardianReward, &f.guardianApproach, &f.optionalSupply, &f.supply, &f.spawn.cell})
        *c = rotate(*c, f.width, turn);
    for (unsigned i = 0; i < f.cacheCount; ++i) {
        f.caches[i].cell = rotate(f.caches[i].cell, f.width, turn);
        f.caches[i].facing = heading(f.caches[i].facing);
    }
    f.spawn.heading = heading(f.spawn.heading);
    return f;
}
std::string signature(const FloorLayout& f) {
    std::string best;
    for (unsigned mirror = 0; mirror < 2; ++mirror)
        for (unsigned turn = 0; turn < 4; ++turn) {
            std::string s(unsigned(f.width * f.height), '#');
            for (int y = 0; y < f.height; ++y)
                for (int x = 0; x < f.width; ++x) {
                    const auto p = rotate({mirror ? f.width - 1 - x : x, y}, f.width, turn);
                    s[unsigned(p.y * f.width + p.x)] = f.floor[unsigned(y * f.width + x)] ? '.' : '#';
                }
            if (best.empty() || s < best)
                best = s;
        }
    return best;
}
void architectureRules() {
    auto fixture = [](unsigned area, Cell size, unsigned shape = 0) {
        FloorLayout f;
        f.roomCount = 1;
        f.rooms[0].size = size;
        f.rooms[0].shape = std::uint8_t(shape);
        for (unsigned n = 0; n < area; ++n) {
            const unsigned i = (1 + n / unsigned(size.x)) * unsigned(f.width) + 1 + n % unsigned(size.x);
            f.floor[i] = 1;
            f.membership[i] = 0;
        }
        return f;
    };
    check(roomCeilings(fixture(19, {5, 4}))[0] == 5, "19-cell room is small");
    check(roomCeilings(fixture(20, {5, 4}))[0] == 6, "20-cell room is medium");
    check(roomCeilings(fixture(29, {5, 6}))[0] == 6, "Missing corner prevents large tier");
    check(roomCeilings(fixture(30, {5, 6}))[0] == 7, "30-cell broad room is large");
    check(roomCeilings(fixture(32, {4, 8}))[0] == 6, "Narrow hall cannot become a great hall");
    check(roomCeilings(fixture(30, {5, 7}, 2))[0] == 6, "L room remains medium");
    check(roomCeilings(fixture(24, {5, 5}, 1))[0] == 6, "Small pillar room has no landmark bonus");
    auto f = fixture(34, {5, 7}, 1);
    check(roomCeilings(f)[0] == 8, "Large pillared room gets tallest ceiling");
    for (unsigned turn = 0; turn < 4; ++turn)
        check(roomCeilings(rotated(f, turn))[0] == 8, "Ceiling classification survives rotation");
    for (auto role : {Role::Entrance, Role::Objective}) {
        f.rooms[0].role = role;
        check(roomCeilings(f)[0] == 4, "Stair regions retain original ceiling");
    }
    f.rooms[0].role = Role::Junction;
    f.fixtures.count = 2;
    f.fixtures.entries[0] = {{1, 1}, Heading::North, FixtureKind::Door, 0};
    f.fixtures.entries[1] = {{2, 1}, Heading::North, FixtureKind::GuardianGate, 0};
    WorldDefinition world;
    world.decorations = {{{3, 1}, Heading::North, DecorationKind::Pilaster},
                         {{3, 1}, Heading::North, DecorationKind::WebA, 0, .5f, .82f, .7f, .34f},
                         {{1, 1}, Heading::North, DecorationKind::DoorSurround, 0, .5f, 0, 1, 1},
                         {{3, 1}, Heading::North, DecorationKind::BannerA}};
    const auto banner = world.decorations.back();
    prepareArchitecture(f, world);
    const auto at = [&](Cell c) { return world.surfaces[unsigned(c.y * f.width + c.x)]; };
    check(at({1, 1}).ceiling == 4 && at({2, 1}).ceiling == 4 && at({3, 1}).ceiling == 8,
          "Low door/gate thresholds override room height locally");
    check(at({0, 0}).ceiling == 4 && at({3, 1}).floor == 0, "Default surfaces stay low and level");
    check(surfacePassage(at({2, 1}), at({3, 1})), "Room transition retains movement clearance");
    check(world.decorations[0].height == 2 && std::abs(world.decorations[1].elevation - 1.82f) < .00001f &&
              world.decorations[1].height == .34f && world.decorations[2].height == 1 &&
              world.decorations.back().elevation == banner.elevation,
          "Pilaster reaches ceiling; web keeps offset/size; doorway and banner retain dimensions");
    const auto identity = world.fingerprint();
    world.surfaces[unsigned(f.width + 3)].ceiling = 7;
    check(identity != world.fingerprint(), "World identity includes architectural ceilings");
}
void checkArchitecture(const GeneratedFloor& floor) {
    const auto& f = floor.layout;
    const auto ceilings = roomCeilings(f);
    for (int y = 0; y < f.height; ++y)
        for (int x = 0; x < f.width; ++x) {
            const Cell c{x, y};
            const auto region = regionAt(f, c);
            int expected = floorCell(f, c) && region < f.roomCount ? ceilings[region] : 4;
            for (unsigned i = 0; i < f.fixtures.count; ++i) {
                const auto& v = f.fixtures.entries[i];
                if (v.cell == c && (v.kind == FixtureKind::Door || v.kind == FixtureKind::GuardianGate))
                    expected = 4;
            }
            check(floor.map.surface(c).floor == 0 && floor.map.surface(c).ceiling == expected,
                  "Published surfaces honor room membership and thresholds");
        }
    // Architecture consumes fresh authored reservations and ordinary dressing.
    auto layout = f;
    WorldDefinition original;
    BlueprintReservations reservations;
    check(!prepareBlueprintRooms(layout, test::campaignBlueprints(), reservations, &original),
          "Authored reservations reconstruct");
    check(dressUnderkeep(layout, floor.request, original, &reservations),
          "Fresh dressing remains reproducible");
    const auto& actual = floor.map.definition().decorations;
    check(actual.size() == original.decorations.size(), "Architecture preserves decoration count");
    for (unsigned i = 0; i < actual.size(); ++i) {
        const auto& a = actual[i];
        const auto& b = original.decorations[i];
        check(a.cell == b.cell && a.kind == b.kind && a.face == b.face && a.container == b.container,
              "Architecture preserves decoration placement and container identity");
    }
    check(seedFor(floor.request, Domain::Dressing) ==
              seedFor({floor.request.seed, floor.request.depth, 12, 9}, Domain::Dressing),
          "Explicit dressing stream is independent of request identity fields");
}
void blueprintContracts() {
    const auto& catalog = test::campaignBlueprints();
    const auto result = generate({5, 1}, catalog);
    check(bool(result), "Blueprint integration fixture generates");
    const auto& f = result.floor->layout;
    check(f.blueprintCount == 4 && f.blueprintFingerprint == blueprints::fingerprint(catalog),
          "Exact roster and canonical catalog identity");
    BlueprintReservations masks;
    WorldDefinition expected;
    check(!prepareBlueprintRooms(f, catalog, masks, &expected), "Combined arrangement prepares");
    check(expected.scenery.size() <= 36 && expected.ambientFires.size() == 6,
          "All four kits fit deliberate bounds");
    check(!validateBlueprintWorld(f, catalog, result.floor->map),
          "Published world matches authored definitions");
    for (unsigned turn = 0; turn < 4; ++turn) {
        auto r = rotated(f, turn);
        BlueprintReservations rotatedMasks;
        check(!prepareBlueprintRooms(r, catalog, rotatedMasks),
              "Metadata, furniture and sockets rotate together");
    }
    for (unsigned i = 0; i < f.populationCount; ++i) {
        auto c = f.population[i].cell;
        auto n = unsigned(c.y * f.width + c.x);
        auto r = regionAt(f, c);
        check(!masks.blocked[n] &&
                  (!masks.authored[r] || (masks.encounters[n] && !masks.furnished[n] && !masks.rewards[n])),
              "Population respects authored encounter and reward reservations");
    }
    unsigned beds = 0;
    for (const auto& p : result.floor->map.definition().scenery) {
        if (p.model == "sleeping-bay" || p.model == "sleeping-bay-red") {
            ++beds;
            check(result.floor->map.movementBlocked(p.anchor),
                  "Each bed retains its accepted one-cell collision");
        }
    }
    check(beds == 6, "Six accepted beds are present");
    for (const auto& d : result.floor->map.definition().decorations)
        if (blueprintAt(f, regionAt(f, d.cell)))
            check(d.kind == DecorationKind::Chest, "Generic dressing never enters authored interiors");
    auto malformed = f;
    malformed.blueprints[0].sockets = 0;
    check(bool(prepareBlueprintRooms(malformed, catalog, masks)), "Unpublished/missing socket mask rejects");
    malformed = f;
    malformed.blueprints[1].purpose = malformed.blueprints[0].purpose;
    check(bool(prepareBlueprintRooms(malformed, catalog, masks)), "Purpose identity tampering rejects");
    malformed = f;
    ++malformed.blueprints[0].rotation;
    check(bool(prepareBlueprintRooms(malformed, catalog, masks)), "Untransformed geometry rejects");
    auto missing = catalog;
    --missing.roomCount;
    auto failure = generate({5}, missing);
    check(!failure && failure.diagnostics.attempts == 0 &&
              failure.diagnostics.failure.code == Error::Blueprint,
          "Missing roster fails before retries");
    auto duplicate = catalog;
    duplicate.rooms[1].purpose = duplicate.rooms[0].purpose;
    check(!generate({5}, duplicate), "Duplicate purposes fail closed");
    auto incompatible = catalog;
    ++incompatible.rooms[incompatible.room("kitchen").index].ceiling;
    failure = generate({5}, incompatible, true);
    check(!failure && failure.diagnostics.fallback && failure.diagnostics.failure.code == Error::Blueprint,
          "Incompatible fallback never omits or alters required content");
}
void deterministicAndVariety() {
    std::set<LayoutProfile> profiles;
    for (unsigned depth = 1; depth <= 3; ++depth) {
        std::set<std::string> shapes, degrees;
        for (unsigned seed : {0u, 1u, 5u, 8u, 13u, 21u, 34u, 55u}) {
            const auto a = generate({seed, depth}, blobber::test::campaignBlueprints()),
                       b = generate({seed, depth}, blobber::test::campaignBlueprints());
            check(bool(a) && bool(b) && !a.diagnostics.fallback,
                  "fixed normal sample generates without fallback");
            const auto& f = a.floor->layout;
            check(!validate(f) && a.floor->map.connected(f.spawn.cell), "published cells and metadata agree");
            check(a.floor->fingerprint == b.floor->fingerprint && ascii(f) == ascii(b.floor->layout),
                  "deterministic geometry and metadata");
            checkArchitecture(*a.floor);
            profiles.insert(f.profile);
            shapes.insert(signature(f));
            std::array<unsigned, maxRooms> d{};
            for (unsigned i = 0; i < f.connectionCount; ++i) {
                ++d[f.connections[i].a];
                ++d[f.connections[i].b];
            }
            std::sort(d.begin(), d.begin() + f.roomCount);
            std::string degree;
            for (unsigned i = 0; i < f.roomCount; ++i)
                degree += char('0' + d[i]);
            degrees.insert(degree);
            for (unsigned turn = 0; turn < 4; ++turn)
                check(!validate(rotated(f, turn)), "full geometry/progression rotates consistently");
            std::set<unsigned> areas;
            for (unsigned r = 0; r < f.roomCount; ++r) {
                unsigned area = 0;
                for (auto id : f.membership)
                    area += id == r;
                areas.insert(area);
            }
            check(areas.size() >= 4, "room scale changes within a floor");
        }
        check(shapes.size() == 8 && degrees.size() > 1,
              "layouts differ beyond rotation/reflection and vary connectivity");
    }
    check(profiles.size() == 2, "both spatial profiles are exercised");
    std::set<std::uint64_t> streams;
    for (unsigned domain = 1; domain <= 6; ++domain)
        for (unsigned depth = 1; depth <= 3; ++depth)
            streams.insert(seedFor({5, depth}, Domain(domain)));
    check(streams.size() == 18 && seedFor({5}, Domain::Interior, 1) != seedFor({5}, Domain::Interior, 2),
          "domains/depths/regions isolate random streams");
    check(!floorSpec(0) && !floorSpec(4) && floorSpec(2)->keyedSector && floorSpec(3)->finalFloor &&
              std::string(underkeepBiome().assetPack) == "underkeep",
          "floor/biome specification preserves campaign bands");
}
void pacing() {
    constexpr std::array<unsigned, 3> baseline{54, 90, 84};
    for (unsigned depth = 1; depth <= 3; ++depth) {
        std::array<unsigned, 4> walks{};
        for (unsigned seed = 0; seed < 4; ++seed) {
            const auto result = generate({seed, depth}, blobber::test::campaignBlueprints());
            check(bool(result) && !result.diagnostics.fallback, "review floor generates normally");
            const auto m = layoutMetrics(result.floor->layout);
            check(m.walkableCells >= 230 && m.walkableCells <= 390 && m.distinctRoomAreas >= 4 &&
                      m.maxRoomArea >= m.minRoomArea * 2 && m.distinctConnectionLengths >= 3,
                  "review spatial variety/area budgets");
            walks[seed] = m.progressionSteps;
        }
        std::sort(walks.begin(), walks.end());
        const unsigned twiceMedian = walks[1] + walks[2];
        check(twiceMedian * 2 >= baseline[depth - 1] * 3 && twiceMedian * 2 <= baseline[depth - 1] * 5,
              "review median pacing within 25 percent of baseline");
    }
}
void failures() {
    check(!generate({1, 0}, blobber::test::campaignBlueprints()) &&
              !generate({1, 4}, blobber::test::campaignBlueprints()) &&
              !generate({1, 1, 999}, blobber::test::campaignBlueprints()) &&
              !generate({1, 1, 1, 0}, blobber::test::campaignBlueprints()),
          "unsupported generation requests reject");
    const auto good = generate({5}, blobber::test::campaignBlueprints());
    check(bool(good), "owned mutation fixture");
    const auto original = good.floor->layout;
    const auto extreme = generate({UINT32_MAX, 3}, blobber::test::campaignBlueprints());
    check(bool(extreme) && !validate(extreme.floor->layout), "maximum seed publishes validated geometry");
    auto bad = [&](auto mutate) {
        auto f = original;
        mutate(f);
        check(bool(validate(f)), "corrupt explicit geometry rejects");
    };
    bad([](auto& f) { f.roomCount = maxRooms + 1; });
    bad([](auto& f) { f.connectionCount = maxConnections + 1; });
    bad([](auto& f) { f.pathCount = maxCells + 1; });
    bad([](auto& f) { f.width = 41; });
    bad([](auto& f) { f.floor[0] = 1; });
    bad([](auto& f) { f.rooms[0].origin = {-1, -1}; });
    bad([](auto& f) { f.rooms[0].origin = {INT32_MAX, INT32_MAX}; });
    bad([](auto& f) { f.rooms[0].size = {INT32_MAX, INT32_MAX}; });
    bad([](auto& f) {
        f.connections[0].facingA = Heading::North;
        f.connections[0].socketA = {0, 0};
    });
    bad([](auto& f) { f.connections[1] = f.connections[0]; });
    bad([](auto& f) { f.spawn.cell = {0, 0}; });
    bad([](auto& f) {
        f.membership[unsigned(f.rooms[0].anchor.y * f.width + f.rooms[0].anchor.x)] = noRegion;
    });
    bad([](auto& f) { f.paths[0] = 0; });
    bad([](auto& f) { f.connections[0].count = 0; });
    bad([](auto& f) { f.loopConnection = f.connectionCount - 1; });
    bad([](auto& f) {
        for (unsigned i = 0; i < unsigned(f.width * f.height); ++i)
            if (!f.floor[i]) {
                f.floor[i] = 1;
                break;
            }
    });
    bad([](auto& f) {
        const auto c = f.rooms[0].anchor;
        f.floor[unsigned(c.y * f.width + c.x)] = 0;
    });
    auto changed = original;
    changed.profile = original.profile == LayoutProfile::CompactWings ? LayoutProfile::ChamberNetwork
                                                                      : LayoutProfile::CompactWings;
    check(fingerprint(changed) != fingerprint(original), "canonical identity includes profile and geometry");
    changed = original;
    ++changed.paths[0];
    check(fingerprint(changed) != fingerprint(original), "canonical identity includes passage cells");
    constexpr unsigned fallbackSeeds[3][2]{{0, 21}, {0, 13}, {8, 5}};
    for (unsigned depth = 1; depth <= 3; ++depth) {
        std::set<LayoutProfile> fallbackProfiles;
        for (unsigned seed : fallbackSeeds[depth - 1]) {
            const auto a = generate({seed, depth}, blobber::test::campaignBlueprints(), true),
                       b = generate({seed, depth}, blobber::test::campaignBlueprints(), true);
            check(bool(a) && bool(b) && a.diagnostics.fallback && a.diagnostics.attempts == maxAttempts &&
                      a.diagnostics.fallbackId == unsigned(a.floor->layout.profile) + 1 &&
                      !validate(a.floor->layout),
                  "bounded exhaustion publishes checked authored fallback");
            check(a.floor->fingerprint == b.floor->fingerprint, "fallback reproducible");
            checkArchitecture(*a.floor);
            fallbackProfiles.insert(a.floor->layout.profile);
            for (auto rejection : a.diagnostics.rejected)
                check(rejection.code == Error::ForcedFallback, "every exhausted attempt diagnosed");
            auto broken = a.floor->layout;
            broken.floor[unsigned(broken.spawn.cell.y * broken.width + broken.spawn.cell.x)] = 0;
            check(bool(validate(broken)), "broken authored fallback cannot pass validation");
        }
        check(fallbackProfiles.size() == 2, "both authored fallbacks covered at every depth");
    }
    const auto view = diagnosticView(*good.floor);
    check(view.count >= 4 && view.lines[2][0], "prepared diagnostic identity/markers");
}
void progression() {
    for (unsigned depth : {2u, 3u})
        for (bool fallback : {false, true}) {
            const auto result = generate({5, depth}, blobber::test::campaignBlueprints(), fallback);
            check(bool(result), "sector fixture");
            auto f = result.floor->layout;
            unsigned locks = 0;
            for (unsigned i = 0; i < f.fixtures.count; ++i)
                if (f.fixtures.entries[i].key && f.fixtures.entries[i].kind == FixtureKind::Door) {
                    ++locks;
                    auto broken = f;
                    broken.fixtures.entries[i].key = 0;
                    check(bool(validate(broken)), "either sector bypass rejects");
                }
            check(locks == 2, "both entrances protect sector");
            for (unsigned i = 0; i < f.fixtures.count; ++i)
                if (f.fixtures.entries[i].kind == FixtureKind::Key) {
                    auto broken = f;
                    broken.fixtures.entries[i].cell = f.guardianApproach;
                    check(bool(validate(broken)), "key behind its own locks rejects");
                }
            auto broken = f;
            broken.fixtures.entries[f.guardianGate].cell = f.spawn.cell;
            check(bool(validate(broken)), "guardian gate moved off its threshold rejects");
            broken = f;
            broken.fixtures.npcExcluded.reset();
            check(bool(validate(broken)), "unprotected landing rejects");
            broken = f;
            broken.population[0].cell = f.spawn.cell;
            check(bool(validate(broken)), "population cannot occupy a landing");
        }
}
} // namespace
int main() {
    try {
        architectureRules();
        blueprintContracts();
        deterministicAndVariety();
        pacing();
        failures();
        progression();
        std::cout << "Generation contracts passed; retained=" << sizeof(GeneratedFloor) + maxCells
                  << " scratch=" << scratchBytes() << '\n';
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

#include "generation/diagnostic_view.hpp"
#include "generation/floor.hpp"
#include "support/check.hpp"
#include <algorithm>
#include <iostream>
#include <set>
using namespace blobber;
using namespace blobber::generation;
namespace {
FloorLayout rotated(const FloorLayout& source, unsigned turn) {
    auto f = source;
    f.floor.fill(0);
    for (int y = 0; y < f.height; ++y)
        for (int x = 0; x < f.width; ++x) {
            const auto p = rotate({x, y}, f.width, turn);
            f.floor[p.y * f.width + p.x] = source.floor[y * f.width + x];
        }
    f.rotation = (f.rotation + turn) % 4;
    for (unsigned i = 0; i < f.roomCount; ++i) {
        auto& r = f.rooms[i];
        r.site = rotate(r.site, 4, turn);
        r.anchor = rotate(r.anchor, f.width, turn);
        r.rotation = std::uint8_t((r.rotation + turn) % 4);
        r.facing = Heading((unsigned(r.facing) + turn) % 4);
        r.ports = std::uint8_t(((r.ports << turn) | (r.ports >> (4 - turn))) & 15);
    }
    for (unsigned i = 0; i < f.fixtures.count; ++i) {
        auto& v = f.fixtures.entries[i];
        v.cell = rotate(v.cell, f.width, turn);
        v.facing = Heading((unsigned(v.facing) + turn) % 4);
    }
    f.fixtures.npcExcluded.reset();
    for (int y = 0; y < f.height; ++y)
        for (int x = 0; x < f.width; ++x)
            if (source.fixtures.npcExcluded[unsigned(y * f.width + x)]) {
                auto c = rotate({x, y}, f.width, turn);
                f.fixtures.npcExcluded.set(unsigned(c.y * f.width + c.x));
            }
    for (unsigned i = 0; i < f.fixtures.landingCount; ++i)
        f.fixtures.landings[i] = rotate(f.fixtures.landings[i], f.width, turn);
    for (unsigned i = 0; i < f.populationCount; ++i) {
        auto& p = f.population[i];
        p.cell = rotate(p.cell, f.width, turn);
        p.subcell = rotate(p.subcell, 2, turn);
        p.facing = Heading((unsigned(p.facing) + turn) % 4);
    }
    f.guardianReward = rotate(f.guardianReward, f.width, turn);
    f.optionalSupply = rotate(f.optionalSupply, f.width, turn);
    f.guardianApproach = rotate(f.guardianApproach, f.width, turn);
    f.spawn.cell = rotate(f.spawn.cell, f.width, turn);
    f.spawn.heading = Heading((unsigned(f.spawn.heading) + turn) % 4);
    return f;
}
void deterministic() {
    std::set<std::uint64_t> identities;
    std::set<unsigned> families, shapes, rotations;
    // Small, fixed structural sample, not a seed sweep or balance workload.
    for (unsigned seed : {0u, 1u, 5u, 8u, 13u, 21u, 34u, 55u, 89u, UINT32_MAX}) {
        auto a = generate({seed}), b = generate({seed});
        check(bool(a) && bool(b) && !a.floor->diagnostics.fallback,
              "built-in templates generate without fallback");
        check(a.floor->fingerprint == b.floor->fingerprint &&
                  ascii(a.floor->layout) == ascii(b.floor->layout),
              "same seed/versions reproduce cells and role metadata");
        identities.insert(a.floor->fingerprint);
        families.insert(a.floor->layout.family);
        for (unsigned i = 0; i < a.floor->layout.roomCount; ++i) {
            shapes.insert(a.floor->layout.rooms[i].shape);
            rotations.insert(a.floor->layout.rooms[i].rotation);
        }
        check(a.floor->map.connected(a.floor->layout.spawn.cell),
              "published Map agrees with tile validation");
        for (unsigned turn = 0; turn < 4; ++turn)
            check(!validate(rotated(a.floor->layout, turn)),
                  "all four full-layout rotations preserve routes and facing");
    }
    check(identities.size() >= 8 && families.size() == 2 && shapes.size() == 5 && rotations.size() == 4,
          "fixed fixtures exercise meaningful topology/interior/orientation variation");
    std::set<std::uint64_t> streams;
    for (unsigned domain = 1; domain <= 5; ++domain)
        for (unsigned depth = 1; depth <= 3; ++depth)
            streams.insert(seedFor({5, depth}, Domain(domain)));
    check(streams.size() == 15, "depth and phase seeds are separated");
    check(seedFor({5}, Domain::Interior, 1) != seedFor({5}, Domain::Interior, 2) &&
              seedFor({5}, Domain::Topology, 0, 0) != seedFor({5}, Domain::Topology, 0, 1),
          "stable room IDs and attempt number separate draws");
    auto changed = templates();
    changed[3].floor[5 * 7 + 5] = 0;
    const auto original = generate({5}), altered = generate({5}, changed);
    check(bool(altered) && original.floor->runtimeSeed == altered.floor->runtimeSeed,
          "interior edits cannot consume runtime random draws");
    check(original.floor->fingerprint != altered.floor->fingerprint,
          "canonical fingerprint includes actual cells");
}
void failures() {
    check(!generate({1, 0}) && !generate({1, 4}) && !generate({1, 1, 999}) && !generate({1, 1, 1, 0}),
          "unsupported depth/version fails before generation");
    const auto good = generate({5});
    check(bool(good), "fixture generated");
    auto f = good.floor->layout;
    f.roomCount = maxRooms + 1;
    check(validate(f).code == Error::Capacity, "room capacity rejection");
    f = good.floor->layout;
    f.connectionCount = maxConnections + 1;
    check(validate(f).code == Error::Capacity, "connection capacity rejection");
    f = good.floor->layout;
    f.width = 41;
    check(validate(f).code == Error::Capacity, "dimension rejected before indexing");
    f = good.floor->layout;
    f.floor[0] = 1;
    check(validate(f).code == Error::Bounds, "enclosed perimeter");
    f = good.floor->layout;
    f.rooms[1].site = f.rooms[0].site;
    check(validate(f).code == Error::Room, "overlapping room sites");
    f = good.floor->layout;
    f.rooms[0].ports ^= 1;
    check(validate(f).code == Error::Port, "metadata cannot invent a port");
    f = good.floor->layout;
    f.connections[1] = f.connections[0];
    check(validate(f).code == Error::Connection, "duplicate connection");
    f = good.floor->layout;
    f.spawn.cell = {0, 0};
    check(validate(f).code == Error::Anchor, "invalid spawn");
    f = good.floor->layout;
    f.floor[1 * f.width + 1] = 1;
    check(validate(f).code == Error::Port, "unplanned tile outside declared rooms and corridors");
    f = good.floor->layout;
    const auto& r = f.rooms[0];
    const Cell center{4 + r.site.x * 9, 4 + r.site.y * 9};
    for (int y = -2; y <= 2; ++y)
        for (int x = -2; x <= 2; ++x)
            f.floor[(center.y + y) * f.width + center.x + x] = 0;
    f.floor[r.anchor.y * f.width + r.anchor.x] = 1;
    check(validate(f).code == Error::Disconnected,
          "tile flood rejects disconnected interior despite valid graph");
    f = good.floor->layout;
    f.loopConnection = f.connectionCount - 1;
    check(validate(f).code == Error::Loop, "dead-end branch cannot masquerade as useful loop");
    auto catalog = templates();
    catalog[2].floor[0] = 1;
    const auto invalid = generate({5}, catalog);
    check(!invalid && invalid.diagnostics.failure.code == Error::Template &&
              invalid.diagnostics.attempts == 0,
          "malformed authored rim rejects before candidates");
    for (unsigned depth = 1; depth <= 3; ++depth) {
        const auto a = generate({1, depth}, templates(), true), b = generate({99, depth}, templates(), true);
        check(bool(a) && bool(b) && a.diagnostics.fallback && a.diagnostics.attempts == maxAttempts &&
                  a.diagnostics.fallbackId == depth,
              "bounded forced exhaustion publishes validated depth fallback");
        check(a.floor->fingerprint == b.floor->fingerprint && !validate(a.floor->layout),
              "authored fallback layout independent of failed seed");
        for (auto rejection : a.diagnostics.rejected)
            check(rejection.code == Error::ForcedFallback, "retain every forced rejection reason");
    }
    catalog = templates();
    catalog[0].floor.fill(0);
    catalog[0].floor[24] = 1;
    const auto broken = generate({5}, catalog);
    check(!broken && broken.diagnostics.attempts == maxAttempts && broken.diagnostics.fallback &&
              broken.diagnostics.failure.code == Error::Anchor,
          "invalid actual fallback fails without relaxing arrival-clearance validation");
    const auto view = diagnosticView(*good.floor);
    check(view.count >= 4 && view.lines[2][0],
          "prepared diagnostic model contains role markers and identity");
}
} // namespace
int main() {
    try {
        deterministic();
        failures();
        std::cout << "Generation contracts passed; retained upper bound=" << sizeof(GeneratedFloor) + maxCells
                  << " scratch=" << scratchBytes() << '\n';
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}

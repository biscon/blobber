#include "generation/topology.hpp"
#include <cstdlib>
namespace blobber::generation {
namespace {
struct Random {
    std::uint64_t state;
    unsigned next(unsigned bound) noexcept {
        state += 0x9e3779b97f4a7c15ull;
        auto z = state;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
        z ^= z >> 31;
        return unsigned((std::uint64_t(std::uint32_t(z >> 32)) * bound) >> 32);
    }
};
unsigned bit(unsigned i) noexcept {
    return 1u << i;
}
unsigned count(unsigned bits) noexcept {
    unsigned n = 0;
    for (; bits; bits &= bits - 1)
        ++n;
    return n;
}
unsigned neighbours(unsigned i) noexcept {
    unsigned out = 0;
    if (i % 4)
        out |= bit(i - 1);
    if (i % 4 < 3)
        out |= bit(i + 1);
    if (i >= 4)
        out |= bit(i - 4);
    if (i < 12)
        out |= bit(i + 4);
    return out;
}
bool connected(unsigned mask) noexcept {
    if (!mask)
        return false;
    unsigned seen = mask & (~mask + 1), frontier = seen;
    while (frontier) {
        unsigned next = 0;
        for (unsigned i = 0; i < 16; ++i)
            if (frontier & bit(i))
                next |= neighbours(i);
        frontier = next & mask & ~seen;
        seen |= frontier;
    }
    return seen == mask;
}
unsigned leaves(unsigned mask, unsigned reserved) noexcept {
    unsigned out = 0;
    for (unsigned i = 0; i < 16; ++i)
        if (!(bit(i) & (mask | reserved)) && (neighbours(i) & mask & ~bit(6)))
            out |= bit(i);
    return out;
}
unsigned pick(unsigned mask, Random& rng) noexcept {
    auto n = rng.next(count(mask));
    for (unsigned i = 0; i < 16; ++i)
        if ((mask & bit(i)) && n-- == 0)
            return i;
    return 16;
}
void room(FloorLayout& f, unsigned site, Role role) noexcept {
    const auto i = f.roomCount++;
    f.rooms[i].site = {int(site % 4), int(site / 4)};
    f.rooms[i].id = std::uint8_t(i);
    f.rooms[i].role = role;
}
void edge(FloorLayout& f, unsigned a, unsigned b) noexcept {
    f.connections[f.connectionCount++] = {std::uint8_t(a), std::uint8_t(b)};
}
} // namespace
bool buildTopology(FloorLayout& f, GenerationRequest request, unsigned attempt, bool fallback) noexcept {
    Random rng{seedFor(request, Domain::Topology, 0, attempt)};
    f.family = fallback ? 1 : 0;
    f.rotation = fallback ? request.depth - 1 : rng.next(4);
    if (fallback) {
        for (unsigned s : {4u, 5u, 6u, 10u, 9u, 8u})
            room(f, s, Role::Junction);
        for (unsigned i = 0; i < 6; ++i)
            edge(f, i, (i + 1) % 6);
        room(f, 0, Role::Entrance);
        edge(f, 0, 6);
        room(f, 7, Role::Approach);
        edge(f, 2, 7);
        room(f, 3, Role::Objective);
        edge(f, 7, 8);
        room(f, 12, Role::Optional);
        edge(f, 5, 9);
    } else {
        const unsigned n = 6 + rng.next(3), optional = n == 8 ? 1 : 1 + rng.next(2);
        const unsigned goal = rng.next(2) ? 11 : 3, reserved = bit(7) | bit(goal);
        unsigned chosen = 0, admitted = 0;
        // Exhaustive finite domain; reservoir sampling needs only a single mask.
        for (unsigned mask = 0; mask < (1u << 16); ++mask) {
            if ((mask & reserved) || !(mask & bit(6)) || count(mask) != n ||
                count(neighbours(6) & mask) != 2 || !connected(mask & ~bit(6)) ||
                count(leaves(mask, reserved)) < optional + 1)
                continue;
            if (rng.next(++admitted) == 0)
                chosen = mask;
        }
        if (!chosen)
            return false;
        std::array<unsigned, 16> ids{};
        for (unsigned i = 0; i < 16; ++i)
            if (chosen & bit(i)) {
                ids[i] = f.roomCount;
                room(f, i, Role::Junction);
            }
        bool loop = false;
        for (unsigned a = 0; a < 16; ++a)
            if (chosen & bit(a))
                for (unsigned b = a + 1; b < 16; ++b)
                    if ((chosen & neighbours(a) & bit(b))) {
                        if (!loop && (a == 6 || b == 6)) {
                            f.loopConnection = f.connectionCount;
                            loop = true;
                        }
                        edge(f, ids[a], ids[b]);
                    }
        unsigned available = leaves(chosen, reserved);
        const auto attach = [&](Role role) {
            const auto site = pick(available, rng);
            available &= ~bit(site);
            const auto neighbour = pick(neighbours(site) & chosen & ~bit(6), rng);
            const auto id = f.roomCount;
            room(f, site, role);
            edge(f, ids[neighbour], id);
        };
        attach(Role::Entrance);
        const auto approach = f.roomCount;
        room(f, 7, Role::Approach);
        edge(f, ids[6], approach);
        room(f, goal, Role::Objective);
        edge(f, approach, approach + 1);
        for (unsigned i = 0; i < optional; ++i)
            attach(Role::Optional);
    }
    for (unsigned i = 0; i < f.roomCount; ++i)
        f.rooms[i].site = rotate(f.rooms[i].site, 4, f.rotation);
    return true;
}
} // namespace blobber::generation

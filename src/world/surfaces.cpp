#include "world/surfaces.hpp"
#include <cstring>
namespace blobber {
std::uint64_t WorldDefinition::fingerprint() const noexcept {
    std::uint64_t h = 14695981039346656037ull;
    auto add = [&](std::uint64_t v) {
        for (unsigned i = 0; i < 8; ++i) {
            h ^= (v >> (i * 8)) & 255;
            h *= 1099511628211ull;
        }
    };
    auto real = [&](float f) {
        std::uint32_t bits;
        std::memcpy(&bits, &f, sizeof(bits));
        add(bits);
    };
    add(1);
    add(surfaces.size());
    for (auto s : surfaces) {
        add(std::uint16_t(s.floor));
        add(std::uint16_t(s.ceiling));
        add(s.wallMaterial);
        add(s.floorMaterial);
        add(s.ceilingMaterial);
        add(unsigned(s.environment));
    }
    add(decorations.size());
    for (const auto& d : decorations) {
        add(d.cell.x);
        add(d.cell.y);
        add(unsigned(d.face));
        add(unsigned(d.kind));
        add(d.material);
        real(d.offset);
        real(d.elevation);
        real(d.width);
        real(d.height);
        real(d.depth);
    }
    return h;
}
} // namespace blobber

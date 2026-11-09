#include "world/surfaces.hpp"
#include <cstring>
namespace blobber {
const char* decorationName(DecorationKind k) noexcept {
    constexpr const char* names[]{"crest",  "vine",    "bench",   "urn",      "damp-a",    "damp-b",
                                  "cracks", "soot",    "blood",   "banner-a", "banner-b",  "web-a",
                                  "web-b",  "bones-a", "bones-b", "pilaster", "wall-base", "door-surround",
                                  "cage",   "rack",    "chest"};
    return unsigned(k) < decorationKinds ? names[unsigned(k)] : "invalid";
}
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
    add(4);
    add(assetPack.size());
    for (unsigned char c : assetPack)
        add(c);
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
        add(d.container);
        add(d.sealed);
        real(d.offset);
        real(d.elevation);
        real(d.width);
        real(d.height);
        real(d.depth);
    }
    add(buildings.size());
    for (const auto& b : buildings) {
        add(b.model.size());
        for (unsigned char c : b.model)
            add(c);
        add(b.anchor.x);
        add(b.anchor.y);
        add(unsigned(b.facing));
        add(b.footprint.x);
        add(b.footprint.y);
        add(b.width);
        add(b.depth);
    }
    add(scenery.size());
    for (const auto& s : scenery) {
        add(s.model.size());
        for (unsigned char c : s.model)
            add(c);
        add(s.anchor.x);
        add(s.anchor.y);
        add(unsigned(s.facing));
        add(s.footprint.x);
        add(s.footprint.y);
        add(s.width);
        add(s.depth);
        if (!s.blocking)
            add(0x6465636f72617465ull); // Default true preserves established fingerprints.
    }
    // Absent extension retains established dungeon generation fingerprints.
    if (hearth) {
        add(0x686561727468ull);
        real(hearth->x);
        real(hearth->y);
        real(hearth->z);
    }
    if (!ambientFires.empty()) {
        add(0x6669726573ull);
        add(ambientFires.size());
        for (const auto& f : ambientFires) {
            real(f.x);
            real(f.y);
            real(f.z);
        }
    }
    return h;
}
} // namespace blobber

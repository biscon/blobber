#pragma once
#include "generation/floor.hpp"
namespace blobber::generation {
struct DiagnosticView {
    struct Marker {
        Cell cell;
        Role role;
    };
    std::array<Marker, maxRooms> markers{};
    unsigned count = 0;
    std::array<std::array<char, 96>, 10> lines{};
};
// Prepare once; borrowed by presentation only while the floor owner is alive.
DiagnosticView diagnosticView(const GeneratedFloor&) noexcept;
} // namespace blobber::generation

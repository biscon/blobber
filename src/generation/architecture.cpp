#include "generation/architecture.hpp"
#include "generation/blueprint_rooms.hpp"
namespace blobber::generation {
RoomCeilings roomCeilings(const FloorLayout& f) noexcept {
    std::array<unsigned, maxRooms> areas{};
    for (int i = 0; i < f.width * f.height; ++i)
        if (f.floor[unsigned(i)] && f.membership[unsigned(i)] < f.roomCount)
            ++areas[f.membership[unsigned(i)]];
    RoomCeilings heights{};
    heights.fill(4);
    for (unsigned i = 0; i < f.roomCount; ++i) {
        const auto& r = f.rooms[i];
        if (r.role == Role::Entrance || r.role == Role::Objective)
            continue;
        if (const auto* b = blueprintAt(f, i)) {
            heights[i] = b->ceiling;
            continue;
        }
        heights[i] = areas[i] < 20 ? 5 : 6;
        // Stone shape IDs: 1 = pillar chamber, 2 = L room.
        if (areas[i] >= 30 && r.size.x >= 5 && r.size.y >= 5 && r.shape != 2)
            heights[i] = r.shape == 1 ? 8 : 7;
    }
    return heights;
}
void prepareArchitecture(const FloorLayout& f, WorldDefinition& world) {
    const auto ceilings = roomCeilings(f);
    const auto count = unsigned(f.width * f.height);
    world.surfaces.assign(count, CellSurface{});
    for (unsigned i = 0; i < count; ++i)
        if (f.floor[i] && f.membership[i] < f.roomCount)
            world.surfaces[i].ceiling = ceilings[f.membership[i]];
    for (unsigned i = 0; i < f.fixtures.count; ++i) {
        const auto& fixture = f.fixtures.entries[i];
        if (fixture.kind == FixtureKind::Door || fixture.kind == FixtureKind::GuardianGate)
            world.surfaces[unsigned(fixture.cell.y * f.width + fixture.cell.x)].ceiling = 4;
    }
    for (auto& d : world.decorations) {
        const auto ceiling = elevation(world.surfaces[unsigned(d.cell.y * f.width + d.cell.x)].ceiling);
        if (d.kind == DecorationKind::Pilaster)
            d.height = ceiling;
        else if (d.web())
            d.elevation += ceiling - 1.f;
    }
}
} // namespace blobber::generation

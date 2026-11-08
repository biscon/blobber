#include "game/smithy_interior.hpp"
#include "game/campaign.hpp"
#include "game/dialogue.hpp"
#include <algorithm>
#include <stdexcept>
namespace blobber {
NpcInstance blacksmithPlacement(const std::vector<std::string>& types) {
    const auto type = std::find(types.begin(), types.end(), "blacksmith");
    if (type == types.end())
        throw std::invalid_argument("Campaign requires the blacksmith appearance");
    NpcInstance npc{
        {std::uint16_t(type - types.begin())}, smithyResidentCell, smithyResidentSubcell, Heading::South};
    npc.role = NpcRole::Resident;
    npc.residentId = dialogueKey("sanctuary.blacksmith");
    return npc;
}
SanctuaryHub smithyInterior() {
    constexpr int size = 12;
    std::vector<std::string> rows(size, std::string(size, '#'));
    for (int y = 2; y <= 9; ++y)
        for (int x = 2; x <= 9; ++x)
            rows[y][x] = '.';
    WorldDefinition world;
    world.assetPack = "sanctuary-smithy";
    world.surfaces.resize(size * size);
    for (auto& surface : world.surfaces) {
        surface.ceiling = 6;
        surface.environment = Environment::Daylight;
    }
    for (int x = 2; x <= 9; ++x) {
        world.buildings.push_back({"wall", {x, 1}, {0, 0}, Heading::South, 1, 1});
        world.buildings.push_back({x == 5 ? "exit-door" : "wall", {x, 10}, {0, 0}, Heading::North, 1, 1});
    }
    for (int y = 2; y <= 9; ++y) {
        world.buildings.push_back({"wall", {1, y}, {0, 0}, Heading::East, 1, 1});
        world.buildings.push_back({y == 3 ? "window-wall" : "wall", {10, y}, {0, 0}, Heading::West, 1, 1});
    }
    world.scenery = {
        {"forge", {2, 2}, {0, 0}, Heading::East, 2, 1},
        {"shelves", {6, 2}, {-1, 0}, Heading::North, 3, 1},
        {"counter", {6, 4}, {-1, 0}, Heading::North, 3, 1},
        {"anvil", {3, 5}, {0, 0}, Heading::North, 1, 1},
        {"quench-tub", {2, 5}, {0, 0}, Heading::North, 1, 1},
        {"charcoal", {2, 7}, {0, 0}, Heading::North, 1, 1},
        {"iron-stock", {2, 8}, {0, 0}, Heading::North, 1, 1},
        {"equipment", {9, 6}, {0, 0}, Heading::West, 3, 1},
    };
    world.hearth = AmbientFire{2.62f, .28f, 3.0f};
    const Player entry{{5, 9}, Heading::North}, exit{{5, 9}, Heading::South};
    SanctuaryHub result{Map(rows, std::move(world)), entry, entry, exit, {}, {}};
    result.fixtures.discovery = true;
    result.fixtures.landingCount = 1;
    result.fixtures.landings[0] = entry.cell;
    result.fixtures.npcExcluded.set(std::size_t(entry.cell.y * size + entry.cell.x));
    if (!result.map.connected(entry.cell))
        throw std::invalid_argument("Smithy furniture disconnects walking routes");
    return result;
}
void validateSmithyInterior(const Map& map, const CampaignLocationState& state) {
    const auto expected = smithyInterior();
    const auto& f = state.fixtures;
    const auto& e = expected.fixtures;
    const auto resident = blacksmithPlacement(state.types);
    if (state.placements.size() != 1)
        throw std::invalid_argument("Smithy requires exactly one blacksmith");
    const auto& npc = state.placements[0];
    if (npc.type.index != resident.type.index || npc.role != NpcRole::Resident ||
        npc.residentId != resident.residentId || !(npc.cell == resident.cell) ||
        !(npc.subcell == resident.subcell) || npc.facing != resident.facing ||
        npc.animation != NpcAnimation::Idle || npc.phaseMs || npc.monster ||
        npc.ai.routine != NpcRoutine::Guard || npc.ai.waypointCount)
        throw std::invalid_argument("Unsupported blacksmith placement");
    if (map.width() != expected.map.width() || map.height() != expected.map.height() ||
        map.definition().fingerprint() != expected.map.definition().fingerprint() ||
        !(state.entry == expected.entry) || !(state.exit == expected.exit) || state.objective.count ||
        state.objective.rewardCount || state.recovery.groundCount || state.recovery.sanctuaryCount ||
        state.recovery.freeRestEverywhere || f.count || f.depth || !f.discovery ||
        f.landingCount != e.landingCount || f.landings != e.landings || f.npcExcluded != e.npcExcluded)
        throw std::invalid_argument("Unsupported Sanctuary smithy definition");
    for (int y = 0; y < map.height(); ++y)
        for (int x = 0; x < map.width(); ++x)
            if (map.solid({x, y}) != expected.map.solid({x, y}))
                throw std::invalid_argument("Unsupported Sanctuary smithy terrain");
}
} // namespace blobber

#include "game/sanctuary_hub.hpp"
#include "game/campaign.hpp"
#include "game/dialogue.hpp"
#include "world/sanctuary_city.hpp"
#include <algorithm>
#include <stdexcept>
namespace blobber {
std::vector<NpcInstance> sanctuaryResidents(const std::vector<std::string>& types) {
    const char* appearances[]{"mayor", "boric", "balin"};
    const char* identities[]{"sanctuary.mayor", "sanctuary.boric", "sanctuary.balin"};
    const Cell cells[]{{9, 8}, {8, 8}, {8, 9}};
    const Cell subcells[]{{0, 1}, {1, 0}, {1, 0}};
    std::vector<NpcInstance> result;
    result.reserve(3);
    for (unsigned i = 0; i < 3; ++i) {
        const auto type = std::find(types.begin(), types.end(), appearances[i]);
        if (type == types.end())
            throw std::invalid_argument("Campaign requires Sanctuary resident appearances");
        NpcInstance npc{{std::uint16_t(type - types.begin())}, cells[i], subcells[i], Heading::East};
        npc.role = NpcRole::Resident;
        npc.residentId = dialogueKey(identities[i]);
        result.push_back(npc);
    }
    return result;
}
SanctuaryHub sanctuaryHub() {
    auto city = sanctuaryCity();
    auto returning = city.dungeonLanding;
    returning.heading = Heading((unsigned(returning.heading) + 2) % 4);
    SanctuaryHub hub{std::move(city.map), city.spawn, city.recallArrival, returning, {}, {}};
    hub.fixtures.count = 1;
    hub.fixtures.discovery = true;
    hub.fixtures.entries[0] = {city.dungeonFixture, city.dungeonLanding.heading, FixtureKind::Downstairs};
    hub.fixtures.landingCount = 2;
    hub.fixtures.landings = {hub.entry.cell, hub.exit.cell};
    for (const auto c : {hub.spawn.cell, hub.entry.cell, hub.exit.cell, city.dungeonFixture})
        hub.fixtures.npcExcluded.set(std::size_t(c.y * hub.map.width() + c.x));
    for (const auto* name : {"inn", "smithy"}) {
        const auto approach = city.doorway(name).approach.cell;
        hub.fixtures.npcExcluded.set(std::size_t(approach.y * hub.map.width() + approach.x));
    }
    return hub;
}
void validateSanctuaryHub(const Map& map, const CampaignLocationState& state) {
    const auto expected = sanctuaryHub();
    const auto residents = sanctuaryResidents(state.types);
    if (state.placements.size() != residents.size())
        throw std::invalid_argument("Sanctuary requires its three residents");
    for (std::size_t i = 0; i < residents.size(); ++i) {
        const auto& npc = state.placements[i];
        const auto& canonical = residents[i];
        if (npc.type.index != canonical.type.index || npc.role != NpcRole::Resident ||
            npc.residentId != canonical.residentId || !(npc.cell == canonical.cell) ||
            !(npc.subcell == canonical.subcell) || npc.facing != canonical.facing ||
            npc.animation != NpcAnimation::Idle || npc.phaseMs || npc.monster ||
            npc.ai.routine != NpcRoutine::Guard || npc.ai.waypointCount)
            throw std::invalid_argument("Unsupported Sanctuary resident placement");
    }
    const auto& f = state.fixtures;
    const auto& e = expected.fixtures;
    if (map.width() != expected.map.width() || map.height() != expected.map.height() ||
        map.definition().fingerprint() != expected.map.definition().fingerprint() ||
        !(state.entry == expected.entry) || !(state.exit == expected.exit) || state.objective.count ||
        state.recovery.groundCount || state.recovery.sanctuaryCount || state.recovery.freeRestEverywhere ||
        f.count != e.count || f.discovery != e.discovery || f.depth != e.depth ||
        f.landingCount != e.landingCount || f.landings != e.landings || f.npcExcluded != e.npcExcluded ||
        !(f.entries[0].cell == e.entries[0].cell) || f.entries[0].facing != e.entries[0].facing ||
        f.entries[0].kind != e.entries[0].kind || f.entries[0].key != e.entries[0].key)
        throw std::invalid_argument("Unsupported Sanctuary city definition");
    for (int y = 0; y < map.height(); ++y)
        for (int x = 0; x < map.width(); ++x)
            if (map.solid({x, y}) != expected.map.solid({x, y}))
                throw std::invalid_argument("Unsupported Sanctuary city terrain");
}
} // namespace blobber

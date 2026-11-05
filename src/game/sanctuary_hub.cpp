#include "game/sanctuary_hub.hpp"
#include "game/campaign.hpp"
#include "world/sanctuary_city.hpp"
#include <stdexcept>
namespace blobber {
SanctuaryHub sanctuaryHub() {
    auto city = sanctuaryCity();
    auto returning = city.dungeonLanding;
    returning.heading = Heading((unsigned(returning.heading) + 2) % 4);
    SanctuaryHub hub{std::move(city.map), city.spawn, city.rest, returning, {}, {}};
    hub.fixtures.count = 1;
    hub.fixtures.discovery = true;
    hub.fixtures.entries[0] = {city.dungeonFixture, city.dungeonLanding.heading, FixtureKind::Downstairs};
    hub.fixtures.landingCount = 2;
    hub.fixtures.landings = {hub.entry.cell, hub.exit.cell};
    for (const auto c : {hub.spawn.cell, hub.entry.cell, hub.exit.cell, city.dungeonFixture})
        hub.fixtures.npcExcluded.set(std::size_t(c.y * hub.map.width() + c.x));
    hub.recovery.sanctuaryCount = 1;
    hub.recovery.sanctuaries[0] = city.rest.cell;
    return hub;
}
void validateSanctuaryHub(const Map& map, const CampaignLocationState& state) {
    const auto expected = sanctuaryHub();
    const auto& f = state.fixtures;
    const auto& e = expected.fixtures;
    if (map.width() != expected.map.width() || map.height() != expected.map.height() ||
        map.definition().fingerprint() != expected.map.definition().fingerprint() ||
        !(state.entry == expected.entry) || !(state.exit == expected.exit) || !state.placements.empty() ||
        state.objective.count || state.recovery.groundCount || state.recovery.sanctuaryCount != 1 ||
        !(state.recovery.sanctuaries[0] == expected.entry.cell) || f.count != e.count ||
        f.discovery != e.discovery || f.depth != e.depth || f.landingCount != e.landingCount ||
        f.landings != e.landings || f.npcExcluded != e.npcExcluded ||
        !(f.entries[0].cell == e.entries[0].cell) || f.entries[0].facing != e.entries[0].facing ||
        f.entries[0].kind != e.entries[0].kind || f.entries[0].key != e.entries[0].key)
        throw std::invalid_argument("Unsupported Sanctuary city definition");
    for (int y = 0; y < map.height(); ++y)
        for (int x = 0; x < map.width(); ++x)
            if (map.solid({x, y}) != expected.map.solid({x, y}))
                throw std::invalid_argument("Unsupported Sanctuary city terrain");
}
} // namespace blobber

#pragma once
#include "app/options.hpp"
#include "generation/diagnostic_view.hpp"
#include "platform/presenter.hpp"
#include <optional>
namespace blobber {
class WorldRenderer;
class CampaignRun;
struct ArenaScenario;
class CampaignStore;
// Standalone sessions start from idle placements. Campaigns already own restored
// actors, so rebuilding initial placements against their current pose is invalid.
std::optional<NpcWorld> gameplayPlacements(const Map& map, const NpcWorld& source, Cell spawn = Player{}.cell,
                                           const CampaignRun* campaign = nullptr);
enum class PlayOutcome { MainMenu, ArenaSetup, Quit };
struct GameplayLaunch {
    Player spawn;
    std::uint32_t seed = 5;
    bool arena = false, enableAi = true;
    const generation::DiagnosticView* diagnostics = nullptr;
    const FloorFixtures* fixtures = nullptr;
    const FloorObjectiveDefinition* objective = nullptr;
    CampaignRun* campaign = nullptr;
    CampaignStore* saves = nullptr;
    const ArenaScenario* arenaScenario = nullptr;
};
PlayOutcome runGameplay(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
                        const std::vector<NpcTimings>& timings, Presenter& presenter, const Options& options,
                        WorldRenderer& world, const RecoveryLevel& recovery = {},
                        const rules::PresetParty* starting = nullptr, const GameplayLaunch* launch = nullptr);
} // namespace blobber

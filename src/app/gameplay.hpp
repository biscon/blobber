#pragma once
#include "app/options.hpp"
#include "generation/diagnostic_view.hpp"
#include "platform/presenter.hpp"
namespace blobber {
class WorldRenderer;
class CampaignRun;
class CampaignStore;
NpcWorld gameplayPlacements(const Map& map, const NpcWorld& source, Cell spawn = Player{}.cell);
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
};
PlayOutcome runGameplay(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
                        const std::vector<NpcTimings>& timings, Presenter& presenter, const Options& options,
                        WorldRenderer& world, const RecoveryLevel& recovery = {},
                        const rules::PresetParty* starting = nullptr, const GameplayLaunch* launch = nullptr);
} // namespace blobber

#pragma once
#include "app/options.hpp"
#include "platform/presenter.hpp"
#include "render/frame_pipeline.hpp"
namespace blobber {
NpcWorld gameplayPlacements(const Map& map, const NpcWorld& source, Cell spawn = Player{}.cell);
Player combatCamera(const Map& map, const NpcWorld& placements);
enum class PlayOutcome { MainMenu, ArenaSetup, Quit };
struct GameplayLaunch {
    Player spawn;
    std::uint32_t seed = 5;
    bool arena = false, enableAi = true;
};
PlayOutcome runGameplay(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
                        const std::vector<NpcTimings>& timings, const SceneRenderer& renderer,
                        Presenter& presenter, const Options& options, const RecoveryLevel& recovery = {},
                        const rules::PresetParty* starting = nullptr, const GameplayLaunch* launch = nullptr);
void captureCombat(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
                   const std::vector<NpcTimings>& timings, const SceneRenderer& renderer,
                   Presenter& presenter, const std::filesystem::path& directory);
} // namespace blobber

#pragma once
#include "app/options.hpp"
#include "platform/presenter.hpp"
#include "render/frame_pipeline.hpp"
namespace blobber {
NpcWorld gameplayPlacements(const Map& map, const NpcWorld& source);
Player combatCamera(const Map& map, const NpcWorld& placements);
void runGameplay(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
                 const std::vector<NpcTimings>& timings, const SceneRenderer& renderer, Presenter& presenter,
                 const Options& options, const RecoveryLevel& recovery = {});
void captureCombat(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
                   const std::vector<NpcTimings>& timings, const SceneRenderer& renderer,
                   Presenter& presenter, const std::filesystem::path& directory);
} // namespace blobber

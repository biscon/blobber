#pragma once
#include "app/gameplay.hpp"
namespace blobber {
Map aiDemoMap();
Player aiDemoSpawn() noexcept;
NpcWorld aiDemoPlacements(const Map& map, const NpcWorld& source);
void captureAi(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
               const std::vector<NpcTimings>& timings, const SceneRenderer& renderer, Presenter& presenter,
               const std::filesystem::path& directory);
} // namespace blobber

#pragma once
#include "app/gameplay.hpp"
namespace blobber {
void captureHud(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
                const std::vector<NpcTimings>& timings, const SceneRenderer& renderer, Presenter& presenter,
                const std::filesystem::path& directory);
}

#pragma once
#include "platform/presenter.hpp"
#include "render/frame_pipeline.hpp"
#include <filesystem>

namespace blobber {
// Explicit diagnostic path: fresh state per scenario, never normal frame work.
void captureActors(const Map& map, const Party& party, const rules::Catalog& catalog,
                   const NpcWorld& placements, const std::vector<NpcTimings>& timings,
                   const SceneRenderer& renderer, Presenter& presenter,
                   const std::filesystem::path& directory);
} // namespace blobber

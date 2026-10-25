#pragma once
#include "platform/presenter.hpp"
#include "render/frame_pipeline.hpp"
namespace blobber {
// Diagnostics only: fresh owners per scenario prevent fixture state leaking between runs.
void captureUi(const Map& map, const Party& party, const rules::Catalog& catalog, const NpcWorld& placements,
               const std::vector<NpcTimings>& timings, const SceneRenderer& renderer, Presenter& presenter,
               const std::filesystem::path& directory);
void runUiDemo(const Map& map, const Party& party, const rules::Catalog& catalog, const NpcWorld& placements,
               const std::vector<NpcTimings>& timings, const SceneRenderer& renderer, Presenter& presenter);
} // namespace blobber

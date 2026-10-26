#pragma once
#include "app/gameplay.hpp"
namespace blobber {
Map hybridDemoMap();
Player hybridDemoSpawn() noexcept;
NpcWorld hybridDemoPlacements(const Map&, const NpcWorld&, const rules::Catalog&);
void captureHybrid(const Map&, const rules::Catalog&, const NpcWorld&, const std::vector<NpcTimings>&,
                   const SceneRenderer&, Presenter&, const std::filesystem::path&);
} // namespace blobber

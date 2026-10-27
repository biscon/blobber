#pragma once
#include "app/gameplay.hpp"
namespace blobber {
NpcWorld recoveryDemoPlacements(const Map&, const NpcWorld&);
RecoveryLevel recoveryDemoLevel(const rules::Catalog&);
void captureRecovery(const rules::Catalog&, const NpcWorld&, const std::vector<NpcTimings>&,
                     const SceneRenderer&, Presenter&, const std::filesystem::path&);
} // namespace blobber

#pragma once
#include "app/gameplay.hpp"
namespace blobber {
void captureInventory(const Map&, const rules::Catalog&, const NpcWorld&, const std::vector<NpcTimings>&,
                      const SceneRenderer&, Presenter&, const std::filesystem::path&);
}

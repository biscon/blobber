#pragma once
#include "app/inventory_captures.hpp"
namespace blobber {
void captureBuilds(const Map&, const rules::Catalog&, const NpcWorld&, const std::vector<NpcTimings>&,
                   const SceneRenderer&, Presenter&, const std::filesystem::path&);
}

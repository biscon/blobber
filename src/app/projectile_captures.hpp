#pragma once
#include "app/archer_captures.hpp"
namespace blobber {
void captureProjectiles(const rules::Catalog&, const NpcWorld&, const std::vector<NpcTimings>&,
                        const SceneRenderer&, Presenter&, const std::filesystem::path&);
}

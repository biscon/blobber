#pragma once
#include "app/build_captures.hpp"
namespace blobber {
void captureProgression(const rules::Catalog&, const NpcWorld&, const std::vector<NpcTimings>&,
                        const SceneRenderer&, Presenter&, const std::filesystem::path&);
}

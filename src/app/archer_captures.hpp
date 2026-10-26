#pragma once
#include "app/actor_captures.hpp"

namespace blobber {
// Shipped-art diagnostics; no source image loading or OS input automation.
void captureArchers(const rules::Catalog&, const NpcWorld&, const std::vector<NpcTimings>&,
                    const SceneRenderer&, Presenter&, const std::filesystem::path&);
} // namespace blobber

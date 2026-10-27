#pragma once
#include "app/projectile_captures.hpp"
namespace blobber {
void captureSpells(const rules::Catalog&, const NpcWorld&, const std::vector<NpcTimings>&,
                   const SceneRenderer&, Presenter&, const std::filesystem::path&);
}

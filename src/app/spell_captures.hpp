#pragma once
#include "app/projectile_captures.hpp"
namespace blobber {
void captureSpellReview(const rules::Catalog&, const NpcWorld&, const std::vector<NpcTimings>&,
                        const SceneRenderer&, Presenter&, const std::filesystem::path&);
void captureSpells(const rules::Catalog&, const NpcWorld&, const std::vector<NpcTimings>&,
                   const SceneRenderer&, Presenter&, const std::filesystem::path&);
} // namespace blobber

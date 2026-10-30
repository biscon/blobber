#pragma once
#include "app/gameplay.hpp"
#include "generation/population.hpp"
namespace blobber {
void captureGuardians(const generation::GeneratedFloor&, const generation::PreparedPopulation&,
                      const NpcWorld&, const rules::Catalog&, const rules::PresetParty&,
                      const std::vector<NpcTimings>&, const SceneRenderer&, Presenter&,
                      const std::filesystem::path&);
}

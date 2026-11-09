#pragma once
#include "assets/world_assets.hpp"
#include "world/blueprints.hpp"
namespace blobber::blueprints {
// Offline only. Adds an isolated review pack; leaves Underkeep bindings untouched.
void appendBlockouts(const BlueprintCatalog&, world_assets::Writer&, std::vector<world_assets::Pack>&,
                     std::vector<world_assets::PropMesh>&);
} // namespace blobber::blueprints

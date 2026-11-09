#pragma once
#include "assets/world_assets.hpp"
#include "world/blueprints.hpp"

namespace blobber::blueprints {
// Preparation only. The publication handles finite data, normals, winding and UVs;
// these checks enforce the tighter authored kit bounds and grouped mesh budget.
void validateKitArt(const Kit&, const world_assets::PropMesh&);
void validatePublishedArt(const BlueprintCatalog&, const ReviewScene&, const world_assets::Publication&);
} // namespace blobber::blueprints

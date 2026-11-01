#pragma once
#include "generation/floor.hpp"
namespace blobber::generation {
// Preparation-only. Adds independent cosmetic choices and validated furniture.
bool dressUnderkeep(FloorLayout&, GenerationRequest, WorldDefinition&);
} // namespace blobber::generation

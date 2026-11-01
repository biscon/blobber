#pragma once
#include "generation/floor.hpp"
namespace blobber::generation {
// Preparation-only, bounded graph construction. False lets the caller reject an attempt.
bool buildTopology(FloorLayout&, GenerationRequest, unsigned attempt, bool fallback) noexcept;
} // namespace blobber::generation

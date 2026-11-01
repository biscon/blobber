#pragma once
#include "generation/floor.hpp"
namespace blobber::generation {
// Preparation-only, bounded stone geometry construction. False lets the caller reject an attempt.
bool buildFallback(FloorLayout&) noexcept;
bool buildStoneLayout(FloorLayout&, GenerationRequest, unsigned attempt, bool fallback) noexcept;
} // namespace blobber::generation

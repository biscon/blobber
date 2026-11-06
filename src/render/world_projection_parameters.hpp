#pragma once

namespace blobber::world_render {
// Fraction from viewport top. B (.40f) accepted 2026-11-06; original reference: .34f.
// Shared by CPU/GPU projection, clipping, picking and sky reconstruction.
constexpr float viewHorizon = .40f;
} // namespace blobber::world_render

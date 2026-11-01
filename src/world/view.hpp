#pragma once

namespace blobber {
// Fixed first-person horizontal projection (84 degrees); shared by simulation
// body-axis eligibility and rendering, independent of viewport resolution.
constexpr float partyCameraSetback = .4f;
constexpr float partyViewHalfTangent = .900404044f;
} // namespace blobber

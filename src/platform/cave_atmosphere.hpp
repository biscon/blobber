#pragma once
#include "platform/gpu_resources.hpp"
#include <string>

namespace blobber {
// Preparation-only distance field for cave surfaces, signed in cell units.
GpuLease<TextureHandle> prepareCaveShore(GpuResources&, world_render::Scene&);
std::string withCaveAtmosphere(std::string shader);
} // namespace blobber

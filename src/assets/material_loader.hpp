#pragma once
#include "render/material.hpp"
#include <filesystem>

namespace blobber {
Material loadMaterial(const std::filesystem::path& path);
Materials loadMaterials(const std::filesystem::path& directory);
} // namespace blobber

#pragma once
#include "world/blueprints.hpp"
#include <filesystem>
namespace blobber::blueprints {
constexpr std::size_t maxJsonBytes = 256 * 1024;
BlueprintCatalog decode(std::string_view json);
BlueprintCatalog load(const std::filesystem::path&);
} // namespace blobber::blueprints

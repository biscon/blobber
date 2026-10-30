#pragma once
#include "generation/floor.hpp"
#include <nlohmann/json.hpp>
namespace blobber::generation {
nlohmann::json report(const GenerationResult&, GenerationRequest, bool includeLayout = true);
nlohmann::json report(const GeneratedFloor&, bool includeLayout = true);
} // namespace blobber::generation

#pragma once
#include "generation/caves.hpp"
#include "generation/population.hpp"

namespace blobber::generation {
Failure validateCavePopulation(const CaveFloor&);
Failure planCavePopulation(CaveFloor&);
PreparedPopulation prepareCavePopulation(const CaveFloor&, std::uint32_t seed, const rules::Catalog&,
                                         const std::vector<std::string>& types);
} // namespace blobber::generation

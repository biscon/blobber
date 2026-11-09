#pragma once
#include "world/blueprints.hpp"
#include <ostream>
namespace blobber::blueprints {
void writePlan(std::ostream&, const BlueprintCatalog&, const ReviewScene&, std::string_view title);
}

#pragma once
#include "game/movement.hpp"
#include "world/map.hpp"
#include <array>
namespace blobber {
struct CityDoorway {
    const char* name;
    Player approach;
};
struct SanctuaryCityReview {
    Map map;
    Player spawn;
    std::array<CityDoorway, 2> doors;
};
// Development-only authored scene; never substitutes the campaign hub.
SanctuaryCityReview sanctuaryCityReview();
} // namespace blobber

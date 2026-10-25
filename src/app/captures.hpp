#pragma once
#include "game/movement.hpp"
#include <string>
#include <vector>
namespace blobber {
struct Capture {
    std::string name;
    Player from, to;
    Action action;
    float t;
    std::uint64_t npcTimeMs = 0;
};
std::vector<Capture> makeCaptures();
} // namespace blobber

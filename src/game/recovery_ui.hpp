#pragma once
#include "game/recovery.hpp"
namespace blobber {
struct CampMember {
    const char* name = "";
    int hp = 0, maxHp = 0, mana = 0, maxMana = 0, available = 0, sides = 0, modifier = 0;
    bool living = true;
};
struct CampHud {
    bool open = false, playing = false;
    double elapsedMs = 0;
    RestRequest request;
    RestPreview preview;
    std::array<CampMember, 4> members{};
    const char* supplier = "";
    RestResult result;
};
struct LootPage {
    Cell cell{};
    unsigned page = 0;
    std::uint64_t revision = 0;
};
} // namespace blobber

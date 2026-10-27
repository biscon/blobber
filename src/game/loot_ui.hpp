#pragma once
#include "game/loot.hpp"
namespace blobber {
struct LootEntryHud {
    LootId id;
    const rules::ItemDefinition* definition = nullptr;
    unsigned quantity = 0;
};
struct LootHud {
    static constexpr unsigned visibleCount = 10;
    bool open = false;
    Cell cell{};
    std::uint64_t world = 0;
    std::array<LootId, lootCapacity> ids{};
    std::array<LootEntryHud, visibleCount> entries{};
    unsigned count = 0, page = 0;
    int hovered = -1;
    rules::CharacterId recipient{};
    const char* name = "";
    unsigned portrait = 0;
    std::int64_t weight = 0, capacity = 0;
    const char* reason = "";
    bool canTake = false, canTakeAny = false;
    std::array<char, 192> feedback{};
};
} // namespace blobber

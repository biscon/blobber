#pragma once
#include "game/actors.hpp"
#include "game/movement.hpp"
#include "rules/creation.hpp"
#include "world/npc.hpp"
namespace blobber {
struct StartingSnapshot {
    std::array<char, 129> id{};
    rules::CharacterDraft draft;
};
struct ArenaMonster {
    rules::MonsterId monster;
    unsigned quantity = 0;
};
struct ArenaScenario {
    std::array<char, 65> name{};
    std::array<StartingSnapshot, 4> party{}; // Formation slot order; independent copies.
    std::array<ArenaMonster, NpcWorld::maxTypes> roster{};
    std::size_t count = 0;
    std::uint32_t seed = 5;
    unsigned total() const noexcept;
    bool adjust(rules::MonsterId, int delta) noexcept;
};
ArenaScenario defaultArena(const rules::Catalog&);
const char* arenaError(const rules::Catalog&, const ArenaScenario&) noexcept;
rules::PresetParty arenaParty(const rules::Catalog&, const ArenaScenario&);
Map arenaMap();
Player arenaSpawn() noexcept;
NpcWorld arenaPlacements(const Map&, const rules::Catalog&, const ArenaScenario&,
                         const std::vector<std::string>& types);
bool arenaCleared(const ActorSnapshot&) noexcept;
} // namespace blobber

#pragma once
#include "game/actors.hpp"
#include "game/movement.hpp"
#include "game/party.hpp"
#include "game/starting_party.hpp"
#include "world/npc.hpp"
#include <memory>
namespace blobber {
struct ArenaMonster {
    rules::MonsterId monster;
    unsigned quantity = 0;
};
struct ArenaPreparation {
    unsigned targetLevel = 1;
    std::array<std::array<rules::AdvancementChoices, 4>, 2> advancements{};
};
struct ArenaScenario {
    std::array<char, 65> name{};
    std::array<StartingSnapshot, 4> party{}; // Formation slot order; independent copies.
    std::array<ArenaMonster, NpcWorld::maxTypes> roster{};
    std::size_t count = 0;
    std::uint32_t seed = 5;
    ArenaPreparation preparation;
    unsigned total() const noexcept;
    bool adjust(rules::MonsterId, int delta) noexcept;
};
ArenaScenario defaultArena(const rules::Catalog&);
const char* arenaError(const rules::Catalog&, const ArenaScenario&) noexcept;
rules::PresetParty arenaParty(const rules::Catalog&, const ArenaScenario&);
// Pure validation; preparation allocates only before simulation.
const char* arenaPreparationError(const rules::Catalog&, const StartingParty&,
                                  const ArenaPreparation&) noexcept;
std::unique_ptr<Party> prepareArenaParty(const rules::Catalog&, const ArenaScenario&);
Map arenaMap();
Player arenaSpawn() noexcept;
NpcWorld arenaPlacements(const Map&, const rules::Catalog&, const ArenaScenario&,
                         const std::vector<std::string>& types);
bool arenaCleared(const ActorSnapshot&) noexcept;
} // namespace blobber

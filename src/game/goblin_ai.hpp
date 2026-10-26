#pragma once
#include "game/movement.hpp"
#include "game/navigation.hpp"
namespace blobber {
enum class Awareness : std::uint8_t { Routine, Pursuing, Investigating, Searching, Returning };
struct GoblinAiState {
    Awareness awareness = Awareness::Routine;
    Cell home{}, lastSeen{};
    Heading homeFacing = Heading::North;
    NpcAiConfig config{};
    std::uint8_t waypoint = 0, searchTurns = 0;
    std::uint64_t lastSeenMs = 0, searchStartMs = 0, moveDueMs = 0;
};
struct GoblinAiSnapshot {
    std::array<GoblinAiState, NpcWorld::maxInstances> actors{};
    std::size_t count = 0, searchWork = 0, completedSearches = 0;
    std::uint64_t timeMs = 0;
};
bool goblinSees(const Map& map, Cell from, Heading facing, Cell target) noexcept;
class GoblinAiController {
  public:
    GoblinAiController(const Map& map, ActorWorld& actors, std::uint32_t seed);
    GoblinAiController(Map&&, ActorWorld&, std::uint32_t) = delete;
    GoblinAiController(const Map&&, ActorWorld&, std::uint32_t) = delete;
    GoblinAiController(const GoblinAiController&) = delete;
    GoblinAiController& operator=(const GoblinAiController&) = delete;
    void update(std::uint64_t elapsedMs, std::uint64_t sessionTime, Player party) noexcept;
    const GoblinAiSnapshot& snapshot() const noexcept { return state_; }

  private:
    const Map& map_;
    ActorWorld& actors_;
    Navigation navigation_;
    std::mt19937 rng_;
    GoblinAiSnapshot state_;
    std::uint64_t tickDue_ = 100;
    std::size_t priority_ = 0;
    struct Goal {
        bool active = false, adjacent = false;
        Cell target{};
    };
    Goal think(std::size_t slot, Player party) noexcept;
    bool step(std::size_t slot, Cell destination, Cell party, std::uint64_t now) noexcept;
    void face(std::size_t slot, Cell target) noexcept;
};
} // namespace blobber

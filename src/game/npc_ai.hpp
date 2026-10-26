#pragma once
#include "game/movement.hpp"
#include "game/navigation.hpp"
namespace blobber {
enum class Awareness : std::uint8_t { Routine, Pursuing, Investigating, Searching, Returning };
struct NpcAiState {
    Awareness awareness = Awareness::Routine;
    Cell home{}, lastSeen{};
    Heading homeFacing = Heading::North;
    NpcAiConfig config{};
    std::uint8_t waypoint = 0, searchTurns = 0;
    std::uint64_t lastSeenMs = 0, searchStartMs = 0, moveDueMs = 0;
};
struct NpcAiSnapshot {
    std::array<NpcAiState, NpcWorld::maxInstances> actors{};
    std::size_t count = 0, searchWork = 0, completedSearches = 0;
    std::uint64_t timeMs = 0;
};
bool npcSees(const Map& map, Cell from, Heading facing, Cell target) noexcept;
class NpcAiController {
  public:
    NpcAiController(const Map& map, ActorWorld& actors, std::uint32_t seed);
    NpcAiController(Map&&, ActorWorld&, std::uint32_t) = delete;
    NpcAiController(const Map&&, ActorWorld&, std::uint32_t) = delete;
    NpcAiController(const NpcAiController&) = delete;
    NpcAiController& operator=(const NpcAiController&) = delete;
    void update(std::uint64_t elapsedMs, std::uint64_t sessionTime, Player party,
                const std::array<bool, NpcWorld::maxInstances>* frozen = nullptr) noexcept;
    void alert(ActorId id, Cell party) noexcept;
    bool aware(ActorId id) const noexcept;
    bool approach(ActorId id, Player party, std::uint64_t now) noexcept;
    bool escape(ActorId id, Player party, std::uint64_t now) noexcept;
    const NpcAiSnapshot& snapshot() const noexcept { return state_; }

  private:
    const Map& map_;
    ActorWorld& actors_;
    Navigation navigation_;
    std::mt19937 rng_;
    NpcAiSnapshot state_;
    std::array<bool, NpcWorld::maxInstances> rangedOnly_{};
    std::uint64_t tickDue_ = 100;
    std::size_t priority_ = 0;
    struct Goal {
        bool active = false, adjacent = false;
        Cell target{};
        std::optional<Cell> preferred{};
    };
    Goal think(std::size_t slot, Player party) noexcept;
    std::optional<Cell> preferredFront(Cell from, Player party) const noexcept;
    bool step(std::size_t slot, Cell destination, Cell party, std::uint64_t now) noexcept;
    void face(std::size_t slot, Cell target) noexcept;
};
} // namespace blobber

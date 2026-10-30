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
bool npcSees(GeometryView map, Cell from, Heading facing, Cell target) noexcept;
class NpcAiController {
  public:
    NpcAiController(GeometryView map, ActorWorld& actors, std::uint32_t seed);
    NpcAiController(Map&&, ActorWorld&, std::uint32_t) = delete;
    NpcAiController(const Map&&, ActorWorld&, std::uint32_t) = delete;
    NpcAiController(const NpcAiController&) = delete;
    NpcAiController& operator=(const NpcAiController&) = delete;
    void update(std::uint64_t elapsedMs, std::uint64_t sessionTime, Player party,
                const std::array<bool, NpcWorld::maxInstances>* frozen = nullptr,
                const std::array<bool, NpcWorld::maxInstances>* incapacitated = nullptr) noexcept;
    void alert(ActorId id, Cell party) noexcept;
    void bindEffects(EffectController* e) noexcept { effects_ = e; }
    void turnMovement(bool on) noexcept { turnMovement_ = on; }
    void discardPreparation(ActorId id) noexcept {
        if (id.slot < prepared_.size())
            prepared_[id.slot].reset();
    }
    bool aware(ActorId id) const noexcept;
    bool approach(ActorId id, Player party, std::uint64_t now) noexcept;
    bool escape(ActorId id, Player party, std::uint64_t now) noexcept;
    const NpcAiSnapshot& snapshot() const noexcept { return state_; }

    // Preparation-only owned values; no borrowed pointers or navigation workspace.
    struct Saved {
        NpcAiSnapshot state;
        std::array<std::optional<Cell>, NpcWorld::maxInstances> prepared{};
        std::mt19937 rng;
        std::uint64_t tickDue = 100;
        std::size_t priority = 0;
    };

  private:
    friend class GameSession;
    GeometryView map_;
    ActorWorld& actors_;
    Navigation navigation_;
    EffectController* effects_ = nullptr;
    bool turnMovement_ = false;
    std::array<std::optional<Cell>, NpcWorld::maxInstances> prepared_{};
    std::mt19937 rng_;
    NpcAiSnapshot state_;
    std::array<bool, NpcWorld::maxInstances> rangedOnly_{};
    std::uint64_t tickDue_ = 100, geometryRevision_ = 0;
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

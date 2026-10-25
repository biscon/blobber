#pragma once
#include "game/combat.hpp"

namespace blobber {
// Map/catalog outlive this owner and every UI/animation borrower of its members.
class GameSession {
  public:
    GameSession(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements, std::uint32_t seed,
                Player spawn = {});
    GameSession(Map&&, const rules::Catalog&, const NpcWorld&, std::uint32_t, Player = {}) = delete;
    GameSession(const Map&&, const rules::Catalog&, const NpcWorld&, std::uint32_t, Player = {}) = delete;
    GameSession(const Map&, rules::Catalog&&, const NpcWorld&, std::uint32_t, Player = {}) = delete;
    GameSession(const Map&, const rules::Catalog&&, const NpcWorld&, std::uint32_t, Player = {}) = delete;
    GameSession(const GameSession&) = delete;
    GameSession& operator=(const GameSession&) = delete;
    Party& party() noexcept { return party_; }
    const Party& party() const noexcept { return party_; }
    ActorWorld& actors() noexcept { return actors_; }
    const ActorWorld& actors() const noexcept { return actors_; }
    CombatController& combat() noexcept { return combat_; }
    const CombatController& combat() const noexcept { return combat_; }
    const MovementState& movement() const noexcept { return movement_.state(); }
    std::uint64_t timeMs() const noexcept { return timeMs_; }
    CombatOutcome update(double seconds, bool paused) noexcept;
    void move(Action action, bool fresh = true) noexcept;
    void clearPending() noexcept { movement_.clearPending(); }
    void setAnimated(bool enabled) noexcept { movement_.setAnimated(enabled); }
    CombatOutcome submit(const PartyCommand& command) noexcept;

  private:
    const Map& map_;
    Party party_;
    ActorWorld actors_;
    Controller movement_;
    rules::DiceRng rng_;
    CombatController combat_;
    std::uint64_t timeMs_ = 0;
    double remainderMs_ = 0;
    void detectContact() noexcept;
};
} // namespace blobber

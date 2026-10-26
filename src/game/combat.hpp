#pragma once
#include "game/command_types.hpp"
#include "game/movement.hpp"
#include "game/npc_animation.hpp"

namespace blobber {
constexpr std::uint64_t combatFeedbackDurationMs = 1000;
enum class CombatPhase : std::uint8_t { Exploration, Contact, Turns, Victory, Defeat };
enum class CombatEvent : std::uint8_t { None, Started, Attack, Defend, Aid, DeathSave, Finished };
struct CombatOutcome {
    CombatEvent event = CombatEvent::None;
    CommandReason reason = CommandReason::Resolved;
    rules::CharacterId character{}, recipient{};
    ActorId enemy{};
    bool enemyAttack = false, hit = false, critical = false, died = false;
    std::int64_t damage = 0;
    int roll = 0;
    CombatPhase phase = CombatPhase::Exploration;
};
struct CombatParticipant {
    std::optional<rules::CharacterId> character;
    std::optional<ActorId> enemy;
    bool deathSave = false;
};
struct CombatSnapshot {
    CombatPhase phase = CombatPhase::Exploration;
    std::uint64_t world = 0, turn = 0, round = 0;
    std::optional<rules::CharacterId> activeCharacter;
    std::optional<ActorId> activeEnemy;
    bool awaitingAction = false, resolvingAction = false;
    std::array<bool, rules::partySize> defending{};
    std::array<ActorId, 4> enemies{};
    std::size_t enemyCount = 0;
    std::array<CombatParticipant, 8> upcoming{};
    std::size_t upcomingCount = 0;
};
// Borrows one session's immovable owners. No graphics, visibility oracle, or event queue.
class CombatController {
  public:
    static constexpr std::uint64_t actionDelayMs = 450;
    CombatController(Party& party, ActorWorld& actors, rules::DiceRng& rng,
                     const std::vector<NpcTimings>& timings = {});
    CombatController(const CombatController&) = delete;
    CombatController& operator=(const CombatController&) = delete;
    const CombatSnapshot& snapshot() const noexcept { return state_; }
    bool contact(Player pose) noexcept;
    CombatOutcome update(Player pose, bool moving, std::uint64_t now) noexcept;
    CommandReason validate(const PartyCommand& command, Player pose) const noexcept;
    CombatOutcome submit(const PartyCommand& command, Player pose, std::uint64_t now) noexcept;
    bool continueExploring() noexcept;
    void previewTurns(CombatSnapshot& destination) const noexcept;
    bool member(ActorId id) const noexcept;
    bool exposed(rules::CharacterId id) const noexcept;
    bool exposed(ActorId id) const noexcept;

  private:
    struct Entry {
        bool party = true;
        std::uint16_t index = 0;
        std::int64_t initiative = 0;
        int dexterity = 0;
    };
    Party& party_;
    ActorWorld& actors_;
    rules::DiceRng& rng_;
    CombatSnapshot state_;
    Player origin_;
    Cell cell_{};
    std::array<rules::AttackId, NpcWorld::maxInstances> melee_{};
    struct PresentationTiming {
        std::uint32_t attack = 0, impact = 0, hurt = 0, death = 0;
    };
    std::array<PresentationTiming, NpcWorld::maxTypes> presentation_{};
    std::array<rules::MonsterId, 4> groups_{};
    std::array<Entry, 8> order_{};
    std::size_t groupCount_ = 0, orderCount_ = 0, position_ = 0, enemyCursor_ = 0;
    std::uint64_t due_ = 0;
    bool near(const ActorState& actor) const noexcept;
    // Current front slot, or a free front slot reachable before this enemy's attack.
    std::optional<Cell> meleeSlot(ActorId id, std::uint8_t occupied) const noexcept;
    std::uint8_t occupiedSlots() const noexcept;
    bool finish() noexcept;
    void begin(std::uint64_t now) noexcept;
    void selectTurn() noexcept;
    void advance(std::uint64_t now) noexcept;
    void holdAttack(const CombatOutcome& outcome, std::uint64_t now) noexcept;
    const rules::WeaponPreview* weapon(rules::CharacterId id) const noexcept;
};
} // namespace blobber

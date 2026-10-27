#pragma once
#include "game/attack_timeline.hpp"
#include "game/npc_ai.hpp"
#include "game/npc_animation.hpp"
#include <cassert>
#include <cstdlib>

namespace blobber {
constexpr std::uint64_t combatFeedbackDurationMs = 1000;
constexpr std::uint64_t combatRoundMs = 6000;
enum class CombatPhase : std::uint8_t {
    Exploration,
    Contact,
    Turns,
    Victory,
    Defeat,
    Approach,
    Facing,
    RoundEnd
};
inline bool realTime(CombatPhase phase) noexcept {
    return phase == CombatPhase::Exploration || phase == CombatPhase::Approach;
}
enum class CombatEvent : std::uint8_t {
    None,
    Started,
    Attack, // Resolved impact; never command acceptance.
    AttackStarted,
    ProjectileReleased,
    AttackCanceled,
    Defend,
    Aid,
    DeathSave,
    Finished,
    Move,
    Round,
    Spent,
    Disengaged
};
struct CombatOutcome {
    CombatEvent event = CombatEvent::None;
    CommandReason reason = CommandReason::Resolved;
    rules::CharacterId character{}, recipient{};
    ActorId enemy{};
    bool enemyAttack = false, hit = false, critical = false, died = false;
    std::int64_t damage = 0;
    int roll = 0;
    CombatPhase phase = CombatPhase::Exploration;
    rules::AttackKind kind = rules::AttackKind::Melee;
    std::uint32_t impactMs = 0; // Strike/release offset in acceptance events.
    CombatAttackId attackId;
    std::uint64_t occurredMs = 0;
    bool targetGone = false;
};
struct CombatEvents {
    static constexpr std::size_t capacity = attackCapacity * 2 + 4;
    std::array<CombatOutcome, capacity> values{};
    std::size_t count = 0;
    CombatOutcome step; // The single automatic/phase step, separate from draining deadlines.
    void clear() noexcept {
        count = 0;
        step = {};
    }
    void append(const CombatOutcome& out) noexcept {
        if (out.event == CombatEvent::None)
            return;
        assert(count < capacity);
        if (count >= capacity)
            std::abort(); // Internal bound violation must never drop consequences.
        values[count++] = out;
    }
    const CombatOutcome* begin() const noexcept { return values.data(); }
    const CombatOutcome* end() const noexcept { return values.data() + count; }
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
    std::array<ActorId, NpcWorld::maxInstances> enemies{};
    std::size_t enemyCount = 0;
    std::array<CombatParticipant, 8> upcoming{};
    std::size_t upcomingCount = 0;
    std::array<std::uint64_t, rules::partySize> cooldownMs{};
    std::array<bool, rules::partySize> spent{};
    std::optional<Heading> facing;
    std::uint8_t knownDirections = 0;
    std::size_t pendingCount = 0;
    std::uint64_t gameplayMs = 0;
};
class CombatController {
  public:
    static constexpr std::uint64_t actionDelayMs = 450;
    CombatController(const Map&, Party&, ActorWorld&, rules::DiceRng&, NpcAiController&,
                     const std::vector<NpcTimings>& timings = {});
    CombatController(const CombatController&) = delete;
    CombatController& operator=(const CombatController&) = delete;
    const CombatSnapshot& snapshot() const noexcept { return state_; }
    const AttackVisualSnapshot& attacks() const noexcept { return visuals_; }
    void resolveImpacts(Player pose, std::uint64_t now, CombatEvents&) noexcept;
    bool contact(Player pose) noexcept;
    void elapse(std::uint64_t elapsed) noexcept;
    void observe(Player pose) noexcept;
    std::array<bool, NpcWorld::maxInstances> movementLocks(std::uint64_t now) const noexcept;
    CombatOutcome update(Player pose, bool moving, std::uint64_t now) noexcept;
    CommandReason validate(const PartyCommand&, Player) const noexcept;
    CombatOutcome submit(const PartyCommand&, Player, std::uint64_t now) noexcept;
    bool continueExploring() noexcept;
    void previewTurns(CombatSnapshot&) const noexcept;
    bool member(ActorId) const noexcept;
    bool exposed(rules::CharacterId) const noexcept;
    bool exposed(ActorId) const noexcept;
    bool adjacent(Cell) const noexcept;
    bool occupiedFront(Player) const noexcept;

  private:
    struct Entry {
        bool party = true;
        std::uint16_t index = 0;
        std::int64_t initiative = 0;
        int dexterity = 0;
    };
    struct Timing {
        std::uint32_t attack = 0, impact = 0, hurt = 0, death = 0, bow = 0, bowImpact = 0;
    };
    const Map& map_;
    Party& party_;
    ActorWorld& actors_;
    rules::DiceRng& rng_;
    NpcAiController& ai_;
    CombatSnapshot state_;
    struct PendingAttack {
        CombatOutcome result;
        std::uint64_t clipEndMs = 0, hurtUntilMs = 0;
        bool turn = false, releaseNotice = false;
    };
    std::array<PendingAttack, attackCapacity> pending_{};
    AttackVisualSnapshot visuals_;
    std::uint64_t nextAttack_ = 1;
    bool drainedThisUpdate_ = false;
    bool attackBusy(std::size_t slot) const noexcept;
    bool pendingImpacts() const noexcept;
    CombatOutcome queueAttack(CombatOutcome, Player, std::uint64_t now) noexcept;
    void setAttackHold(std::size_t slot, std::uint64_t until) noexcept;
    void interruptWindup(ActorId, std::uint64_t when) noexcept;
    void cancelAttack(std::size_t slot, std::uint64_t when, CombatEvents&, bool targetGone) noexcept;
    Player origin_;
    std::array<rules::AttackId, NpcWorld::maxInstances> melee_{}, ranged_{};
    std::array<Timing, NpcWorld::maxTypes> presentation_{};
    std::array<rules::MonsterId, rules::Catalog::maxMonsters> groups_{};
    std::array<Entry, rules::partySize + rules::Catalog::maxMonsters> order_{};
    std::array<bool, NpcWorld::maxInstances> members_{}, surprised_{}, closeNext_{}, known_{},
        episodeActors_{};
    std::array<std::uint64_t, NpcWorld::maxInstances> knownUntil_{}, enemyReady_{}, enemyBusy_{};
    std::array<std::uint64_t, rules::partySize> partyReady_{}, saveDue_{}, defendDue_{};
    std::size_t groupCount_ = 0, orderCount_ = 0, position_ = 0, enemyCursor_ = 0, autoCursor_ = 0;
    std::uint64_t due_ = 0, partyBusy_ = 0;
    bool episode_ = false, enableRoaming_ = true, resumeAwaiting_ = false;
    CombatPhase resumePhase_ = CombatPhase::Contact;
    bool partyExposed(rules::CharacterId) const noexcept;
    bool frontal(ActorId) const noexcept;
    bool settleFront(std::uint64_t now) noexcept;
    bool presentationBusy(std::uint64_t now) const noexcept;
    void approach() noexcept;
    bool near(const ActorState&, Heading approach) const noexcept;
    std::optional<Cell> meleeSlot(ActorId, std::uint8_t occupied) const noexcept;
    bool finish(std::uint64_t now) noexcept;
    void begin(std::uint64_t now) noexcept;
    void enroll(Player, bool opening) noexcept;
    void addGroups() noexcept;
    void selectTurn() noexcept;
    void advance(std::uint64_t now) noexcept;
    void alert(ActorId, Player) noexcept;
    void refreshReadiness() noexcept;
    void leave() noexcept;
    const rules::WeaponPreview* weapon(rules::CharacterId, bool ranged = false) const noexcept;
    CombatOutcome enemyAttack(ActorId, bool ranged, std::uint64_t now, bool opening = false) noexcept;
    CombatOutcome enemyTurn(ActorId, std::uint64_t now) noexcept;
    CombatOutcome deathSave(rules::CharacterId, std::uint64_t now) noexcept;

  public:
    void enableRoaming(bool enabled) noexcept { enableRoaming_ = enabled; }
};
} // namespace blobber

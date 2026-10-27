#pragma once
#include "game/actors.hpp"
#include "game/party.hpp"

namespace blobber {
struct CombatEvents;
enum class EffectKind : std::uint8_t { Bless, Bane, ShieldOfFaith, MageArmor, HoldPerson, Count };
struct EffectId {
    std::uint64_t world = 0, serial = 0;
};
struct EffectTarget {
    bool npc = false;
    rules::CharacterId character{};
    ActorId enemy{};
    EffectTarget() = default;
    EffectTarget(rules::CharacterId id) : character(id) {}
    EffectTarget(ActorId id) : npc(true), enemy(id) {}
};
bool operator==(EffectTarget a, EffectTarget b) noexcept;
struct EffectRequest {
    std::uint64_t world = 0;
    rules::CharacterId caster{};
    EffectKind kind = EffectKind::Bless;
    std::array<EffectTarget, 3> targets{};
    std::size_t count = 0;
    int difficulty = 10;
};
enum class EffectError : std::uint8_t {
    None,
    InvalidWorld,
    InvalidCaster,
    InvalidKind,
    InvalidTargets,
    InvalidDifficulty,
    Capacity,
    InvalidHandle
};
struct EffectResult {
    EffectError error = EffectError::None;
    EffectId id{}; // Empty when every initial save succeeds.
    explicit operator bool() const noexcept { return error == EffectError::None; }
};
enum class EffectNotice : std::uint8_t {
    Applied,
    InitialSave,
    RepeatSave,
    ConcentrationSave,
    RecipientRemoved,
    Ended
};
enum class EffectEnd : std::uint8_t {
    None,
    Expired,
    Replaced,
    Dismissed,
    Incapacitated,
    FailedSave,
    Saved,
    NoTargets,
    InvalidTarget,
    Armor
};
struct EffectEvent {
    EffectNotice notice = EffectNotice::Applied;
    EffectEnd reason = EffectEnd::None;
    EffectId id{};
    EffectKind kind = EffectKind::Bless;
    rules::CharacterId caster{};
    EffectTarget target{};
    std::uint64_t gameplayMs = 0;
    int roll = 0;
    std::int64_t total = 0, difficulty = 0;
    bool succeeded = false;
};
struct EffectInstance {
    EffectId id{};
    EffectKind kind = EffectKind::Bless;
    rules::CharacterId caster{};
    std::array<EffectTarget, 3> targets{};
    std::size_t count = 0;
    int difficulty = 10;
    std::uint64_t expiresMs = 0, retryMs = 0;
    bool active = false;
};
// Fixed concrete effects, not a spell executor. Owners/RNG outlive this object.
class EffectController {
  public:
    static constexpr std::size_t capacity = 64, eventAllowance = 512;
    static constexpr std::uint64_t retryIntervalMs = 6000;
    EffectController(Party&, ActorWorld&, rules::DiceRng&) noexcept;
    EffectController(const EffectController&) = delete;
    EffectController& operator=(const EffectController&) = delete;
    const std::array<EffectInstance, capacity>& snapshot() const noexcept { return instances_; }
    const EffectInstance* find(EffectId) const noexcept;
    EffectId concentration(rules::CharacterId) const noexcept;
    EffectError validate(const EffectRequest&) const noexcept;
    bool guiding(ActorId) const noexcept;
    void guide(ActorId, std::uint64_t now, CombatEvents&) noexcept;
    void consumeGuiding(ActorId, std::uint64_t now, CombatEvents&) noexcept;
    EffectResult apply(const EffectRequest&, std::uint64_t sessionMs, CombatEvents&) noexcept;
    EffectResult remove(EffectId, std::uint64_t sessionMs, CombatEvents&) noexcept;
    void reconcile(std::uint64_t sessionMs, CombatEvents&) noexcept;
    void advance(std::uint64_t gameplayMs, std::uint64_t sessionMs, bool realTime, CombatEvents&) noexcept;
    void damaged(rules::CharacterId, std::int64_t damage, std::uint64_t sessionMs, CombatEvents&) noexcept;
    bool has(EffectTarget, EffectKind) const noexcept;
    bool canAct(EffectTarget) const noexcept;
    bool mobileParty() const noexcept;
    std::array<bool, NpcWorld::maxInstances> incapacitatedActors() const noexcept;
    int armorClass(EffectTarget) const noexcept;
    int rollModifier(EffectTarget) noexcept;
    rules::SaveResult save(EffectTarget, rules::Ability, std::int64_t difficulty) noexcept;
    std::uint64_t gameplayMs() const noexcept { return gameplayMs_; }

  private:
    Party& party_;
    ActorWorld& actors_;
    rules::DiceRng& rng_;
    std::array<EffectInstance, capacity> instances_{};
    std::array<std::uint64_t, NpcWorld::maxInstances> guidingUntil_{};
    std::uint64_t nextSerial_ = 1, gameplayMs_ = 0;
    bool living(EffectTarget) const noexcept;
    bool unarmored(rules::CharacterId) const noexcept;
    void end(EffectInstance&, EffectEnd, std::uint64_t sessionMs, CombatEvents&) noexcept;
    void emit(const EffectInstance&, EffectNotice, EffectEnd, EffectTarget, const rules::SaveResult&,
              std::int64_t dc, std::uint64_t sessionMs, CombatEvents&) const noexcept;
};
} // namespace blobber

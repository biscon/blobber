#pragma once
#include "game/actors.hpp"
#include "game/party.hpp"
#include "rules/spells.hpp"
#include <optional>

namespace blobber {
struct CombatEvents;
enum class EffectKind : std::uint8_t {
    Bless,
    Bane,
    ShieldOfFaith,
    MageArmor,
    HoldPerson,
    Guidance,
    Resistance,
    Sanctuary,
    Sleep,
    Blind,
    Slow,
    Knockdown,
    Aid,
    MirrorImage,
    Web,
    Restrained,
    SpiritualWeapon,
    Count
};
enum class StatusClock : std::uint8_t { Seconds, Turns, CasterTurn, Standing, Escape };
struct StatusEntry {
    EffectKind effect = EffectKind::Count;
    rules::SpellKind spell = rules::SpellKind::Count;
    const char* name = "";
    const char* ending = "";
    std::uint64_t remainingMs = 0;
    StatusClock clock = StatusClock::Seconds;
    bool harmful = false;
    unsigned stacks = 0;
};
struct StatusSummary {
    static constexpr std::size_t capacity = static_cast<std::size_t>(EffectKind::Count) + 2;
    std::array<StatusEntry, capacity> entries{};
    std::size_t count = 0;
};
rules::SpellKind effectSpell(EffectKind) noexcept;
bool harmfulEffect(EffectKind) noexcept;
void statusDuration(const StatusEntry&, char* output, std::size_t bytes) noexcept;
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
    Armor,
    Consumed,
    Woken,
    Recovered
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
    std::uint8_t duplicates = 0, reserved = 0;
    Cell cell{};
    std::uint64_t readyMs = 0;
};
struct ConditionState {
    rules::CharacterId caster{};
    std::uint64_t untilMs = 0, remainingMs = 0, casterTurn = 0;
    bool active = false, turnBound = false;
};
struct AreaId {
    std::uint64_t world = 0, serial = 0;
};
struct GroundArea {
    AreaId id{};
    Cell cell{};
    rules::CharacterId caster{};
    int difficulty = 0;
    std::uint64_t untilMs = 0;
    bool active = false;
    bool web = false;
    EffectId effect{};
    std::uint64_t burnMs = 0;
};
struct MirrorReservation {
    EffectId effect;
    std::uint8_t duplicate = 0;
    bool hit = false;
};
struct Restraint {
    AreaId area;
    Cell cell{};
    int difficulty = 0;
    rules::CharacterId caster;
};
struct SpectralWeapon {
    Cell cell{};
    bool active = false;
};
struct GroundSnapshot {
    std::array<SpectralWeapon, 4> weapons{};
    std::array<GroundArea, 16> areas{};
    std::uint64_t revision = 0;
};
const char* effectName(EffectKind) noexcept;
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
    StatusSummary statuses(EffectTarget) const noexcept;
    std::optional<StatusEntry> concentrationStatus(rules::CharacterId) const noexcept;
    bool canAct(EffectTarget) const noexcept;
    bool mobileParty() const noexcept;
    bool conscious(EffectTarget) const noexcept;
    bool sees(EffectTarget) const noexcept;
    bool sleepImmune(EffectTarget) const noexcept;
    bool metalArmor(EffectTarget) const noexcept;
    bool helpless(EffectTarget) const noexcept;
    rules::RollMode attackMode(EffectTarget attacker, EffectTarget target, bool close, bool advantage = false,
                               bool disadvantage = false) const noexcept;
    int oneShot(EffectTarget, EffectKind) noexcept;
    int saveModifier(EffectTarget t) noexcept { return rollModifier(t) + oneShot(t, EffectKind::Resistance); }
    void breakWard(EffectTarget, std::uint64_t, CombatEvents&) noexcept;
    bool wardAllows(EffectTarget attacker, EffectTarget target, std::uint64_t, CombatEvents&) noexcept;
    void condition(EffectTarget, EffectKind, rules::CharacterId caster, std::uint64_t,
                   CombatEvents&) noexcept;
    void wake(EffectTarget, std::uint64_t, CombatEvents&) noexcept;
    bool stand(EffectTarget, std::uint64_t, CombatEvents&) noexcept;
    void casterTurn(rules::CharacterId, std::uint64_t, CombatEvents&) noexcept;
    void clockMode(bool turns) noexcept;
    void bindEvents(CombatEvents& events, std::uint64_t now) noexcept {
        events_ = &events;
        noticeMs_ = now;
    }
    const GroundSnapshot& ground() const noexcept { return ground_; }
    bool canCreateArea() const noexcept;
    bool canCreateWeb(const Cell*, std::size_t, rules::CharacterId) const noexcept;
    bool createWeb(const Cell*, std::size_t, rules::CharacterId, int, std::uint64_t, CombatEvents&) noexcept;
    void ignite(Cell) noexcept;
    bool escapeWeb(EffectTarget, std::uint64_t, CombatEvents&) noexcept;
    bool mobile(EffectTarget t) const noexcept { return canAct(t) && !has(t, EffectKind::Restrained); }
    MirrorReservation mirror(EffectTarget attacker, rules::CharacterId target, int attackRoll,
                             int bonus) noexcept;
    void resolveMirror(MirrorReservation, bool canceled, std::uint64_t, CombatEvents&) noexcept;
    bool restoreCondition(rules::CharacterId, EffectKind, std::uint64_t, CombatEvents&) noexcept;
    const EffectInstance* spiritualWeapon(rules::CharacterId) const noexcept;
    bool moveWeapon(rules::CharacterId, Cell, bool initial = false) noexcept;
    bool weaponCanMove(rules::CharacterId, Cell) const noexcept;
    AreaId createArea(Cell, rules::CharacterId, int difficulty, std::uint64_t, CombatEvents&) noexcept;
    const GroundArea* area(Cell) const noexcept;
    unsigned movementFactor(EffectTarget, Cell from, Cell to) const noexcept;
    void enter(EffectTarget, Cell, std::uint64_t) noexcept;
    void groundCheck(EffectTarget, Cell, std::uint64_t, CombatEvents&) noexcept;
    void groundOccupants(Cell partyCell, std::uint64_t, CombatEvents&) noexcept;
    void partyCell(Cell cell) noexcept { partyCell_ = cell; }
    void skipHazards(std::uint64_t until) noexcept { groundDueMs_ = until + 6000; }

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
    static constexpr std::size_t creatureCount = NpcWorld::maxInstances + rules::partySize;
    std::array<std::array<ConditionState, 4>, creatureCount> conditions_{};
    std::array<std::uint64_t, 4> casterTurns_{};
    GroundSnapshot ground_{};
    std::array<Restraint, creatureCount> restrained_{};
    void reconcileAid() noexcept;
    void reconcileWeb(std::uint64_t, CombatEvents&) noexcept;
    void burnWeb(std::uint64_t, CombatEvents&) noexcept;
    std::uint64_t nextArea_ = 1, groundDueMs_ = 6000, noticeMs_ = 0;
    Cell partyCell_{};
    bool turns_ = false;
    CombatEvents* events_ = nullptr; // Borrowed only from the session's permanent event owner.
    std::size_t conditionIndex(EffectTarget) const noexcept;
    void conditionNotice(EffectTarget, EffectKind, EffectNotice, EffectEnd, rules::CharacterId, std::uint64_t,
                         CombatEvents&) const noexcept;
    std::uint64_t conditionDeadline(bool realTime) const noexcept;
    void advanceConditions(std::uint64_t goal, std::uint64_t session, bool realTime, CombatEvents&) noexcept;

    bool living(EffectTarget) const noexcept;
    bool unarmored(rules::CharacterId) const noexcept;
    void end(EffectInstance&, EffectEnd, std::uint64_t sessionMs, CombatEvents&) noexcept;
    void emit(const EffectInstance&, EffectNotice, EffectEnd, EffectTarget, const rules::SaveResult&,
              std::int64_t dc, std::uint64_t sessionMs, CombatEvents&) const noexcept;
};
} // namespace blobber

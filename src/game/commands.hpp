#pragma once
#include "game/npc_animation.hpp"
#include "game/session.hpp"
#include <optional>

namespace blobber {
class CombatLog {
  public:
    static constexpr std::size_t capacity = 32, messageBytes = 192;
    void append(const char* message) noexcept;
    const char* newest(std::size_t offset) const noexcept;
    std::size_t size() const noexcept { return count_; }
    std::size_t lineCount(int columns) const noexcept;
    bool line(std::size_t fromOldest, int columns, char* output, std::size_t bytes) const noexcept;

  private:
    std::array<std::array<char, messageBytes>, capacity> entries_{};
    std::size_t next_ = 0, count_ = 0;
};
enum class DisplayStatus : std::uint8_t { Ready, Unconscious, Dead, Stable };
struct DemoResources {
    int hp = 0, mana = 0, maxMana = 0;
    DisplayStatus status = DisplayStatus::Ready;
};
// Explicit diagnostic data, never an authoritative Party snapshot.
struct HudDemo {
    bool enabled = false;
    int scenario = 0;
    std::array<DemoResources, rules::partySize> resources{};
};
struct HudCard {
    rules::CharacterId id;
    const char *name = "", *className = "";
    int hp = 0, maxHp = 0, mana = 0, maxMana = 0;
    bool hasMana = false, defending = false, active = false, recentlyHit = false, hasDeathSaves = false;
    std::uint8_t successes = 0, failures = 0;
    DisplayStatus status = DisplayStatus::Ready;
    const char* raceName = "";
    std::array<const char*, 3> equipment{{"None", "None", "None"}};
    rules::Abilities abilities{};
    int armorClass = 0, level = 1;
    unsigned portrait = 11;
};
enum class HoverKind { None, Enemy, Action, Portrait };
struct HudHover {
    HoverKind kind = HoverKind::None;
    int index = 0;
    std::optional<ActorId> enemy;
    float x = 0, y = 0, health = 0;
    CommandReason reason = CommandReason::Unavailable;
    std::array<std::array<char, 96>, 4> lines{};
};
struct CombatFeedback {
    static constexpr std::uint64_t durationMs = combatFeedbackDurationMs;
    bool active = false, party = false, hit = false;
    rules::CharacterId character;
    ActorId enemy;
    TargetAnchor anchor;
    std::int64_t damage = 0;
    std::uint64_t startedMs = 0;
};
struct HudSnapshot {
    std::array<HudCard, rules::partySize> cards{};
    PartyCommand selection;
    TargetSnapshot targets;
    std::array<bool, NpcWorld::maxInstances> protectedTargets{};
    std::size_t targetPage = 0, logOffset = 0;
    CommandReason availability = CommandReason::MissingTarget;
    std::array<CommandReason, static_cast<std::size_t>(PartyAction::Count)> actionAvailability{};
    bool suspended = false, demo = false, logExpanded = false;
    int demoScenario = 0;
    CombatLog log;
    bool production = false;
    CombatSnapshot combat;
    std::array<rules::CharacterId, rules::partySize> allies{};
    std::size_t allyCount = 0;
    std::array<CombatFeedback, 8> feedback{};
    HudHover hover;
    std::optional<rules::CharacterId> inspected;
    bool menuOpen = false, quitRequested = false;
    bool modal() const noexcept { return logExpanded || inspected.has_value() || menuOpen; }
};
inline constexpr std::array<PartyAction, 6> dockActions{{PartyAction::Melee, PartyAction::Defend,
                                                         PartyAction::Ranged, PartyAction::Spell,
                                                         PartyAction::Item, PartyAction::Aid}};

// Owners outlive this controller. The session overload routes checked combat requests;
// the const-owner overload is isolated preview-only presentation.
class CommandController {
  public:
    CommandController(const Party& party, const ActorWorld& actors);
    explicit CommandController(GameSession& session, const std::vector<NpcTimings>& timings = {});
    CommandController(GameSession&&) = delete;
    void record(const CombatOutcome& outcome) noexcept;
    void selectAlly(std::size_t index) noexcept;
    CommandController(Party&&, const ActorWorld&) = delete;
    CommandController(const Party&&, const ActorWorld&) = delete;
    CommandController(const Party&, ActorWorld&&) = delete;
    CommandController(const Party&, const ActorWorld&&) = delete;
    CommandController(const CommandController&) = delete;
    CommandController& operator=(const CommandController&) = delete;
    void refresh(const TargetSnapshot& targets, bool moving, bool mapOpen, bool focused,
                 const HudDemo* demo = nullptr) noexcept;
    void selectCharacter(std::size_t formationIndex) noexcept;
    void selectAction(PartyAction action) noexcept;
    void cycleAction(int delta) noexcept;
    void selectTarget(ActorId target) noexcept;
    void cycleTarget(int delta) noexcept;
    void pageTargets(int delta) noexcept;
    void scrollLog(int delta) noexcept;
    void toggleLog() noexcept;
    void latestLog() noexcept { state_.logOffset = 0; }
    void inspect(std::size_t formationIndex) noexcept;
    void toggleMenu() noexcept;
    void closeOverlay() noexcept;
    void requestQuit() noexcept { state_.quitRequested = true; }
    bool pointerMoved(float x, float y) noexcept {
        const bool moved = !pointerKnown_ || x != pointerX_ || y != pointerY_;
        pointerKnown_ = true;
        pointerX_ = x;
        pointerY_ = y;
        return moved;
    }
    void hover(HoverKind kind, int index = 0, std::optional<ActorId> enemy = {}, float x = 0,
               float y = 0) noexcept;
    void cancel() noexcept;
    void movementStarted() noexcept;
    CommandReason validate(const PartyCommand& command) const noexcept;
    CommandReason submit(const PartyCommand& command) noexcept;
    CommandReason confirm() noexcept { return submit(state_.selection); }
    const HudSnapshot& snapshot() const noexcept { return state_; }

  private:
    bool visible(ActorId id) const noexcept;
    void updateAvailability() noexcept;
    void recordFeedback(const CombatOutcome& outcome) noexcept;
    void showFeedback(const CombatFeedback& feedback) noexcept;
    void refreshFeedback(bool clear) noexcept;
    void updateHover() noexcept;
    void appendLog(const char* message) noexcept;
    bool pointerKnown_ = false;
    float pointerX_ = 0, pointerY_ = 0;
    GameSession* session_ = nullptr;
    std::array<std::uint64_t, rules::partySize> hitUntil_{};
    std::array<std::uint32_t, NpcWorld::maxTypes> attackImpactMs_{};
    std::array<CombatFeedback, 8> pendingFeedback_{};
    const Party& party_;
    const ActorWorld& actors_;
    HudSnapshot state_;
    CommandReason suspension_ = CommandReason::PreviewAccepted;
};
} // namespace blobber

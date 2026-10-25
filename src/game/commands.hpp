#pragma once
#include "game/actors.hpp"
#include "game/party.hpp"
#include <optional>

namespace blobber {
inline bool sameActor(ActorId a, ActorId b) noexcept {
    return a.slot == b.slot && a.world == b.world;
}
enum class PartyAction : std::uint8_t { Melee, BackMelee, Defend, Ranged, Spell, Item, Count };
enum class CommandReason : std::uint8_t {
    PreviewAccepted,
    InvalidCharacter,
    InvalidAction,
    Unavailable,
    BackRowOnly,
    MissingTarget,
    InvalidTarget,
    NotVisible,
    Moving,
    MapOpen,
    Unfocused,
    CannotAct
};
const char* actionName(PartyAction action) noexcept;
const char* commandMessage(CommandReason reason) noexcept;
struct PartyCommand {
    rules::CharacterId character;
    PartyAction action = PartyAction::Melee;
    std::optional<ActorId> target;
};
// Presentation eligibility only; future combat must validate its own range/LOS/turn rules.
struct TargetSnapshot {
    std::array<ActorId, NpcWorld::maxInstances> ids{};
    std::size_t count = 0;
    std::uint64_t world = 0, revision = 0;
};
class CombatLog {
  public:
    static constexpr std::size_t capacity = 32, messageBytes = 192;
    void append(const char* message) noexcept;
    const char* newest(std::size_t offset) const noexcept;
    std::size_t size() const noexcept { return count_; }

  private:
    std::array<std::array<char, messageBytes>, capacity> entries_{};
    std::size_t next_ = 0, count_ = 0;
};
enum class DisplayStatus : std::uint8_t { Ready, Unconscious, Dead };
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
    bool hasMana = false;
    DisplayStatus status = DisplayStatus::Ready;
};
struct HudSnapshot {
    std::array<HudCard, rules::partySize> cards{};
    PartyCommand selection;
    TargetSnapshot targets;
    std::size_t targetPage = 0, logOffset = 0;
    CommandReason availability = CommandReason::MissingTarget;
    std::array<CommandReason, static_cast<std::size_t>(PartyAction::Count)> actionAvailability{};
    bool suspended = false, demo = false, logExpanded = false;
    int demoScenario = 0;
    CombatLog log;
};
// Party/catalog/actors outlive this controller. No authoritative resource mutation.
class CommandController {
  public:
    CommandController(const Party& party, const ActorWorld& actors);
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
    void toggleLog() noexcept { state_.logExpanded = !state_.logExpanded; }
    void cancel() noexcept;
    void movementStarted() noexcept;
    CommandReason validate(const PartyCommand& command) const noexcept;
    CommandReason submit(const PartyCommand& command) noexcept;
    CommandReason confirm() noexcept { return submit(state_.selection); }
    const HudSnapshot& snapshot() const noexcept { return state_; }

  private:
    bool visible(ActorId id) const noexcept;
    void updateAvailability() noexcept;
    const Party& party_;
    const ActorWorld& actors_;
    HudSnapshot state_;
    CommandReason suspension_ = CommandReason::PreviewAccepted;
};
} // namespace blobber

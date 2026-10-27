#pragma once
#include "game/inventory_ui.hpp"
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
    std::size_t rewrapOffset(std::size_t offset, int from, int to) const noexcept;
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
    std::array<char, 96> effects{};
    bool concentrating = false;
    bool hasMana = false, defending = false, active = false, recentlyHit = false, hasDeathSaves = false;
    bool incomingShot = false;
    std::uint8_t successes = 0, failures = 0;
    DisplayStatus status = DisplayStatus::Ready;
    const char* raceName = "";
    std::array<const char*, 4> equipment{{"None", "None", "None", "None"}};
    rules::Abilities abilities{};
    int armorClass = 0, level = 1;
    std::uint64_t cooldownMs = 0;
    bool spent = false, selecting = false;
    unsigned portrait = 100;
    bool castRecipient = false, castEligible = true, castCaster = false, castHovered = false;
    const char* castPreview = "";
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
    bool active = false, party = false, hit = false, healing = false;
    rules::CharacterId character;
    ActorId enemy;
    const char* text = nullptr;
    TargetAnchor anchor;
    std::int64_t damage = 0;
    std::uint64_t startedMs = 0;
};
enum class PickerEntryKind : std::uint8_t { Spell, ActivatedFeature, AutomaticFeature };
enum class PassiveFeature : std::uint8_t { None, Defense, SneakAttack, RearRowMelee };
struct SpellBookEntry {
    rules::SpellId spell;
    rules::Feature feature = rules::Feature::None;
    const char *name = "", *description = "";
    int cost = 0;
    rules::SpellKind kind = rules::SpellKind::Count;
    CommandReason reason = CommandReason::Unavailable;
    int charges = -1; // Negative for entries without a finite-use resource.
    PickerEntryKind category = PickerEntryKind::Spell;
    PassiveFeature passive = PassiveFeature::None;
};
enum class CastingState : std::uint8_t { Normal, SpellPicker, Targeting, Resolving };
struct MagicHud {
    CastingState state = CastingState::Normal;
    PartyCommand pending;
    rules::SpellKind kind = rules::SpellKind::Count;
    std::array<CommandReason, rules::partySize> allyReasons{};
    std::array<bool, rules::partySize> selectedAllies{};
    SpellArea footprint;
    std::array<char, 128> feedback{};
    unsigned assigned = 0, maximum = 0;
    bool single = false;
    bool casting() const noexcept {
        return state == CastingState::Targeting || state == CastingState::Resolving;
    }
    bool open = false, armed = false, hostile = false, area = false, missiles = false;
    // 16 class spells + 3 activated features + 3 automatic features.
    static constexpr std::size_t capacity = 22;
    std::array<SpellBookEntry, capacity> entries{};
    std::size_t count = 0, cursor = 0, offset = 0;
    const char *name = "", *description = "";
    int mana = 0, maxMana = 0, cost = 0, attackBonus = 0, difficulty = 0;
    int charges = -1;
    bool spellcaster = false;
    std::array<ActorId, 12> affected{};
    std::size_t affectedCount = 0;
    std::array<bool, 3> cells{};
    std::array<char, 192> targets{};
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
    std::array<CombatFeedback, 16> feedback{};
    MagicHud magic;
    InventoryHud inventory;
    HudHover hover;
    std::optional<rules::CharacterId> inspected;
    bool menuOpen = false, quitRequested = false;
    bool modal() const noexcept {
        return inventory.open || logExpanded || inspected.has_value() || menuOpen || magic.open;
    }
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
    void openInventory(std::optional<rules::CharacterId> = {}) noexcept;
    void inventoryCharacter(rules::CharacterId) noexcept;
    void inventoryCursor(std::uint16_t cell) noexcept {
        if (cell < 52)
            state_.inventory.cursor = cell;
    }
    void inventorySelect(std::uint16_t) noexcept;
    void inventoryAction(InventoryAction) noexcept;
    void inventoryDrop(ItemLocation, bool split = false) noexcept;
    void inventoryQuantity(int) noexcept;
    void inventoryDescription(int) noexcept;
    void inventoryConfirm() noexcept;
    void inventoryCancel() noexcept;
    void inventoryPointer(float, float, bool press, bool held, bool release, bool shift) noexcept;
    void inventoryAlly(std::size_t, bool commit) noexcept;
    void inventoryBack() noexcept;
    void record(const CombatOutcome& outcome) noexcept;
    void record(const CombatEvents& events) noexcept {
        for (const auto& out : events)
            record(out);
    }
    void selectAlly(std::size_t index) noexcept;
    void openSpellbook() noexcept;
    void browseSpellbook(int delta) noexcept;
    void chooseSpellbook(std::size_t index) noexcept;
    SpellArea floorPreview() const noexcept;
    void backToSpellbook() noexcept;
    void magicEnemy(ActorId, bool remove = false) noexcept;
    void removeMagicRecipient(std::size_t index) noexcept;
    CommandReason magicEligibility(ActorId) const noexcept;
    void scrollSpellbook(int delta) noexcept;
    void magicAlly(std::size_t formationIndex) noexcept;
    bool magicSingleTarget() const noexcept;
    void magicKeyboardTarget(int delta) noexcept;
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
    CommandReason confirm() noexcept;
    const HudSnapshot& snapshot() const noexcept { return state_; }

  private:
    void refreshInventory() noexcept;
    void refreshInventoryActions() noexcept;
    PartyCommand inventoryUseCommand() const noexcept;
    void inventoryFeedback(const char*) noexcept;
    void inventoryExecute(const InventoryRequest&) noexcept;
    void refreshMagic() noexcept;
    void reconcileMagic() noexcept;
    void magicFeedback(const char*) noexcept;
    void setCastingState(CastingState) noexcept;
    std::array<std::size_t, rules::partySize> bookCursors_{}, bookOffsets_{};
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
    const Party& party_;
    const ActorWorld& actors_;
    HudSnapshot state_;
    CommandReason suspension_ = CommandReason::PreviewAccepted;
};
} // namespace blobber

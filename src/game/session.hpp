#pragma once
#include "game/combat.hpp"
#include "game/floor_objective.hpp"
#include "game/loot.hpp"
#include "game/npc_ai.hpp"
#include <memory>

namespace blobber {
// Map/catalog outlive this owner and every UI/animation borrower of its members.
class CampaignRun;
// Save schema 1 keeps the upper half free for checked deadline arithmetic.
inline constexpr std::uint64_t maxCheckpointClock = UINT64_MAX / 2;
struct FloorState {
    FloorGeometry::Saved fixtures;
    ActorSnapshot actors;
    LootWorld::Saved loot;
    NpcAiController::Saved ai;
    EffectController::Saved effects;
    CombatController::Saved combat;
    FloorObjectiveSnapshot objective;
    std::array<bool, NpcWorld::maxInstances> creditedDeaths{};
    std::uint64_t rewardRevision = UINT64_MAX, timeMs = 0, restToken = 1;
    double remainderMs = 0;
    rules::DiceRng rng{0};
    std::uint64_t digest() const; // Diagnostic only: canonical fields, excludes runtime world identities.
};
class GameSession {
  public:
    // Staged borrowed-party construction does not observe/reconcile/mutate Party.
    GameSession(const Map&, Party&, const NpcWorld&, std::uint32_t seed, Player,
                const std::vector<NpcTimings>&, const RecoveryLevel&, const FloorFixtures&,
                const FloorObjectiveDefinition&);
    void exportFloor(FloorState&) const noexcept;
    void validateCheckpoint(const FloorState&) const;
    void importFloor(const FloorState&); // Preparation only; rejects invalid values.
    CampaignRun* campaign() const noexcept { return campaign_; }
    void restorePose(Player, bool animated); // Preparation only.
    const char* saveSafety() const noexcept;
    Navigation::Saved navigationCheckpoint() const { return ai_.navigation_.checkpoint(); }
    void restoreNavigation(const Navigation::Saved& s) { ai_.navigation_.restore(s); }
    const char* travelSafety() const noexcept;
    bool runActive() const noexcept;
    GameSession(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements, std::uint32_t seed,
                Player spawn = {}, bool enableAi = true, const std::vector<NpcTimings>& timings = {},
                const RecoveryLevel& recovery = {}, const FloorFixtures& fixtures = {},
                const FloorObjectiveDefinition& objective = {});
    GameSession(const Map&, const rules::Catalog&, const rules::PresetParty&, const NpcWorld&,
                std::uint32_t seed, Player spawn = {}, bool enableAi = true,
                const std::vector<NpcTimings>& timings = {}, const RecoveryLevel& recovery = {},
                const FloorFixtures& fixtures = {}, const FloorObjectiveDefinition& objective = {});
    GameSession(Map&&, const rules::Catalog&, const NpcWorld&, std::uint32_t, Player = {}, bool = true,
                const std::vector<NpcTimings>& = {}, const RecoveryLevel& = {}, const FloorFixtures& = {},
                const FloorObjectiveDefinition& = {}) = delete;
    GameSession(const Map&&, const rules::Catalog&, const NpcWorld&, std::uint32_t, Player = {}, bool = true,
                const std::vector<NpcTimings>& = {}, const RecoveryLevel& = {}, const FloorFixtures& = {},
                const FloorObjectiveDefinition& = {}) = delete;
    GameSession(const Map&, rules::Catalog&&, const NpcWorld&, std::uint32_t, Player = {}, bool = true,
                const std::vector<NpcTimings>& = {}, const RecoveryLevel& = {}, const FloorFixtures& = {},
                const FloorObjectiveDefinition& = {}) = delete;
    GameSession(const Map&, const rules::Catalog&&, const NpcWorld&, std::uint32_t, Player = {}, bool = true,
                const std::vector<NpcTimings>& = {}, const RecoveryLevel& = {}, const FloorFixtures& = {},
                const FloorObjectiveDefinition& = {}) = delete;
    GameSession(Map&&, const rules::Catalog&, const rules::PresetParty&, const NpcWorld&, std::uint32_t,
                Player = {}, bool = true, const std::vector<NpcTimings>& = {}, const RecoveryLevel& = {},
                const FloorFixtures& = {}, const FloorObjectiveDefinition& = {}) = delete;
    GameSession(const Map&&, const rules::Catalog&, const rules::PresetParty&, const NpcWorld&, std::uint32_t,
                Player = {}, bool = true, const std::vector<NpcTimings>& = {}, const RecoveryLevel& = {},
                const FloorFixtures& = {}, const FloorObjectiveDefinition& = {}) = delete;
    GameSession(const Map&, rules::Catalog&&, const rules::PresetParty&, const NpcWorld&, std::uint32_t,
                Player = {}, bool = true, const std::vector<NpcTimings>& = {}, const RecoveryLevel& = {},
                const FloorFixtures& = {}, const FloorObjectiveDefinition& = {}) = delete;
    GameSession(const Map&, const rules::Catalog&&, const rules::PresetParty&, const NpcWorld&, std::uint32_t,
                Player = {}, bool = true, const std::vector<NpcTimings>& = {}, const RecoveryLevel& = {},
                const FloorFixtures& = {}, const FloorObjectiveDefinition& = {}) = delete;
    GameSession(const GameSession&) = delete;
    GameSession& operator=(const GameSession&) = delete;
    const FloorObjectiveSnapshot& objective() const noexcept { return objective_; }
    bool guardian(ActorId id) const noexcept;
    LandingSafety landingSafety(unsigned index) const noexcept;
    GeometryView geometry() const noexcept { return map_; }
    const FloorGeometry& fixtures() const noexcept { return floor_; }
    FixtureResult interact(const FixtureRequest&, bool preview = false) noexcept;
    FixtureRequest nearbyFixture() const noexcept;
    void discover() noexcept;
    Party& party() noexcept { return party_; }
    const Party& party() const noexcept { return party_; }
    ActorWorld& actors() noexcept { return actors_; }
    const ActorWorld& actors() const noexcept { return actors_; }
    CombatController& combat() noexcept { return combat_; }
    const CombatController& combat() const noexcept { return combat_; }
    const MovementState& movement() const noexcept { return movement_.state(); }
    std::uint64_t timeMs() const noexcept { return timeMs_; }
    const EffectController& effects() const noexcept { return effects_; }
    // All simulation operations invalidate the previous borrowed event batch.
    const CombatEvents& events() const noexcept { return events_; }
    EffectResult applyEffect(const EffectRequest&) noexcept;
    EffectResult removeEffect(EffectId) noexcept;
    EffectResult endConcentration(std::uint64_t world, rules::CharacterId) noexcept;
    const NpcAiSnapshot& ai() const noexcept { return ai_.snapshot(); }
    const CombatEvents& update(double seconds, bool paused) noexcept;
    void move(Action action, bool fresh = true) noexcept;
    void clearPending() noexcept { movement_.clearPending(); }
    void setAnimated(bool enabled) noexcept {
        movement_.setAnimated(enabled);
        discover();
    }
    InventoryError inventoryAccess(const InventoryRequest&) const noexcept;
    InventoryResult inventoryChange(const InventoryRequest&) noexcept;
    CombatOutcome submit(const PartyCommand& command) noexcept;
    const LootWorld& loot() const noexcept { return loot_; }
    PickupResult dropItem(ItemHandle, std::uint64_t inventoryRevision, bool preview = false) noexcept;
    PickupResult lootAccess(LootId) const noexcept;
    PickupResult pickup(const PickupRequest&, bool preview = false) noexcept;
    RestPreview previewRest(const RestRequest&) const noexcept;
    RestResult rest(const RestRequest&) noexcept;
    std::uint64_t restToken() const noexcept { return restToken_; }
    AdvancementRequest advancementRequest(rules::CharacterId) const noexcept;
    AdvancementError advancementEligibility(const AdvancementRequest&) const noexcept;
    AdvancementResult previewAdvancement(const AdvancementRequest&) noexcept;
    AdvancementResult advance(const AdvancementRequest&) noexcept;
    void enableDevelopment(bool enabled) noexcept { development_ = enabled; }
    bool development() const noexcept { return development_; }
    bool grantNextLevelExperience() noexcept;

  private:
    GameSession(const Map&, std::unique_ptr<Party>, const NpcWorld&, std::uint32_t, Player, bool,
                const std::vector<NpcTimings>&, const RecoveryLevel&, const FloorFixtures&,
                const FloorObjectiveDefinition&);
    friend class CampaignRun;
    CampaignRun* campaign_ = nullptr;
    std::uint64_t mutation_ = 0;
    FloorGeometry floor_;
    FloorObjectiveDefinition objectiveDefinition_;
    FloorObjectiveSnapshot objective_;
    void prepareObjective();
    void reconcileObjective() noexcept;
    GeometryView map_;
    std::unique_ptr<Party> ownedParty_;
    Party& party_;
    ActorWorld actors_;
    RecoveryLevel recovery_;
    LootWorld loot_;
    Controller movement_;
    rules::DiceRng rng_;
    NpcAiController ai_;
    EffectController effects_;
    CombatController combat_;
    CombatEvents events_;
    bool enableAi_ = true, development_ = false;
    std::array<bool, NpcWorld::maxInstances> creditedDeaths_{};
    std::uint64_t rewardRevision_ = UINT64_MAX;
    void reconcileRewards() noexcept;
    std::uint64_t timeMs_ = 0, restToken_ = 1;
    double remainderMs_ = 0;
    Player discoveredPose_;
    bool discoveredReady_ = false;
    std::uint64_t discoveredFixtures_ = 0;
    void detectContact() noexcept;
    void driveFacing() noexcept;
};
} // namespace blobber

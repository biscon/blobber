#pragma once
#include "game/dialogue.hpp"
#include "game/session.hpp"
#include "game/trade.hpp"
#include "game/travel.hpp"
#include "generation/population.hpp"
namespace blobber {
enum class CampaignStatus : std::uint8_t { Active, Victory, Defeat };
struct DoorwayLink {
    Player approach, arrival;
    CampaignLocationId destination = CampaignLocationId::City;
    const char* action = ""; // Static canonical label; never serialized.
};
struct CampaignLocation {
    std::optional<generation::GeneratedFloor> generated;
    std::optional<Map> authored;
    static constexpr unsigned doorwayCapacity = 3;
    std::array<DoorwayLink, doorwayCapacity> doorways{}; // Canonical; never restored from disk.
    unsigned doorwayCount = 0;
    std::optional<NpcWorld> placements;
    RecoveryLevel recovery;
    FloorFixtures fixtures;
    FloorObjectiveDefinition objective;
    std::unique_ptr<FloorState> saved;
    Player entry{}, exit{}; // Hub: recall / dungeon return; floors: upper / lower landing.
    std::uint32_t seed = 0;
    bool visited = false;
    const Map& map() const noexcept { return generated ? generated->map : *authored; }
};
struct CampaignLocationState {
    bool generated = false, visited = false;
    generation::GenerationRequest generation;
    generation::FloorLayout layout;
    generation::GenerationDiagnostics diagnostics;
    std::uint64_t fingerprint = 0;
    WorldDefinition world;
    std::uint64_t worldFingerprint = 0;
    std::vector<std::string> rows, types;
    std::vector<NpcInstance> placements;
    std::vector<ActorState> initialActors; // Preparation baseline, actual population only; never serialized.
    RecoveryLevel recovery;
    FloorFixtures fixtures;
    FloorObjectiveDefinition objective;
    std::unique_ptr<FloorState> state;
    Player entry{}, exit{}; // Hub: recall / dungeon return; floors: upper / lower landing.
    std::uint32_t seed = 0;
};
// Immutable preparation data shared by captures. Never contains floor snapshots.
struct CampaignDefinitions {
    std::uint32_t seed = 0;
    generation::RewardProfile rewardProfile;
    std::array<CampaignLocationState, campaignLocationCount> locations;
};
struct CampaignCheckpoint {
    std::shared_ptr<const CampaignDefinitions> baseline;
    DialogueHistory dialogue;
    std::vector<MerchantState> merchants;
    Party::Saved party;
    std::array<CampaignLocationState, campaignLocationCount> locations;
    Navigation::Saved navigation;
    Player pose;
    bool animated = true;
    unsigned activeLocation = 0;
    std::uint64_t gameplayMs = 0, revision = 1;
    std::string runId;
};
struct CampaignLocationSave {
    const CampaignLocationState* definition = nullptr; // Owned by CampaignSaveData::definitions.
    bool visited = false;
    std::unique_ptr<FloorState> state;
};
// Disk capture shares immutable definitions; mutable values are exclusively owned.
struct CampaignSaveData {
    std::shared_ptr<const CampaignDefinitions> baseline, definitions;
    DialogueHistory dialogue;
    std::vector<MerchantState> merchants;
    Party::Saved party;
    std::array<CampaignLocationSave, campaignLocationCount> locations;
    Navigation::Saved navigation;
    Player pose;
    bool animated = true;
    unsigned activeLocation = 0;
    std::uint64_t gameplayMs = 0, revision = 1;
    std::string runId;
};
std::array<CampaignLocation, campaignLocationCount>
prepareCampaignLocations(const blueprints::BlueprintCatalog& blueprints, const rules::Catalog&,
                         const std::vector<std::string>&, std::uint32_t seed,
                         generation::RewardProfile profile = {});
std::shared_ptr<const CampaignDefinitions>
campaignDefinitions(const std::array<CampaignLocation, campaignLocationCount>&, std::uint32_t seed,
                    const rules::Catalog&, const CampaignDefinitions* canonical = nullptr,
                    generation::RewardProfile profile = {});
std::shared_ptr<const CampaignDefinitions>
prepareCampaignDefinitions(const blueprints::BlueprintCatalog& blueprints, const rules::Catalog&,
                           const std::vector<std::string>&, std::uint32_t seed,
                           generation::RewardProfile profile = {});
class CampaignRun {
  public:
    class PreparedTravel {
      public:
        PreparedTravel() = default;
        PreparedTravel(PreparedTravel&&) = default;
        PreparedTravel& operator=(PreparedTravel&&) = default;
        GameSession& game() const noexcept { return *session; }
        unsigned destination() const noexcept { return to; }

      private:
        friend class CampaignRun;
        std::unique_ptr<GameSession> session;
        std::unique_ptr<FloorState> source;
        TravelRequest request;
        std::optional<Inventory> chargedInventory;
        rules::CharacterId supplier;
        rules::CharacterStats supplierStats;
        Player arrival;
        unsigned landing = 0;
        unsigned from = 0, to = 0;
        std::uint64_t mutation = 0, actorRevision = 0;
        std::array<std::uint64_t, 100> partyStamp{};
        std::array<std::uint64_t, 4> temporaryUntil{};
    };
    CampaignRun(const blueprints::BlueprintCatalog& blueprints, const rules::Catalog&,
                const rules::PresetParty&, const std::vector<std::string>& types,
                const std::vector<NpcTimings>& timings, std::uint32_t seed);
    CampaignRun(const blueprints::BlueprintCatalog& blueprints, const rules::Catalog&, CampaignCheckpoint&&,
                const std::vector<NpcTimings>&);
    std::unique_ptr<CampaignCheckpoint> checkpoint() const;
    std::unique_ptr<CampaignSaveData> captureSave() const;
    const char* saveSafety() const noexcept { return game().saveSafety(); }
    const std::string& runId() const noexcept { return runId_; }
    void preparePersistenceIdentity(std::string id); // New-owner preparation, before publication.

    CampaignRun(const CampaignRun&) = delete;
    CampaignRun& operator=(const CampaignRun&) = delete;
    GameSession& game() noexcept { return *active_; }
    const GameSession& game() const noexcept { return *active_; }
    DialogueHistory& dialogue() noexcept { return dialogue_; }
    MerchantLedger& merchants() noexcept { return merchants_; }
    Party& party() noexcept { return party_; }
    const CampaignLocation& location(unsigned i) const { return locations_.at(i); }
    const Map& map() const noexcept { return locations_[location_].map(); }
    unsigned activeLocation() const noexcept { return location_; }
    std::uint64_t revision() const noexcept { return revision_; }
    std::uint64_t gameplayMs() const noexcept;
    GameTime gameTime() const noexcept { return {gameplayMs()}; }
    CampaignStatus status() const noexcept;
    const char* name() const noexcept;
    TravelRequest request(FixtureRequest) const noexcept;
    CampaignLocationId activeLocationId() const noexcept { return CampaignLocationId(location_); }
    unsigned dungeonDepth() const noexcept { return dungeonLocation(location_) ? location_ : 0; }
    TravelRequest doorwayRequest() const noexcept;
    const DoorwayLink* doorway(DoorwayId) const noexcept;
    TravelRequest recallRequest() const noexcept;
    TravelRequest waypointRequest(unsigned depth) const noexcept;
    FixtureResult previewTravel(const TravelRequest&) const noexcept;
    PreparedTravel prepareTravel(const TravelRequest&); // Throws; source remains unchanged.
    bool commitTravel(PreparedTravel&) noexcept;        // Old session retained in ticket until its view dies.
    const CombatEvents& update(double seconds, bool paused) noexcept;
    static bool stairs(FixtureKind k) noexcept {
        return k == FixtureKind::Upstairs || k == FixtureKind::Downstairs;
    }
    std::size_t snapshotBytes() const noexcept { return campaignLocationCount * sizeof(FloorState); }

  private:
    DialogueHistory dialogue_;
    MerchantLedger merchants_;
    std::string runId_;
    Party party_;
    std::array<CampaignLocation, campaignLocationCount> locations_;
    std::vector<NpcTimings> timings_;
    unsigned location_ = 0;
    std::uint64_t revision_ = 1, elapsed_ = 0, baseClock_ = 0;
    std::unique_ptr<GameSession> active_;
    std::shared_ptr<const CampaignDefinitions> baseline_, definitions_;
    void configureDoorways();
    std::unique_ptr<GameSession> prepare(unsigned, Player, bool validate = false);
    std::array<std::uint64_t, 100> partyStamp() const noexcept;
};
} // namespace blobber

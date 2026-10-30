#pragma once
#include "game/session.hpp"
#include "game/travel.hpp"
#include "generation/population.hpp"
namespace blobber {
enum class CampaignStatus : std::uint8_t { Active, Victory, Defeat };
struct CampaignLocation {
    std::optional<generation::GeneratedFloor> generated;
    std::optional<Map> authored;
    std::optional<NpcWorld> placements;
    RecoveryLevel recovery;
    FloorFixtures fixtures;
    FloorObjectiveDefinition objective;
    std::unique_ptr<FloorState> saved;
    Player entry{}, exit{};
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
    std::vector<std::string> rows, types;
    std::vector<NpcInstance> placements;
    RecoveryLevel recovery;
    FloorFixtures fixtures;
    FloorObjectiveDefinition objective;
    std::unique_ptr<FloorState> state;
    Player entry{}, exit{};
    std::uint32_t seed = 0;
};
struct CampaignCheckpoint {
    Party::Saved party;
    std::array<CampaignLocationState, 4> locations;
    Navigation::Saved navigation;
    Player pose;
    bool animated = true;
    unsigned activeLocation = 0;
    std::uint64_t gameplayMs = 0, revision = 1;
    std::string runId;
};
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
    CampaignRun(const rules::Catalog&, const rules::PresetParty&, const std::vector<std::string>& types,
                const std::vector<NpcTimings>& timings, std::uint32_t seed);
    CampaignRun(const rules::Catalog&, CampaignCheckpoint&&, const std::vector<NpcTimings>&);
    std::unique_ptr<CampaignCheckpoint> checkpoint() const;
    const char* saveSafety() const noexcept { return game().saveSafety(); }
    const std::string& runId() const noexcept { return runId_; }
    void preparePersistenceIdentity(std::string id); // New-owner preparation, before publication.

    CampaignRun(const CampaignRun&) = delete;
    CampaignRun& operator=(const CampaignRun&) = delete;
    GameSession& game() noexcept { return *active_; }
    const GameSession& game() const noexcept { return *active_; }
    Party& party() noexcept { return party_; }
    const CampaignLocation& location(unsigned i) const { return locations_.at(i); }
    const Map& map() const noexcept { return locations_[location_].map(); }
    unsigned activeLocation() const noexcept { return location_; }
    std::uint64_t revision() const noexcept { return revision_; }
    std::uint64_t gameplayMs() const noexcept;
    CampaignStatus status() const noexcept;
    const char* name() const noexcept;
    TravelRequest request(FixtureRequest) const noexcept;
    TravelRequest recallRequest() const noexcept;
    TravelRequest waypointRequest(unsigned depth) const noexcept;
    FixtureResult previewTravel(const TravelRequest&) const noexcept;
    PreparedTravel prepareTravel(const TravelRequest&); // Throws; source remains unchanged.
    bool commitTravel(PreparedTravel&) noexcept;        // Old session retained in ticket until its view dies.
    const CombatEvents& update(double seconds, bool paused) noexcept;
    static bool stairs(FixtureKind k) noexcept {
        return k == FixtureKind::Upstairs || k == FixtureKind::Downstairs;
    }
    std::size_t snapshotBytes() const noexcept { return 4 * sizeof(FloorState); }

  private:
    std::string runId_;
    Party party_;
    std::array<CampaignLocation, 4> locations_;
    std::vector<NpcTimings> timings_;
    unsigned location_ = 0;
    std::uint64_t revision_ = 1, elapsed_ = 0, baseClock_ = 0;
    std::unique_ptr<GameSession> active_;
    std::unique_ptr<GameSession> prepare(unsigned, Player, bool validate = false);
    std::array<std::uint64_t, 100> partyStamp() const noexcept;
};
} // namespace blobber

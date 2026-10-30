#pragma once
#include "game/session.hpp"
#include "generation/population.hpp"
namespace blobber {
enum class CampaignStatus : std::uint8_t { Active, Victory, Defeat };
struct TravelRequest {
    std::uint64_t revision = 0;
    FixtureRequest fixture;
};
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
        unsigned from = 0, to = 0;
        std::uint64_t mutation = 0, actorRevision = 0;
        std::array<std::uint64_t, 100> partyStamp{};
        std::array<std::uint64_t, 4> temporaryUntil{};
    };
    CampaignRun(const rules::Catalog&, const rules::PresetParty&, const std::vector<std::string>& types,
                const std::vector<NpcTimings>& timings, std::uint32_t seed);
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
    TravelRequest request(FixtureRequest r) const noexcept { return {revision_, r}; }
    FixtureResult previewTravel(const TravelRequest&) const noexcept;
    PreparedTravel prepareTravel(const TravelRequest&); // Throws; source remains unchanged.
    bool commitTravel(PreparedTravel&) noexcept;        // Old session retained in ticket until its view dies.
    const CombatEvents& update(double seconds, bool paused) noexcept;
    static bool stairs(FixtureKind k) noexcept {
        return k == FixtureKind::Upstairs || k == FixtureKind::Downstairs;
    }
    std::size_t snapshotBytes() const noexcept { return 4 * sizeof(FloorState); }

  private:
    Party party_;
    std::array<CampaignLocation, 4> locations_;
    std::vector<NpcTimings> timings_;
    unsigned location_ = 0;
    std::uint64_t revision_ = 1, elapsed_ = 0, baseClock_ = 0;
    std::unique_ptr<GameSession> active_;
    std::unique_ptr<GameSession> prepare(unsigned, Player);
    std::array<std::uint64_t, 100> partyStamp() const noexcept;
};
} // namespace blobber

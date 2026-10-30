#pragma once
#include "assets/npc_metadata.hpp"
#include "automation/scenario.hpp"
#include "game/session.hpp"
#include "game/target_visibility.hpp"
#include "generation/population.hpp"
#include <memory>
namespace blobber::automation {
constexpr unsigned tickMs = 10, maxRequestTicks = 1000, maxRunTicks = 360000, maxMutations = 100000;
enum class Mode { Diagnostic, Visual };
// Optional adapter owns no gameplay; created once per session and sampled on demand.
class Visibility {
  public:
    virtual ~Visibility() = default;
    virtual const TargetSnapshot& sample(const Map&, const GameSession&) = 0;
    virtual bool lootVisible(LootId) const noexcept = 0;
};
using VisibilityFactory = std::unique_ptr<Visibility> (*)(void*, const Map&, const GameSession&);
struct Observation {
    const GameSession* session = nullptr;
    const Map* map = nullptr;
    TargetSnapshot targets;
    std::array<bool, NpcWorld::maxInstances> visible{};
    std::array<bool, lootCapacity> lootVisible{};
    bool diagnostic = false;
};
// A fixed-shape request uses the same typed commands as the live game. Parsing/JSON is external.
enum class Operation { Combat, Move, Inventory, Drop, Pickup, Rest, Advancement, Continue, Interact };
struct ActionRequest {
    Operation operation = Operation::Combat;
    PartyCommand combat;
    Action movement = Action::Forward;
    InventoryRequest inventory;
    ItemHandle item;
    PickupRequest pickup;
    RestRequest rest;
    AdvancementRequest advancement;
    FixtureRequest fixture;
};
struct ActionResult {
    bool accepted = false;
    const char* reason = "Unavailable";
};
class Session {
  public:
    Session(const rules::Catalog&, const NpcDefinitions&, const Scenario&, Mode, VisibilityFactory = nullptr,
            void* factoryContext = nullptr);
    const Scenario& scenario() const noexcept { return scenario_; }
    GameSession& replayGame() noexcept { return game_; } // For existing HUD presentation; never wire-exposed.
    const GameSession& game() const noexcept { return game_; }
    const Map& map() const noexcept { return map_; }
    bool arena() const noexcept { return std::holds_alternative<ArenaScenario>(scenario_); }
    const generation::GeneratedFloor* generated() const noexcept {
        return generated_ ? &*generated_ : nullptr;
    }
    void observe(Observation&, bool diagnostic = false);
    ActionResult action(const ActionRequest&, bool preview);
    const CombatEvents& tick() noexcept;
    const CombatEvents& events() const noexcept { return game_.events(); }
    bool terminal() const noexcept;
    bool result() const noexcept;
    bool decision() const noexcept;
    unsigned ticks() const noexcept { return ticks_; }
    Mode mode() const noexcept { return mode_; }

  private:
    Scenario scenario_;
    std::optional<generation::GeneratedFloor> generated_;
    std::optional<Map> arenaMap_;
    const Map& map_;
    std::optional<generation::PreparedPopulation> population_;
    NpcWorld placements_;
    GameSession game_;
    Mode mode_;
    unsigned ticks_ = 0;
    std::unique_ptr<Visibility> visibility_;
    Observation scratch_;
};
} // namespace blobber::automation

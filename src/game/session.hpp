#pragma once
#include "game/combat.hpp"
#include "game/loot.hpp"
#include "game/npc_ai.hpp"

namespace blobber {
// Map/catalog outlive this owner and every UI/animation borrower of its members.
class GameSession {
  public:
    GameSession(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements, std::uint32_t seed,
                Player spawn = {}, bool enableAi = true, const std::vector<NpcTimings>& timings = {},
                const RecoveryLevel& recovery = {});
    GameSession(const Map&, const rules::Catalog&, const rules::PresetParty&, const NpcWorld&,
                std::uint32_t seed, Player spawn = {}, bool enableAi = true,
                const std::vector<NpcTimings>& timings = {}, const RecoveryLevel& recovery = {});
    GameSession(Map&&, const rules::Catalog&, const NpcWorld&, std::uint32_t, Player = {}, bool = true,
                const std::vector<NpcTimings>& = {}, const RecoveryLevel& = {}) = delete;
    GameSession(const Map&&, const rules::Catalog&, const NpcWorld&, std::uint32_t, Player = {}, bool = true,
                const std::vector<NpcTimings>& = {}, const RecoveryLevel& = {}) = delete;
    GameSession(const Map&, rules::Catalog&&, const NpcWorld&, std::uint32_t, Player = {}, bool = true,
                const std::vector<NpcTimings>& = {}, const RecoveryLevel& = {}) = delete;
    GameSession(const Map&, const rules::Catalog&&, const NpcWorld&, std::uint32_t, Player = {}, bool = true,
                const std::vector<NpcTimings>& = {}, const RecoveryLevel& = {}) = delete;
    GameSession(Map&&, const rules::Catalog&, const rules::PresetParty&, const NpcWorld&, std::uint32_t,
                Player = {}, bool = true, const std::vector<NpcTimings>& = {},
                const RecoveryLevel& = {}) = delete;
    GameSession(const Map&&, const rules::Catalog&, const rules::PresetParty&, const NpcWorld&, std::uint32_t,
                Player = {}, bool = true, const std::vector<NpcTimings>& = {},
                const RecoveryLevel& = {}) = delete;
    GameSession(const Map&, rules::Catalog&&, const rules::PresetParty&, const NpcWorld&, std::uint32_t,
                Player = {}, bool = true, const std::vector<NpcTimings>& = {},
                const RecoveryLevel& = {}) = delete;
    GameSession(const Map&, const rules::Catalog&&, const rules::PresetParty&, const NpcWorld&, std::uint32_t,
                Player = {}, bool = true, const std::vector<NpcTimings>& = {},
                const RecoveryLevel& = {}) = delete;
    GameSession(const GameSession&) = delete;
    GameSession& operator=(const GameSession&) = delete;
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
    void setAnimated(bool enabled) noexcept { movement_.setAnimated(enabled); }
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

  private:
    const Map& map_;
    Party party_;
    ActorWorld actors_;
    RecoveryLevel recovery_;
    LootWorld loot_;
    Controller movement_;
    rules::DiceRng rng_;
    NpcAiController ai_;
    EffectController effects_;
    CombatController combat_;
    CombatEvents events_;
    bool enableAi_ = true;
    std::uint64_t timeMs_ = 0, restToken_ = 1;
    double remainderMs_ = 0;
    void detectContact() noexcept;
    void driveFacing() noexcept;
};
} // namespace blobber

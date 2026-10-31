#pragma once
#include "rules/content.hpp"
#include "world/fixtures.hpp"
#include "world/npc.hpp"
#include <array>
#include <limits>

namespace blobber {
struct ActorId {
    std::uint16_t slot = std::numeric_limits<std::uint16_t>::max();
    std::uint64_t world = 0;
};
enum class ActorLife : std::uint8_t { Alive, Dead, Removed };
inline constexpr std::uint64_t npcWalkDurationMs = 600;
struct ActorPosition {
    float x = 0, y = 0;
};
struct ActorState {
    NpcInstance npc;
    rules::MonsterId monster;
    std::int32_t hitPoints = 0, maxHitPoints = 0;
    ActorLife life = ActorLife::Alive;
    bool hasDeath = false; // Permanent authoritative marker; removal never erases it.
    bool hasDamage = false;
    std::uint64_t damageTimeMs = 0;
    bool hasAttack = false;
    bool rangedAttack = false;
    std::uint64_t attackTimeMs = 0;
    bool resumableAttack = false;
    std::uint64_t attackPausedMs = 0, attackPauseFromMs = 0, attackPauseUntilMs = 0;
    bool hasWalk = false;
    std::uint64_t walkTimeMs = 0, walkDurationMs = npcWalkDurationMs;
    bool hasTravel = false;
    ActorPosition walkFrom;
    float walkFromElevation = 0;
};
ActorPosition actorPosition(const ActorState& actor, std::uint64_t timeMs) noexcept;
float actorElevation(const ActorState&, GeometryView, std::uint64_t) noexcept;
bool actorWalking(const ActorState& actor, std::uint64_t timeMs) noexcept;
struct ActorSnapshot {
    std::array<ActorState, NpcWorld::maxInstances> actors{};
    std::size_t count = 0, typeCount = 0;
    std::uint64_t world = 0, revision = 0;
};
enum class ActorError {
    None,
    InvalidHandle,
    NotAlive,
    InvalidDestination,
    Occupied,
    PartyCell,
    InvalidFacing,
    InvalidDamage,
    InvalidTime
};
struct ActorResult {
    ActorError error = ActorError::None;
    bool changed = false, died = false;
    std::int32_t damage = 0;
    explicit operator bool() const noexcept { return error == ActorError::None; }
};
// Placement-only slots never recycle. Map/catalog must outlive this immovable owner.
class ActorWorld {
  public:
    ActorWorld(GeometryView map, const rules::Catalog& catalog, const NpcWorld& placements);
    ActorWorld(Map&&, const rules::Catalog&, const NpcWorld&) = delete;
    ActorWorld(GeometryView, rules::Catalog&&, const NpcWorld&) = delete;
    ActorWorld(const Map&&, const rules::Catalog&, const NpcWorld&) = delete;
    ActorWorld(GeometryView, const rules::Catalog&&, const NpcWorld&) = delete;
    ActorWorld(const ActorWorld&) = delete;
    ActorWorld& operator=(const ActorWorld&) = delete;
    const ActorSnapshot& snapshot() const noexcept { return state_; }
    ActorId id(std::size_t slot) const noexcept;
    const ActorState* actor(ActorId id) const noexcept;
    bool occupied(Cell cell) const noexcept;
    std::uint8_t occupiedSlots(Cell cell) const noexcept;
    ActorResult move(ActorId id, Cell cell, Cell subcell, Cell partyCell) noexcept;
    ActorResult walk(ActorId id, Cell cell, Cell subcell, Cell partyCell, std::uint64_t timeMs,
                     std::uint64_t durationMs = npcWalkDurationMs) noexcept;
    ActorResult face(ActorId id, Heading facing) noexcept;
    ActorResult attack(ActorId id, std::uint64_t timeMs, bool ranged = false) noexcept;
    ActorResult suspendAttack(ActorId, std::uint64_t when, std::uint64_t duration) noexcept;
    ActorResult cancelAttack(ActorId) noexcept;
    ActorResult damage(ActorId id, std::int64_t amount, std::uint64_t timeMs) noexcept;
    ActorResult remove(ActorId id) noexcept;
    GeometryView map() const noexcept { return map_; }
    const rules::Catalog& catalog() const noexcept { return catalog_; }

  private:
    friend class GameSession;
    friend class CampaignRun;
    GeometryView map_;
    const rules::Catalog& catalog_;
    ActorSnapshot state_;
    std::vector<std::uint8_t> occupancy_;
    std::uint8_t& mask(Cell cell) noexcept;
    void vacate(const ActorState& actor) noexcept;
};
} // namespace blobber

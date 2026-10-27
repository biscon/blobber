#pragma once
#include "game/command_types.hpp"
#include "game/movement.hpp"

namespace blobber {
constexpr std::size_t attackCapacity = NpcWorld::maxInstances + rules::partySize;
struct CombatAttackId {
    std::uint64_t world = 0, serial = 0;
};
enum class AttackStage : std::uint8_t { Empty, Windup, Flight, Resolved, Canceled };
// Read-only presentation data, never used to decide health or collision.
struct AttackVisual {
    CombatAttackId id;
    AttackStage stage = AttackStage::Empty;
    ActorId enemy;
    rules::CharacterId character;
    bool incoming = false, ranged = false, hit = false, targetGone = false;
    std::uint64_t startedMs = 0, releaseMs = 0, impactMs = 0, endMs = 0;
    Player partyOrigin;
    NpcInstance npcOrigin;
    ActorPosition sourcePosition;
    ActorPosition targetPosition;
    std::uint8_t formationSlot = 0;
};
struct AttackVisualSnapshot {
    std::array<AttackVisual, attackCapacity> attacks{};
    std::uint64_t world = 0, revision = 0;
};
std::uint32_t arrowFlightMs(Cell from, Cell to) noexcept;
} // namespace blobber

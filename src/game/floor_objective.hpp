#pragma once
#include "game/actors.hpp"
#include "game/recovery.hpp"
namespace blobber {
// Prepared immutable membership. The final credited death earns the survey bonus.
struct EncounterAward {
    std::array<char, 32> name{};
    std::uint8_t first = 0, count = 0;
    std::uint16_t experience = 0;
};
struct FloorObjectiveDefinition {
    std::array<std::uint16_t, 32> members{};
    unsigned count = 0;
    std::uint8_t approachDoor = 0, gate = 0;
    std::array<GroundPlacement, 2> rewards{};
    unsigned rewardCount = 0;
    std::uint32_t lootSeed = 0;
    std::array<EncounterAward, 8> awards{};
    unsigned awardCount = 0;
    bool gatedVault = false;
};
struct FloorObjectiveSnapshot {
    std::array<ActorId, 32> members{};
    unsigned count = 0;
    bool known = false, completed = false, rewardIssued = false, capacityFailure = false;
    std::uint64_t revision = 0;
};
struct LandingSafety {
    bool safe = false;
    const char* reason = "Landing unavailable.";
};
} // namespace blobber

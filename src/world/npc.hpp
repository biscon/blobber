#pragma once
#include "rules/content.hpp"
#include "world/map.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace blobber {
struct NpcTypeId {
    std::uint16_t index = 0;
};
enum class NpcAnimation : std::uint8_t { Idle, Walk, Attack, Hurt, Death, RangedAttack, Prone };
constexpr std::size_t npcAnimationCount = 7;
enum class NpcRoutine : std::uint8_t { Guard, Patrol, Wander };
struct NpcAiConfig {
    NpcRoutine routine = NpcRoutine::Guard;
    std::array<Cell, 8> waypoints{};
    std::uint8_t waypointCount = 0;
};
struct NpcInstance {
    NpcTypeId type;
    Cell cell;
    Cell subcell;
    Heading facing = Heading::North;
    NpcAnimation animation = NpcAnimation::Idle;
    std::uint32_t phaseMs = 0;
    NpcAiConfig ai{};
    // Optional rules variant sharing this appearance; local to the session catalog.
    std::optional<rules::MonsterId> monster{};
};
// Immutable level-lifetime indices: no removal, recycling or runtime spawning.
class NpcWorld {
  public:
    static constexpr std::size_t maxTypes = 32, maxInstances = 256;
    NpcWorld(const Map& map, Cell playerSpawn, std::vector<std::string> types,
             std::vector<NpcInstance> instances);
    bool occupied(Cell cell) const noexcept;
    const std::vector<NpcInstance>& instances() const noexcept { return instances_; }
    const std::string& typeId(NpcTypeId type) const { return types_.at(type.index); }
    std::size_t typeCount() const noexcept { return types_.size(); }

  private:
    int width_, height_;
    std::vector<std::string> types_;
    std::vector<NpcInstance> instances_;
    std::vector<std::uint8_t> occupancy_;
};
} // namespace blobber

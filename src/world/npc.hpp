#pragma once
#include "rules/content.hpp"
#include "world/map.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace blobber {
// Explicit temporary content alias; ordinary IDs retain their own appearance.
inline std::string_view monsterAppearance(std::string_view id) noexcept {
    return id == "goblin.guardian.placeholder" ? "goblin" : id;
}
struct NpcTypeId {
    std::uint16_t index = 0;
};
enum class NpcAnimation : std::uint8_t { Idle, Walk, Attack, Hurt, Death, RangedAttack, Prone, Talk, Laugh };
constexpr std::size_t npcAnimationCount = 9;
constexpr bool directionalIdleFallback(NpcAnimation clip) noexcept {
    return clip == NpcAnimation::Attack || clip == NpcAnimation::Talk || clip == NpcAnimation::Laugh;
}
enum class NpcRole : std::uint8_t { Hostile, Resident };
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
    NpcRole role = NpcRole::Hostile;
    std::uint64_t residentId = 0; // Stable per-character identity; never an appearance/actor slot.
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

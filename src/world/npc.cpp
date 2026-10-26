#include "world/npc.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace blobber {
NpcWorld::NpcWorld(const Map& map, Cell spawn, std::vector<std::string> types,
                   std::vector<NpcInstance> instances)
    : width_(map.width()), height_(map.height()), types_(std::move(types)), instances_(std::move(instances)),
      occupancy_(static_cast<std::size_t>(width_) * height_, 0) {
    if (types_.size() > maxTypes || instances_.size() > maxInstances)
        throw std::runtime_error("NPC level exceeds type/instance capacity");
    for (std::size_t i = 0; i < types_.size(); ++i) {
        if (types_[i].empty() || types_[i].size() > 128)
            throw std::runtime_error("NPC type ID must contain 1..128 bytes");
        for (std::size_t j = 0; j < i; ++j)
            if (types_[i] == types_[j])
                throw std::runtime_error("Duplicate NPC type: " + types_[i]);
    }
    // Static connectivity is loading-only work, shared by all patrol validation.
    std::vector<int> components;
    if (std::any_of(instances_.begin(), instances_.end(),
                    [](const auto& n) { return n.ai.routine == NpcRoutine::Patrol; })) {
        components.assign(occupancy_.size(), -1);
        std::vector<Cell> queue;
        queue.reserve(occupancy_.size());
        int component = 0;
        for (int y = 0; y < height_; ++y)
            for (int x = 0; x < width_; ++x) {
                const Cell start{x, y};
                if (map.solid(start) || components[y * width_ + x] != -1)
                    continue;
                queue.clear();
                queue.push_back(start);
                components[y * width_ + x] = component;
                for (std::size_t head = 0; head < queue.size(); ++head) {
                    for (int d = 0; d < 4; ++d) {
                        const auto c = queue[head] + forward(static_cast<Heading>(d));
                        if (map.solid(c) || components[c.y * width_ + c.x] != -1)
                            continue;
                        components[c.y * width_ + c.x] = component;
                        queue.push_back(c);
                    }
                }
                ++component;
            }
    }
    for (const auto& npc : instances_) {
        if (npc.type.index >= types_.size() || npc.animation > NpcAnimation::Attack ||
            static_cast<unsigned>(npc.facing) >= 4 || npc.subcell.x < 0 || npc.subcell.x > 1 ||
            npc.subcell.y < 0 || npc.subcell.y > 1)
            throw std::runtime_error("Invalid NPC type, facing or subcell");
        if (map.solid(npc.cell) || npc.cell == spawn)
            throw std::runtime_error("NPC placement overlaps wall, boundary or player spawn");
        if (npc.ai.routine > NpcRoutine::Wander || npc.ai.waypointCount > npc.ai.waypoints.size() ||
            (npc.ai.routine == NpcRoutine::Patrol ? npc.ai.waypointCount == 0 : npc.ai.waypointCount != 0))
            throw std::runtime_error("Invalid NPC AI routine or waypoint count");
        for (std::size_t i = 0; i < npc.ai.waypointCount; ++i) {
            const auto c = npc.ai.waypoints[i];
            if (map.solid(c) ||
                components[c.y * width_ + c.x] != components[npc.cell.y * width_ + npc.cell.x])
                throw std::runtime_error("NPC patrol waypoint is blocked or unreachable from home");
        }
        auto& mask = occupancy_[static_cast<std::size_t>(npc.cell.y) * width_ + npc.cell.x];
        const auto bit = static_cast<std::uint8_t>(1u << (npc.subcell.y * 2 + npc.subcell.x));
        if (mask & bit)
            throw std::runtime_error("Duplicate NPC subcell placement");
        mask |= bit;
    }
}
bool NpcWorld::occupied(Cell c) const noexcept {
    return c.x >= 0 && c.y >= 0 && c.x < width_ && c.y < height_ &&
           occupancy_[static_cast<std::size_t>(c.y) * width_ + c.x] != 0;
}
} // namespace blobber

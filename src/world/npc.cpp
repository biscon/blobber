#include "world/npc.hpp"
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
    for (const auto& npc : instances_) {
        if (npc.type.index >= types_.size() || static_cast<unsigned>(npc.animation) >= npcAnimationCount ||
            static_cast<unsigned>(npc.facing) >= 4 || npc.subcell.x < 0 || npc.subcell.x > 1 ||
            npc.subcell.y < 0 || npc.subcell.y > 1)
            throw std::runtime_error("Invalid NPC type, facing or subcell");
        if (map.solid(npc.cell) || npc.cell == spawn)
            throw std::runtime_error("NPC placement overlaps wall, boundary or player spawn");
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

#pragma once
#include "game/actors.hpp"
#include "game/movement.hpp"

namespace blobber {
enum class ApproachRank : unsigned { Hidden = 1, VisibleRear, VisibleNear };
struct ApproachSlot {
    Cell subcell;
    ApproachRank rank;
};
// Borrowed live occupancy; searching never reserves a slot. Arrival queries may
// include the actor's own occupied slot, while commitment only chooses free slots.
std::optional<ApproachSlot> approachSlot(GeometryView, const ActorWorld&, ActorId, Player, Cell destination,
                                         bool includeSelf = false) noexcept;
} // namespace blobber
